#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "signals.h"
#include "SmallShell.h"

int main(int argc, char* argv[]) {

    if(signal(SIGTSTP , ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    SmallShell& smash = SmallShell::getInstance();

    SmallShellNextState smash_next_state = CONTINUE_RUNNING;

    while(smash_next_state == CONTINUE_RUNNING) {
        std::cout << smash.curr_prompt_str;
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        try {
            smash_next_state = smash.executeCommand(cmd_line);
        } catch (ExecutionFail& e) {
            smash_next_state = CONTINUE_RUNNING;
        }
    }

    return 0;
}