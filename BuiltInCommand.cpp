#include "BuiltInCommand.h"

BuiltInCommand::BuiltInCommand(std::string cmd_line) : Command(cmd_line) {}

//-----------------------------------------------------------------------------

// constructors of inheriting classes

ChangePromptCommand::ChangePromptCommand(std::string cmd_line)
        : BuiltInCommand(cmd_line) {}

ShowPidCommand::ShowPidCommand(std::string cmd_line)
        : BuiltInCommand(cmd_line) {}

GetCurrDirCommand::GetCurrDirCommand(std::string cmd_line)
        : BuiltInCommand(cmd_line) {}

ChangeDirCommand::ChangeDirCommand(std::string cmd_line)
    : BuiltInCommand(cmd_line) {}

JobsCommand::JobsCommand(std::string cmd_line)
        : BuiltInCommand(cmd_line) {}

KillCommand::KillCommand(std::string cmd_line)
        : BuiltInCommand(cmd_line), sig_num(0), jobID(0) {}

ForegroundCommand::ForegroundCommand(std::string cmd_line)
    : BuiltInCommand(cmd_line), jobID(-1), noJobIDFromUser(false) {}

BackgroundCommand::BackgroundCommand(std::string cmd_line)
    : BuiltInCommand(cmd_line), jobID(-1), noJobIDFromUser(false) {}

QuitCommand::QuitCommand(std::string cmd_line)
        : BuiltInCommand(cmd_line) {}

//-----------------------------------------------------------------------------

// implementations of execute method

std::string ChangePromptCommand::getNewPrompt() {
    _removeBackgroundSign(cmd_line);
    auto cmd_args = _parseCommandLine(cmd_line);
    std::string new_prompt;

    if (cmd_args.size() == 1) {
        new_prompt = "smash> ";
    } else {
        new_prompt = cmd_args[1] + "> ";
    }

    return new_prompt;
}

SmallShellNextState ChangePromptCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.curr_prompt_str = getNewPrompt();
    return CONTINUE_RUNNING;
}

SmallShellNextState ShowPidCommand::execute() {
    pid_t pid = SmallShell::getInstance().smash_pid;
    std::cout << "smash pid is " << pid << std::endl;
    return CONTINUE_RUNNING;
}

SmallShellNextState GetCurrDirCommand::execute() {
    char cwd_path[PATH_MAX];
    if(getcwd(cwd_path, PATH_MAX) == NULL) {
        perror("smash error: getcwd failed");
        throw SystemCallFail();
    }

    std::cout << std::string(cwd_path) << std::endl;

    return CONTINUE_RUNNING;
}

bool ChangeDirCommand::isValidCommand() {
    _removeBackgroundSign(cmd_line);
    auto args = _parseCommandLine(cmd_line);

    if (args.size() > 2) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        return false;
    }
    if (args.size() == 1) {
        // command is only "cd"
        return false;
    }

    return true;
}

std::string ChangeDirCommand::calculateNewPath() {
    _removeBackgroundSign(cmd_line);
    auto args = _parseCommandLine(cmd_line);

    std::string& new_path = args[1];

    if (new_path != "-") {
        return new_path;
    }

    // here the command is "cd -" which means go to the previous directory
    SmallShell& smash = SmallShell::getInstance();

    if (smash.prev_wd_path.empty()) {
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
        throw CommandFail();
    } else {
        return smash.prev_wd_path;
    }
}

SmallShellNextState ChangeDirCommand::execute() {
    if (!isValidCommand()) {
        return CONTINUE_RUNNING;
    }

    std::string new_path = calculateNewPath();

    if (new_path.empty()) {
        return CONTINUE_RUNNING;
    }

    char cwd_path[PATH_MAX];
    if(getcwd(cwd_path, PATH_MAX) == NULL) {
        perror("smash error: getcwd failed");
        throw SystemCallFail();
    }

    if (chdir(new_path.c_str()) == -1) {
        perror("smash error: chdir failed");
        throw SystemCallFail();
    }

    SmallShell& smash = SmallShell::getInstance();
    smash.prev_wd_path = std::string(cwd_path);

    return CONTINUE_RUNNING;
}

SmallShellNextState JobsCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs.printJobsList();
    return CONTINUE_RUNNING;
}

bool KillCommand::areArgsValid() {
    _removeBackgroundSign(cmd_line);
    auto args = _parseCommandLine(cmd_line);

    // valid cmd format is kill -signum jobID bla
    if (args.size() != 3 || args[1][0] != '-' || args[1].size() == 1) {
        return false;
    }

    // get the signal number without the character '-' at the beginning
    auto signal_str = args[1].substr(1);
    auto& jobID_str = args[2];

    // convert the strings into numbers
    try {
        sig_num = std::stoi(signal_str, nullptr);
        jobID = std::stoi(jobID_str, nullptr);
    } catch (const std::invalid_argument& ia) {
        return false;
    }

    return true;
}

SmallShellNextState KillCommand::execute() {
    if (!areArgsValid()) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return CONTINUE_RUNNING;
    }

    SmallShell& smash = SmallShell::getInstance();

    // here jobID might be negative
    // here sig_num may be invalid and then kill call will fail
    if (smash.jobs.sendSignalToJob(jobID, sig_num) == JOB_DOESNT_EXIST){
        std::cerr << "smash error: kill: job-id " << jobID
                  << " does not exist" << std::endl;
        return CONTINUE_RUNNING;
    }

    // if we get here it means that the job exists
    // if the job should be removed from the job list, it will be done before
    // executing the next command
    std::cout << "signal number " << sig_num << " was sent to pid "
              << smash.jobs.getJobById(jobID)->job_pid << std::endl;

    return CONTINUE_RUNNING;
}

bool ForegroundCommand::areArgsValid() {
    _removeBackgroundSign(cmd_line);
    auto args = _parseCommandLine(cmd_line);

    // valid cmd is "fg jobID" or "fg"
    if (args.size() > 2) {
        return false;
    }
    if (args.size() == 1) {
        // cmd is only "fg"
        noJobIDFromUser = true;
        return true;
    }

    // here args.size() >= 2
    auto& jobID_str = args[1];
    if ( ( jobID_str[0] == '-' && isStringOnlyDigits(jobID_str.substr(1))
        && !jobID_str.substr(1).empty() ) || isStringOnlyDigits(jobID_str)) {
        // jobID is negative or 0 or positive
        try {
            jobID = std::stoi(jobID_str, nullptr);
        } catch (const std::invalid_argument& ia) {
            return false;
        }
        return true;
    } else {
        // supplied jobID is not a number
        return false;
    }
}

JobList::JobEntry* ForegroundCommand::getJobToBringToFg() {
    JobList::JobEntry* job = nullptr;
    SmallShell& smash = SmallShell::getInstance();

    if (noJobIDFromUser) {
        // command is only "fg"
        job = smash.jobs.getLastJob();
        if (job == nullptr) {
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return nullptr;
        }
        jobID = job->jobID;
    } else {
        // command is "fg <jobID>"
        job = smash.jobs.getJobById(jobID);
        if (job == nullptr) {
            std::cerr << "smash error: fg: job-id " << jobID
                      << " does not exist" << std::endl;
            return nullptr;
        }
    }

    return job;
}

SmallShellNextState ForegroundCommand::execute() {
    if (!areArgsValid()) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return CONTINUE_RUNNING;
    }

    JobList::JobEntry* job = getJobToBringToFg();
    if (job == nullptr) {
        return CONTINUE_RUNNING;
    }

    // printing job details
    std::cout << job->getJobCmdLine() << " : " << job->job_pid << std::endl;

    // run job in fg
    SmallShell& smash = SmallShell::getInstance();
    if (smash.updateFgCommand(job->cmd, job->job_pid, job->jobID) == FG_COMMAND_COMPLETED) {
        smash.jobs.removeJobById(jobID);
        return CONTINUE_RUNNING;
    }

    return CONTINUE_RUNNING;
}

bool BackgroundCommand::areArgsValid() {
    _removeBackgroundSign(cmd_line);
    auto args = _parseCommandLine(cmd_line);

    if (args.size() > 2) {
        return false;
    }
    if (args.size() == 1) {
        noJobIDFromUser = true;
        return true;
    }

    // here args.size() >= 2
    auto& jobID_str = args[1];
    if ( ( jobID_str[0] == '-' && isStringOnlyDigits(jobID_str.substr(1))
           && !jobID_str.substr(1).empty() ) || isStringOnlyDigits(jobID_str)) {
        // jobID is negative or 0 or positive
        try {
            jobID = std::stoi(jobID_str, nullptr);
        } catch (const std::invalid_argument& ia) {
            return false;
        }
        return true;
    } else {
        return false;
    }
}

JobList::JobEntry *BackgroundCommand::getStoppedJobToBringToBg() {
    JobList::JobEntry* job = nullptr;
    SmallShell& smash = SmallShell::getInstance();

    if (noJobIDFromUser) {
        // command is only "bg"
        job = smash.jobs.getLastStoppedJob();
        if (job == nullptr) {
            std::cerr << "smash error: bg: there is no stopped jobs to resume"
                      << std::endl;
            return nullptr;
        }
        jobID = job->jobID;
    } else {
        // command is "bg <jobID>"
        job = smash.jobs.getJobById(jobID);
        if (job == nullptr) {
            std::cerr << "smash error: bg: job-id " << jobID
                      << " does not exist" << std::endl;
            return nullptr;
        }
        if (job->job_state == BG) {
            std::cerr << "smash error: bg: job-id " << jobID
                      << " is already running in the background" << std::endl;
            return nullptr;
        }
    }

    return job;
}

SmallShellNextState BackgroundCommand::execute() {
    if (!areArgsValid()) {
        std:: cerr << "smash error: bg: invalid arguments" << std::endl;
        return CONTINUE_RUNNING;
    }

    JobList::JobEntry* job = getStoppedJobToBringToBg();
    if (job == nullptr) {
        return CONTINUE_RUNNING;
    }

    // printing job details
    std::cout << job->getJobCmdLine() << " : " << job->job_pid << std::endl;

    SmallShell& smash = SmallShell::getInstance();
    smash.jobs.updateJobState(job, BG);
    smash.jobs.sendSignalToJob(job->jobID, SIGCONT);

    return CONTINUE_RUNNING;
}

SmallShellNextState QuitCommand::execute() {
    _removeBackgroundSign(cmd_line);
    auto args = _parseCommandLine(cmd_line);

    SmallShell& smash = SmallShell::getInstance();

    bool quit_and_kill = false;

    for (auto& arg : args) {
        if (arg == "kill") {
            quit_and_kill = true;
            break;
        }
    }

    if (quit_and_kill) {
        try {
            smash.jobs.killAllJobs();
        } catch (SystemCallFail& system_call_fail) {
            return QUIT;
        }
    }

    smash.jobs.freeJobsCmdObj();

    return QUIT;
}


