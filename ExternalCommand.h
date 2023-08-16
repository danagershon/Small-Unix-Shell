#ifndef HW1_EXTERNALCOMMAND_H
#define HW1_EXTERNALCOMMAND_H

#include "Command.h"
#include "SmallShell.h"

class ExternalCommand: public Command {
    bool isBgCommand;
    char bash_path[bash_path_length];
    char bash_flag[bash_flag_length];

    void handleChildProccess(pid_t child_pid);
public:
    // constructor
    explicit ExternalCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

#endif //HW1_EXTERNALCOMMAND_H
