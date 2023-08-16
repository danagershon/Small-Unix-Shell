#include "ExternalCommand.h"

ExternalCommand::ExternalCommand(std::string cmd_line)
    : Command(cmd_line), isBgCommand(_isBackgroundCommand(cmd_line)),
      bash_path("/bin/bash"), bash_flag("-c") {}


void ExternalCommand::handleChildProccess(pid_t child_pid) {
    SmallShell& smash = SmallShell::getInstance();
    int status;

    if (isBgCommand) {
        if (waitpid(child_pid, &status, WNOHANG) == child_pid){
            // child proccess failed
            checkChildExitStatus(status);
        }
        // new job, never been in job list before
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
    smash.resetFgCommandInfo();
}

SmallShellNextState ExternalCommand::execute() {
    if (isBgCommand) {
        _removeBackgroundSign(cmd_line);
    }

    char* cmd_line_for_bash = const_cast<char*>(cmd_line.c_str());
    char* const args[] = {bash_path, bash_flag, cmd_line_for_bash, NULL};

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        throw SystemCallFail();
    }
    if (pid == 0){ // child proccess
        changeGroupID();
        execv(args[0], args);
        perror("smash error: execv failed");
        _exit(COMMAND_NOT_RUNNABLE);
    }

    // smash proccess
    handleChildProccess(pid);

    return CONTINUE_RUNNING;
}
