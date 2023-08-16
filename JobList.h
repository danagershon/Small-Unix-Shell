#ifndef HW1_JOBLIST_H
#define HW1_JOBLIST_H

#include <map>
#include "Command.h"

typedef enum {
    BG = 1,
    STOPPED = 2
} JobState;

typedef enum {
    JOB_DOESNT_EXIST = 1,
    JOB_EXISTS = 2
} JobListResult;

class JobList {
public:
    class JobEntry {
    public:
        int jobID;
        JobState job_state;
        Command* cmd;
        pid_t job_pid;
        time_t addition_time;

        // constructor
        explicit JobEntry(int jobID=0, JobState job_state=STOPPED,
                Command* cmd= nullptr, pid_t job_pid=0);

        std::string getJobCmdLine();
    };

private:
    int max_jobID;
    int max_stopped_jobID;
    std::map<int, JobEntry> jobs_map;

    int findNewMaxStoppedJobID();
    int findNewMaxJobID();

public:
    // constructor
    JobList();

    void addJob(Command* cmd, pid_t pid, JobState job_state,
                int originalJobID = 0);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry* getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry* getLastJob();

    JobEntry* getLastStoppedJob();

    JobListResult sendSignalToJob(int jobID, int sig_num);

    void updateJobState(JobEntry* job, JobState new_state);

    void freeJobsCmdObj();

    // print job details for jobs command
    void printJobDetails(JobEntry& job);
};

#endif //HW1_JOBLIST_H
