#include "JobList.h"

#include <signal.h>

#include "SmallShell.h"

// JobEntry implementation

JobList::JobEntry::JobEntry(int jobID, JobState job_state, Command *cmd,
                            pid_t job_pid)
        : jobID(jobID), job_state(job_state), cmd(cmd), job_pid(job_pid) {
    addition_time = time(NULL);
    if (addition_time == -1) {
        perror("smash error: time failed");
        throw SystemCallFail();
    }
}

std::string JobList::JobEntry::getJobCmdLine() {
    return cmd->getCmdLine();
}

// ----------------------------------------------------------------------------

// JobList implementation

JobList::JobList()
    :  max_jobID(0), max_stopped_jobID(0) {}

void JobList::addJob(Command *cmd, pid_t pid, JobState job_state,
                int originalJobID) {
    if (originalJobID != FG_COMMAND_WASNT_IN_JOBLIST_BEFORE) {
        /* means that job (stopped/bg) was in list, then brought to fg with fg
         * command and then sent to job list again because of ctrl-z that
         * stopped it */
        if (jobs_map.count(originalJobID) == 0) {
            return;
        }
        updateJobState(&jobs_map[originalJobID], STOPPED);
        // restart addition time
        jobs_map[originalJobID].addition_time = time(NULL);
        return;
    }

    // otherwise. it's a new job
    removeFinishedJobs();
    int new_jobID = max_jobID + 1;
    max_jobID++;
    if (job_state == STOPPED) {
        max_stopped_jobID = max_jobID;
    }
    jobs_map[new_jobID] = JobEntry(new_jobID, job_state, cmd, pid);
}

void JobList::printJobDetails(JobEntry& job) {
    int seconds_elapsed = static_cast<int>(difftime(time(NULL),
            job.addition_time));

    std::string jobID = "[" + std::to_string(job.jobID) + "] ";
    std::string details = jobID + job.getJobCmdLine() + " : "
                          + std::to_string(job.job_pid) + " "
                          + std::to_string(seconds_elapsed) + " secs";
    if (job.job_state == STOPPED) {
        details += " (stopped)";
    }

    std::cout << details << std::endl;
}

void JobList::printJobsList() {
    removeFinishedJobs();
    for (auto& job : jobs_map) {
        printJobDetails(job.second);
    }
}

void JobList::killAllJobs() {
    removeFinishedJobs();

    std::cout << "smash: sending SIGKILL signal to " <<  jobs_map.size()
        << " jobs:" << std::endl;

    for (auto& job : jobs_map) {
        int jobID = job.first;
        JobEntry& job_entry = job.second;
        if (sendSignalToJob(jobID, SIGKILL) == JOB_DOESNT_EXIST) {
            continue;
        }
        std::cout << job_entry.job_pid << ": " << job_entry.getJobCmdLine()
                    << std::endl;
    }
}

void JobList::removeFinishedJobs() {
    for (auto& job : jobs_map) {
        int jobID = job.first;
        JobEntry& job_entry = job.second;
        if (waitpid(job_entry.job_pid, NULL, WNOHANG) == job_entry.job_pid) {
            // state of job changed to "terminated"
            removeJobById(jobID);
        }
        if (jobs_map.empty()) {
            break;
        }
    }
}

JobList::JobEntry* JobList::getJobById(int jobId) {
    //removeFinishedJobs();

    if (jobs_map.count(jobId) == 1) {
        return &jobs_map[jobId];
    } else {
        // job doesn't exist
        return nullptr;
    }
}

void JobList::removeJobById(int jobId) {
    if (jobs_map.count(jobId) == 0) {
        return;
    }

    if (jobs_map[jobId].job_state == STOPPED && jobId == max_stopped_jobID) {
        max_stopped_jobID = findNewMaxStoppedJobID();
    }
    if (jobId == max_jobID) {
        max_jobID = findNewMaxJobID();
    }

    jobs_map.erase(jobId);
}

JobList::JobEntry *JobList::getLastJob() {
    removeFinishedJobs();
    if (jobs_map.count(max_jobID) == 1) {
        return &jobs_map[max_jobID];
    }
    return nullptr;
}

JobList::JobEntry *JobList::getLastStoppedJob() {
    removeFinishedJobs();
    if (jobs_map.count(max_stopped_jobID) == 1) {
        return &jobs_map[max_stopped_jobID];
    }
    return nullptr;
}

JobListResult JobList::sendSignalToJob(int jobID, int sig_num) {
    //removeFinishedJobs();
    JobEntry* job = getJobById(jobID);

    if (job == nullptr){
        return JOB_DOESNT_EXIST;
    }

    // here sig_num is promised not to be SIGSTOP,SIGCONT
    if (killpg(job->job_pid, sig_num) == -1) {
        perror("smash error: kill failed");
        throw SystemCallFail();
    }

    // so updating the job list after sending signal is redundant
    /*int status;
    if (waitpid(job->job_pid, &status, WUNTRACED) == job->job_pid){
        if (WIFSTOPPED(status)) {
            updateJobState(job, STOPPED);
        }
        if (WIFSIGNALED(status)) { // job killed by signal
            delete job->cmd; // no need for cmd after job finished
            removeJobById(job->jobID);
        }
    }*/

    return JOB_EXISTS;
}

void JobList::updateJobState(JobList::JobEntry* job, JobState new_state) {
    // removeFinishedJobs();
    if (job->job_state == STOPPED && new_state == BG) {
        job->job_state = BG;
        if (job->jobID == max_stopped_jobID) {
            max_stopped_jobID = findNewMaxStoppedJobID();
        }
    }

    if (job->job_state == BG && new_state == STOPPED) {
        job->job_state = STOPPED;
        if (job->jobID > max_stopped_jobID) {
            max_stopped_jobID = job->jobID;
        }
    }
}

int JobList::findNewMaxStoppedJobID() {

    int new_max_stopped_jobID = 0;

    for (auto& job : jobs_map) {
        int jobID = job.first;
        JobEntry& job_entry = job.second;
        if (job_entry.job_state == BG) {
            continue;
        }
        if (jobID != max_stopped_jobID && jobID > new_max_stopped_jobID) {
            new_max_stopped_jobID = jobID;
        }
    }

    return new_max_stopped_jobID;
}

int JobList::findNewMaxJobID() {
    int new_max_jobID = 0;

    for (auto& job : jobs_map) {
        int jobID = job.first;
        if (jobID != max_jobID && jobID > new_max_jobID) {
            new_max_jobID = jobID;
        }
    }

    return new_max_jobID;
}

void JobList::freeJobsCmdObj() {
    for (auto& job : jobs_map) {
        JobEntry& job_entry = job.second;
        delete job_entry.cmd;
        job_entry.cmd = nullptr;
    }
}
