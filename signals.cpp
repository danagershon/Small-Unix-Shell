#include <iostream>
#include <signal.h>

#include "signals.h"
#include "SmallShell.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	std::cout << "smash: got ctrl-Z" << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.stopFgProccess();
}

void ctrlCHandler(int sig_num) {
    std::cout << "smash: got ctrl-C"<< std::endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.killFgProccess();
}

