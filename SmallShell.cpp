#include "SmallShell.h"
#include "utilities.h"

#include <sys/types.h>
#include <signal.h>

// SmallShell constructor
SmallShell::SmallShell()
        : curr_prompt_str("smash> "), prev_wd_path(""), fg_pid(NO_FG_PROCCESS),
          fg_cmd(nullptr), fg_cmd_prev_jobID(FG_COMMAND_WASNT_IN_JOBLIST_BEFORE),
          smash_pid(getpid())
{}

// SmallShell destructor
SmallShell::~SmallShell() {
    //delete fg_cmd;
}

SmallShell& SmallShell::getInstance() {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
}

/* Creates and returns a pointer to Command class which matches the given
 * command line */
Command* SmallShell::createCommand(std::string& cmd_line) {
    auto cmd_args = _parseCommandLine(cmd_line);
    Command* cmd_obj = nullptr;

    if (cmd_args.empty()) {
        // empty command
        return nullptr;
    }

    _removeBackgroundSign(cmd_args[cmd_args.size()-1]);
    std::string& command_name = cmd_args[0];

    if (isRedirectionCommand(cmd_line)){
        cmd_obj = new RedirectionCommand(cmd_line);
    }
    else if (isPipeCommand(cmd_line)){
        cmd_obj = new PipeCommand(cmd_line);
    }
    else if (command_name == "chprompt") {
        cmd_obj = new ChangePromptCommand(cmd_line);
    }
    else if (command_name == "showpid"){
        cmd_obj = new ShowPidCommand(cmd_line);
    }
    else if (command_name == "pwd"){
        cmd_obj = new GetCurrDirCommand(cmd_line);
    }
    else if (command_name == "cd"){
        cmd_obj = new ChangeDirCommand(cmd_line);
    }
    else if (command_name == "jobs"){
        cmd_obj = new JobsCommand(cmd_line);
    }
    else if (command_name == "kill"){
        cmd_obj = new KillCommand(cmd_line);
    }
    else if (command_name == "fg"){
        cmd_obj = new ForegroundCommand(cmd_line);
    }
    else if (command_name == "bg"){
        cmd_obj = new BackgroundCommand(cmd_line);
    }
    else if (command_name == "quit"){
        cmd_obj = new QuitCommand(cmd_line);
    }
    else if (command_name == "cp"){
        cmd_obj = new CopyCommand(cmd_line);
    }
    else {
        cmd_obj = new ExternalCommand(cmd_line);
    }

    return cmd_obj;
}

SmallShellNextState SmallShell::executeCommand(std::string cmd_line) {
    Command* cmd = createCommand(cmd_line);
    if (cmd == nullptr) {
        return CONTINUE_RUNNING;
    }

    jobs.removeFinishedJobs();
    return cmd->execute();
}

void SmallShell::stopFgProccess() {
    if (fg_pid == NO_FG_PROCCESS){
        return;
    }

    // killpg instead of kill
    if (killpg(fg_pid, SIGSTOP) == -1){
        perror("smash error: kill failed");
        throw SystemCallFail();
    }

    std::cout << "smash: process " << fg_pid << " was stopped" << std::endl;
    jobs.addJob(fg_cmd, fg_pid, STOPPED, fg_cmd_prev_jobID);

    resetFgCommandInfo();
}

void SmallShell::killFgProccess() {
    if (fg_pid == NO_FG_PROCCESS) {
        return;
    }

    if (killpg(fg_pid, SIGKILL) == -1) {
        perror("smash error: kill failed");
        throw SystemCallFail();
    }

    std::cout << "smash: process " << fg_pid << " was killed" << std::endl;
    jobs.removeJobById(fg_cmd_prev_jobID);

    resetFgCommandInfo();
}

SmallShellResult SmallShell::updateFgCommand(Command* new_fg_cmd,
        pid_t new_fg_pid, int original_jobID) {
    // brings a stopped or bg job that was in job list, to run in fg
    updateFgCommandInfo(new_fg_cmd, new_fg_pid, original_jobID);

    if (killpg(fg_pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
        resetFgCommandInfo();
        throw SystemCallFail();
    }

    int status;
    pid_t result = waitpid(fg_pid, &status, WUNTRACED);
    if (result == -1) { // waitpid failed
        perror("smash error: waitpid failed");
        resetFgCommandInfo();
        throw SystemCallFail();
    }

    if (WIFSTOPPED(status) || WIFSIGNALED(status)) {
        // child proccess was stopped or killed by user
        // smash got signal and we are here after signal handle
        return FG_COMMAND_GOT_SIGNAL;
    }

    // otherwise, fg cmd completed
    resetFgCommandInfo();

    return FG_COMMAND_COMPLETED;
}

void SmallShell::resetFgCommandInfo() {
    // delete fg_cmd;
    fg_cmd = nullptr;
    fg_pid = NO_FG_PROCCESS;
    fg_cmd_prev_jobID = FG_COMMAND_WASNT_IN_JOBLIST_BEFORE;
}

void SmallShell::updateFgCommandInfo(Command* new_cmd, pid_t new_pid,
        int prev_jobID) {
    fg_pid = new_pid;
    fg_cmd = new_cmd;
    fg_cmd_prev_jobID = prev_jobID;
}

