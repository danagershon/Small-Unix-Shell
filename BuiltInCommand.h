#ifndef HW1_BUILTINCOMMAND_H
#define HW1_BUILTINCOMMAND_H

#include "Command.h"
#include "JobList.h"
#include "SmallShell.h"

class BuiltInCommand: public Command {
public:
    // constructor
    explicit BuiltInCommand(std::string cmd_line);
};

//-----------------------------------------------------------------------------

// inheriting classes

class ChangePromptCommand : public BuiltInCommand {
    std::string getNewPrompt();

public:
    // constructor
    explicit ChangePromptCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    // constructor
    explicit ShowPidCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    // constructor
    explicit GetCurrDirCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    std::string calculateNewPath();
    bool isValidCommand();

public:
    // constructor
    explicit ChangeDirCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

//-----------------------------------------------------------------------------

// inheriting classes that need the jobs list

class JobsCommand : public BuiltInCommand {
public:
    // constructor
    explicit JobsCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class KillCommand : public BuiltInCommand {
    int sig_num;
    int jobID;

    bool areArgsValid();

public:
    // constructor
    explicit KillCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    int jobID;
    bool noJobIDFromUser;

    bool areArgsValid();
    JobList::JobEntry* getJobToBringToFg();

public:
    // constructor
    explicit ForegroundCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    int jobID;
    bool noJobIDFromUser;

    bool areArgsValid();
    JobList::JobEntry* getStoppedJobToBringToBg();

public:
    // constructor
    explicit BackgroundCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class QuitCommand : public BuiltInCommand {
public:
    // constructor
    explicit QuitCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

#endif //HW1_BUILTINCOMMAND_H
