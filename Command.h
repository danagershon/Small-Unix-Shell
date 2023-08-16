#ifndef HW1_COMMAND_H
#define HW1_COMMAND_H

#include <string>
#include <vector>
#include <unistd.h>

// or #include <linux/limits.h>
#include <climits>
#include "utilities.h"
#include <errno.h>

const int bash_path_length = 10;
const int bash_flag_length = 3;

typedef enum {
    QUIT = 0,
    CONTINUE_RUNNING = 1
} SmallShellNextState;

class Command {
public:
    bool execute_without_fork;

protected:
    std::string cmd_line;
    std::string original_cmd_line;

public:
    // constructor
    explicit Command(std::string cmd_line);

    // destructor
    virtual ~Command() = default;

    virtual SmallShellNextState execute() = 0;

    std::string getCmdLine();
};


#endif //HW1_COMMAND_H
