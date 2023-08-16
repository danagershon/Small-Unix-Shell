SUBMITTERS := 209804574_322381716
# c++ compiler is g++ (not gcc)
COMPILER := g++
# -Wall will check for errors and for all kinds of warnings
COMPILER_FLAGS := --std=c++11 -Werror -Wall
# all source files
SRCS := Command.cpp signals.cpp smash.cpp utilities.cpp SpecialCommand.cpp SmallShell.cpp JobList.cpp ExternalCommand.cpp BuiltInCommand.cpp
# executable file name
SMASH_BIN := smash

$(SMASH_BIN):
	$(COMPILER) $(COMPILER_FLAGS) $(SRCS) -o $@

clean:
	rm -rf $(SMASH_BIN)