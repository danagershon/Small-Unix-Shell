#include "SpecialCommand.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

SpecialCommand::SpecialCommand(std::string cmd_line)
    : Command(cmd_line), isBgCommand(_isBackgroundCommand(cmd_line)),
      bash_path("/bin/bash"), bash_flag("-c") {}

void SpecialCommand::handleChildProccess(pid_t child_pid) {
    SmallShell& smash = SmallShell::getInstance();
    int status;
    if (isBgCommand) {
        if (waitpid(child_pid, &status, WNOHANG) == child_pid){
            // child proccess failed
            checkChildExitStatus(status);
        }
        // new job, never been in job list before
        // putting the whole redirection cmd in list (so we have full cmd line)
        smash.jobs.addJob(this, child_pid, BG);
        return;
    }
    // otherwise the child runs in fg and smash waits
    smash.updateFgCommandInfo(this, child_pid);
    int result = waitpid(smash.fg_pid, &status, WUNTRACED);
    if (result == -1){ // waitpid failed
        perror("smash error: waitpid failed");
        throw SystemCallFail();
    }
    // check if child exited with _exit due to failure
    checkChildExitStatus(status);
    if (WIFSTOPPED(status) || WIFSIGNALED(status)) {
        // child proccess was stopped or killed by user
        // smash got signal and we are here after signal handler
        return;
    }
    // we get here if child and waitpid were successful
    // delete smash.fg_cmd;
    smash.resetFgCommandInfo();
}

//----------------------------------------------------------------------------

PipeCommand::PipeCommand(std::string cmd_line)
    : SpecialCommand(cmd_line), left_cmd_line(""), right_cmd_line(""),
      output_channel(-1), writing_proccess_pid(-1), reading_proccess_pid(-1){
    pipe_fd[0] = -1;
    pipe_fd[1] = -1;
}

RedirectionCommand::RedirectionCommand(std::string cmd_line)
    : SpecialCommand(cmd_line), file_name(""), cmd_line_to_run(""), flags(0),
      fd_output_file(-1) {}

CopyCommand::CopyCommand(std::string cmd_line)
    : SpecialCommand(cmd_line), src_file_path(""), dest_file_path(""),
      src_fd(-1), dest_fd(-1) {}

//----------------------------------------------------------------------------

std::string RedirectionCommand::getOutputFileName() {
    size_t right_side_start_pos = cmd_line.find_last_of('>') + 1;

    std::string right_side = cmd_line.substr(right_side_start_pos);

    auto files_names = _parseCommandLine(right_side);
    if (files_names.empty()) {
        return "";
    }

    _removeBackgroundSign(files_names[0]);
    return _trim(files_names[0]);
}

std::string RedirectionCommand::getCmdLineToRun() {
    std::string cmd_line_to_run = cmd_line.substr(0, cmd_line.find('>'));
    return _trim(cmd_line_to_run);
}

int RedirectionCommand::getOpeningFileFlags() {
    bool is_redirection_appending = (cmd_line.find(">>") != std::string::npos);

    int flags;
    if (!is_redirection_appending) {
        // O_TRUNC means that if file exists, overwrite it
        // O_CREAT means that if file doesn't exists, create it
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    }

    return flags;
}

void RedirectionCommand::prepare() {
    file_name = getOutputFileName();
    cmd_line_to_run = getCmdLineToRun();
    flags = getOpeningFileFlags();
}

SmallShellNextState RedirectionCommand::doRedirectionInSmash() {
    SmallShell& smash = SmallShell::getInstance();
    Command* cmd = smash.createCommand(cmd_line_to_run);

    // saving STDOUT in another FTD entry, so we can recover it later
    int std_out = dup(STDOUT_FILENO);
    if (std_out == -1) {
        perror("smash error: dup failed");
        throw SystemCallFail();
    }
    fd_output_file = open(file_name.c_str(), flags, 0666);
    if (fd_output_file == -1) {
        perror("smash error: open failed");
        close(std_out);
        throw SystemCallFail();
    }
    // we want STDOUT entry to point to output file obj
    if (dup2(fd_output_file, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
        close(std_out);
        close(fd_output_file);
        throw SystemCallFail();
    }
    // no need for the open entry of fd_output_file after duplication
    if (close(fd_output_file) == -1) {
        perror("smash error: close failed");
        dup2(std_out, STDOUT_FILENO);
        close(std_out);
        throw SystemCallFail();
    }

    SmallShellNextState smash_next_state;

    try {
        smash_next_state = cmd->execute();
    } catch (ExecutionFail& execution_fail) {
        dup2(std_out, STDOUT_FILENO);
        close(std_out);
        throw execution_fail;
    }

    // recover STDOUT entry original state
    if (dup2(std_out, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
        throw SystemCallFail();
    }
    if (close(std_out) == -1) {
        perror("smash error: close failed");
        throw SystemCallFail();
    }

    return smash_next_state;
}

SmallShellNextState RedirectionCommand::doRedirectionOfExternalCmd() {
    char* cmd_line_for_bash = const_cast<char*>(cmd_line_to_run.c_str());
    char* const args[] = {bash_path, bash_flag, cmd_line_for_bash, NULL};

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        throw SystemCallFail();
    }
    if (pid == 0){ // child proccess
        changeGroupID();
        fd_output_file = open(file_name.c_str(), flags, 0666);
        if (fd_output_file == -1) {
            perror("smash error: open failed");
            _exit(OPEN_FAILED);
        }
        if (dup2(fd_output_file, STDOUT_FILENO) == -1){
            perror("smash error: dup2 failed");
            _exit(DUP2_FAILED);
        }
        if (close(fd_output_file) == -1) {
            _exit(CLOSE_FAILED);
        }
        execv(args[0], args);
        perror("smash error: execv failed");
        _exit(COMMAND_NOT_RUNNABLE);
    } else { // smash proccess
        handleChildProccess(pid);
    }

    return CONTINUE_RUNNING;
}

SmallShellNextState RedirectionCommand::execute() {
    prepare();
    if (file_name.empty() || cmd_line_to_run.empty()) {
        // file path or inner cmd don't appear in cmd_line
        return CONTINUE_RUNNING;
    }

    // for built-in cmd we do the redirection from smash proccess
    if (isBuiltInCommand(cmd_line_to_run)) {
        return doRedirectionInSmash();
    }

    // external command
    return doRedirectionOfExternalCmd();
}

//----------------------------------------------------------------------------

std::string PipeCommand::getPipeLeftCommand() {
    return _trim(cmd_line.substr(0,cmd_line.find('|')));
}

std::string PipeCommand::getPipeRightCommand() {
    std::string right_cmd;
    if (cmd_line.find("|&") != std::string::npos) {
        right_cmd = cmd_line.substr(cmd_line.find('&')+1);
    } else {
        right_cmd = cmd_line.substr(cmd_line.find('|')+1);
    }

    right_cmd = _trim(right_cmd);
    _removeBackgroundSign(right_cmd);
    return _trim(right_cmd);
}

void PipeCommand::prepare() {
    left_cmd_line = getPipeLeftCommand();
    right_cmd_line = getPipeRightCommand();

    if (cmd_line.find("|&") != std::string::npos) {
        output_channel = STDERR_FILENO;
    } else {
        output_channel = STDOUT_FILENO;
    }
}

void PipeCommand::runPipeFromSon() {
    SmallShell& smash = SmallShell::getInstance();

    if (pipe(pipe_fd) == -1) {
        perror("smash error: pipe failed");
        _exit(PIPE_FAILED);
    }

    writing_proccess_pid = fork();
    if (writing_proccess_pid == -1) {
        perror("smash error: fork failed");
        _exit(FORK_FAILED);
    }
    if (writing_proccess_pid == 0) {
        // no changing of groupID
        smash.prev_wd_path = "";
        if (dup2(pipe_fd[1], output_channel) == -1) {
            perror("smash error: dup2 failed");
            _exit(DUP2_FAILED);
        }
        if (close(pipe_fd[0]) == -1 || close(pipe_fd[1]) == -1) {
            perror("smash error: close failed");
            _exit(CLOSE_FAILED);
        }
        if (isBuiltInCommand(left_cmd_line)) {
            Command* left_cmd = smash.createCommand(left_cmd_line);
            if (left_cmd_line.find("cp ") == 0) {
                left_cmd->execute_without_fork = true;
            }
            try {
                left_cmd->execute();
            } catch (ExecutionFail& e) {
                _exit(0);
            }
            _exit(0);
        } else { // left cmd is external
            char* cmd_line_for_bash = const_cast<char*>(left_cmd_line.c_str());
            char* const args[] = {bash_path, bash_flag, cmd_line_for_bash, NULL};
            execv(args[0], args);
            perror("smash error: execv failed");
            _exit(COMMAND_NOT_RUNNABLE);
        }
    }
    // first son

    reading_proccess_pid = fork();
    if (reading_proccess_pid == -1) {
        perror("smash error: fork failed");
        _exit(FORK_FAILED);
    }
    if (reading_proccess_pid == 0) {
        smash.prev_wd_path = "";
        if (dup2(pipe_fd[0], STDIN_FILENO) == -1) {
            perror("smash error: dup2 failed");
            _exit(DUP2_FAILED);
        }
        if (close(pipe_fd[0]) == -1 || close(pipe_fd[1]) == -1) {
            perror("smash error: close failed");
            _exit(CLOSE_FAILED);
        }
        if (isBuiltInCommand(right_cmd_line)) {
            Command* right_cmd = smash.createCommand(right_cmd_line);
            if (right_cmd_line.find("cp ") == 0) {
                right_cmd->execute_without_fork = true;
            }
            try {
                right_cmd->execute();
            } catch (ExecutionFail& e) {
                _exit(0);
            }
            _exit(0);
        } else { // right cmd is external
            char* cmd_line_for_bash = const_cast<char*>(right_cmd_line.c_str());
            char* const args[] = {bash_path, bash_flag, cmd_line_for_bash, NULL};
            execv(args[0], args);
            perror("smash error: execv failed");
            _exit(COMMAND_NOT_RUNNABLE);
        }
    }
    // first son
    close(pipe_fd[1]);
    close(pipe_fd[0]);

    waitpid(writing_proccess_pid, NULL, WUNTRACED);
    waitpid(reading_proccess_pid, NULL, WUNTRACED);
}

SmallShellNextState PipeCommand::execute() {
    prepare();
    if (left_cmd_line.empty() || right_cmd_line.empty()) {
        return CONTINUE_RUNNING;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        throw SystemCallFail();
    }
    if (pid == 0) { // first son proccess, runs pipe
        changeGroupID();
        SmallShell& smash = SmallShell::getInstance();
        smash.prev_wd_path = "";
        runPipeFromSon();
        _exit(0);
    } else { // original father
        handleChildProccess(pid);
    }

    return CONTINUE_RUNNING;
}

//----------------------------------------------------------------------------

std::string CopyCommand::getSourceFilePath() {
    auto args = _parseCommandLine(cmd_line);
    if (args.size() < 3) {
        perror("smash error: cp: invalid arguments");
        return "";
    }
    // "cp <src> <dest> bla"
    return args[1];
}

std::string CopyCommand::getDestFilePath() {
    auto args = _parseCommandLine(cmd_line);
    if (args.size() < 3) {
        return "";
    }
    // "cp <src> <dest> bla" or "cp <src> <dest> bla&"
    // or "cp <src> <dest> bla &"
    _removeBackgroundSign(args[2]);
    return args[2];
}

bool CopyCommand::checkIfSameFile() {
    char real_src_path[PATH_MAX];
    realpath(src_file_path.c_str(), real_src_path);

    char real_dest_path[PATH_MAX];
    realpath(dest_file_path.c_str(), real_dest_path);

    return strcmp(real_src_path, real_dest_path) == 0;
}

void CopyCommand::prepare() {
    src_file_path = getSourceFilePath();
    dest_file_path = getDestFilePath();
}

void CopyCommand::printCopyingMsg() {
    std::cout << "smash: " << src_file_path << " was copied to "
              << dest_file_path << std::endl;
}

bool CopyCommand::doesFileExist(std::string& file_name) {
    int fd = open(file_name.c_str(), O_RDONLY);
    if (fd != -1) {
        close(fd);
        return true;
    } else {
        perror("smash error: open failed");
        return false;
    }
}

bool CopyCommand::isCopyingNeeded() {
    if (src_file_path.empty() || dest_file_path.empty()) {
        // one of file paths (or both) doesn't exist in cmd line
        return false;
    }

    if (checkIfSameFile()) {
        if (doesFileExist(src_file_path)) {
            printCopyingMsg();
        }
        return false;
    }

    return true;
}

void CopyCommand::openSrcDestFiles() {
    src_fd = open(src_file_path.c_str(), O_RDONLY);
    if (src_fd == -1) {
        perror("smash error: open failed");
        _exit(OPEN_FAILED);
    }
    dest_fd = open(dest_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd == -1) {
        perror("smash error: open failed");
        _exit(OPEN_FAILED);
    }
}

void CopyCommand::copySrcToDest() {
    openSrcDestFiles();
    ssize_t bytes_read_count;
    ssize_t bytes_written_count;
    char buff[buff_size];

    // zero indicates end of file
    // -1 indicates error
    while ((bytes_read_count = read(src_fd, buff, sizeof(buff))) > 0) {
        bytes_written_count = write(dest_fd, buff, (size_t)bytes_read_count);
        if (bytes_written_count != bytes_read_count) {
            perror("smash error: write failed");
            _exit(WRITE_FAILED);
        }
    }

    if (close(src_fd) == -1 || close(dest_fd) == -1) {
        perror("smash error: close failed");
        _exit(CLOSE_FAILED);
    }
}

SmallShellNextState CopyCommand::execute() {
    prepare();
    if(!isCopyingNeeded()) {
        return CONTINUE_RUNNING;
    }

    // happens if the cp is part of a pipe
    if (execute_without_fork) {
        copySrcToDest();
        printCopyingMsg();
        return CONTINUE_RUNNING;
    }

    // otherwise, the copying is done in a child proccess
    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        throw SystemCallFail();
    }
    if (pid == 0) { // child proccess
        changeGroupID();
        copySrcToDest();
        printCopyingMsg();
        _exit(0);
    } else { // smash proccess
        handleChildProccess(pid);
    }

    return CONTINUE_RUNNING;
}
