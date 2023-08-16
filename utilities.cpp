#include "utilities.h"
#include "SmallShell.h"

std::string _ltrim(const std::string& s) {
    std::string WHITESPACE = " \n\r\t\f\v";

    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string _rtrim(const std::string& s) {
    std::string WHITESPACE = " \n\r\t\f\v";

    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end+1);
}

std::string _trim(const std::string& s) {
    return _rtrim(_ltrim(s));
}

std::vector<std::string> _parseCommandLine(std::string cmd_line) {
    std::vector<std::string> args;

    std::istringstream iss(_trim(cmd_line));
    for(std::string s; iss >> s; ) {
        args.push_back(s);
    }

    return args;
}

bool _isBackgroundCommand(std::string& cmd_line) {
    std::string WHITESPACE = " \n\r\t\f\v";

    return cmd_line[cmd_line.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(std::string& cmd_line) {
    std::string WHITESPACE = " \n\r\t\f\v";

    // find last character other than spaces
    unsigned long idx = cmd_line.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == std::string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }

    // erases the portion of cmd_line that begins at position idx
    cmd_line.erase(idx);
}

bool isRedirectionCommand(std::string& cmd_line){
    return cmd_line.find('>') != std::string::npos;
}

bool isPipeCommand(std::string& cmd_line){
    return cmd_line.find('|') != std::string::npos;
}

bool isBuiltInCommand(std::string& cmd_line) {
    auto cmd_args = _parseCommandLine(cmd_line);

    if (cmd_args.empty()) {
        // empty command
        return true;
    }

    std::string& command_name = cmd_args[0];

    if (command_name == "chprompt") {
        return true;
    }
    else if (command_name == "showpid"){
        return true;
    }
    else if (command_name == "pwd"){
        return true;
    }
    else if (command_name == "cd"){
        return true;
    }
    else if (command_name == "jobs"){
        return true;
    }
    else if (command_name == "kill"){
        return true;
    }
    else if (command_name == "fg"){
        return true;
    }
    else if (command_name == "bg"){
        return true;
    }
    else if (command_name == "quit"){
        return true;
    }
    else if (isPipeCommand(cmd_line)){
        return false;
    }
    else if (isRedirectionCommand(cmd_line)) {
        return false;
    }
    else if (command_name == "cp") {
        return true;
    }
    else {
        // external command
        return false;
    }
}

bool isStringOnlyDigits(std::string str){
    return str.find_first_not_of("0123456789") == std::string::npos;
}

void checkChildExitStatus(int status) {
    if (!WIFEXITED(status)) {
        return;;
    }

    int child_exit_value = WEXITSTATUS(status);
    if (child_exit_value >= WAITPID_FAILED && child_exit_value <= COMMAND_NOT_RUNNABLE) {
        throw SystemCallFail();
    }
}

void checkGrandChildExitStatus(int status) {
    // checking if grandchild failed and exited. If so, the first son should
    // exit too so the original father will throw the exception
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == SETPGRP_FAILED) {
            _exit(SETPGRP_FAILED);
        }
        if (WEXITSTATUS(status) == OPEN_FAILED) {
            _exit(OPEN_FAILED);
        }
        if (WEXITSTATUS(status) == DUP2_FAILED) {
            _exit(DUP2_FAILED);
        }
        if (WEXITSTATUS(status) == CLOSE_FAILED) {
            _exit(CLOSE_FAILED);
        }
        if (WEXITSTATUS(status) == COMMAND_NOT_RUNNABLE) {
            _exit(COMMAND_NOT_RUNNABLE);
        }
        if (WEXITSTATUS(status) == WRITE_FAILED) {
            _exit(WRITE_FAILED);
        }
        if (WEXITSTATUS(status) == PIPE_FAILED) {
            _exit(PIPE_FAILED);
        }
        if (WEXITSTATUS(status) == FORK_FAILED) {
            _exit(FORK_FAILED);
        }
    }
}

void changeGroupID(){
    if (setpgrp() == -1) {
        perror("smash error: setpgrp failed");
        _exit(SETPGRP_FAILED);
    }
}



