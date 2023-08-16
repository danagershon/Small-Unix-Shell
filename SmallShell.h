#ifndef HW1_SMALLSHELL_H
#define HW1_SMALLSHELL_H

#include "Command.h"
#include "BuiltInCommand.h"
#include "ExternalCommand.h"
#include "SpecialCommand.h"
#include "JobList.h"

const int NO_FG_PROCCESS = 0;
const int FG_COMMAND_WASNT_IN_JOBLIST_BEFORE = 0;

// exit values indicating errors
const int COMMAND_NOT_RUNNABLE  = 127;
const int SETPGRP_FAILED = 126;
const int DUP2_FAILED = 125;
const int CLOSE_FAILED = 124;
const int OPEN_FAILED = 123;
const int WRITE_FAILED = 122;
const int PIPE_FAILED = 121;
const int FORK_FAILED = 120;
const int WAITPID_FAILED = 119;

typedef enum {
    FG_COMMAND_COMPLETED = 1,
    FG_COMMAND_GOT_SIGNAL = 2
} SmallShellResult;

class ExecutionFail {
public:
    // constructor
    ExecutionFail() {}
};

class SystemCallFail: public ExecutionFail {
public:
    // constructor
    SystemCallFail() {}
};

class CommandFail: public ExecutionFail {
public:
    // constructor
    CommandFail() {}
};

class SmallShell {
private:
    // constructor
    SmallShell();

public:
    std::string curr_prompt_str;
    std::string prev_wd_path;
    JobList jobs;
    pid_t fg_pid;
    Command* fg_cmd;
    int fg_cmd_prev_jobID;
    pid_t smash_pid;

    // disable copy ctor
    SmallShell(SmallShell const&) = delete;

    // disable = operator
    void operator=(SmallShell const&) = delete;

    // destructor
    ~SmallShell();

    // make SmallShell singleton
    static SmallShell& getInstance();

    Command* createCommand(std::string& cmd_line);

    SmallShellNextState executeCommand(std::string cmd_line);

    void stopFgProccess();

    void killFgProccess();

    // update fg cmd info, run it and wait for it
    SmallShellResult updateFgCommand(Command* new_fg_cmd, pid_t new_fg_pid,
                        int original_jobID);

    void resetFgCommandInfo();

    void updateFgCommandInfo(Command* new_cmd, pid_t new_pid,
            int prev_jobID = FG_COMMAND_WASNT_IN_JOBLIST_BEFORE);
};

#endif //HW1_SMALLSHELL_H
