#include "Command.h"

Command::Command(std::string cmd_line)
        : execute_without_fork(false), cmd_line(cmd_line), original_cmd_line(cmd_line)
{}

std::string Command::getCmdLine() {
    return original_cmd_line;
}