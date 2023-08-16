#ifndef HW1_SPECIALCOMMAND_H
#define HW1_SPECIALCOMMAND_H

#include "Command.h"
#include "SmallShell.h"

const int buff_size = 4096;

class SpecialCommand : public Command {
protected:
    bool isBgCommand;
    char bash_path[bash_path_length];
    char bash_flag[bash_flag_length];
    void handleChildProccess(pid_t child_pid);

    virtual void prepare() = 0;
public:
    // constructor
    explicit SpecialCommand(std::string cmd_line);
};

class RedirectionCommand : public SpecialCommand {
    std::string file_name;
    std::string cmd_line_to_run;
    int flags;
    int fd_output_file;

    std::string getOutputFileName();
    std::string getCmdLineToRun();
    int getOpeningFileFlags();
    SmallShellNextState doRedirectionInSmash();
    SmallShellNextState doRedirectionOfExternalCmd();

    void prepare() override;
public:
    // constructor
    explicit RedirectionCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class PipeCommand : public SpecialCommand {
    std::string left_cmd_line;
    std::string right_cmd_line;
    int output_channel;
    int pipe_fd[2];
    pid_t writing_proccess_pid;
    pid_t reading_proccess_pid;

    std::string getPipeLeftCommand();
    std::string getPipeRightCommand();
    void runPipeFromSon();

    void prepare() override;
public:
    // constructor
    explicit PipeCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};

class CopyCommand : public SpecialCommand {
    std::string src_file_path;
    std::string dest_file_path;
    int src_fd;
    int dest_fd;

    std::string getSourceFilePath();
    std::string getDestFilePath();
    bool checkIfSameFile();
    void printCopyingMsg();
    bool isCopyingNeeded();
    void openSrcDestFiles();
    void copySrcToDest();
    bool doesFileExist(std::string& file_name);

    void prepare() override;
public:
    explicit CopyCommand(std::string cmd_line);

    SmallShellNextState execute() override;
};


#endif //HW1_SPECIALCOMMAND_H
