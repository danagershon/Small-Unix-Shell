#ifndef HW1_UTILITIES_H
#define HW1_UTILITIES_H

#include <unistd.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <vector>

/* trimming the characters " ", "\n", ect. from the left side of s and
 * returning a new string */
std::string _ltrim(const std::string& s);

/* trimming the characters " ", "\n", ect. from the right side of s and
 * returning a new string */
std::string _rtrim(const std::string& s);

/* trimming the characters " ", "\n", ect. from both sides of s and
 * returning a new string */
std::string _trim(const std::string& s);

/* parsing the string into tokens (words) no matter how mamy spaces between
 * them. Every string in the returned vector is a word */
std::vector<std::string> _parseCommandLine(std::string cmd_line);

/* checking if the string ends with a '&' */
bool _isBackgroundCommand(std::string& cmd_line);

/* removing the '&' from the end of the string, if exist */
void _removeBackgroundSign(std::string& cmd_line);

/* searching for ">" or ">>" in the string (doesn't have to be a separate
 * word) */
bool isRedirectionCommand(std::string& cmd_line);

/* searching for "|" or "|&" in the string (doesn't have to be a separate
 * word) */
bool isPipeCommand(std::string& cmd_line);

/* determining if the string if a built-in command by inspecting the first
 * word and searching for redirection/pipe characters */
bool isBuiltInCommand(std::string& cmd_line);

/* determining if the string characters are only digits 0-9 */
bool isStringOnlyDigits(std::string str);

/* determining the cause of child proccess termination by inspecting it's
 * _exit value, printing a matching error with perror and throwing an
 * exception */
void checkChildExitStatus(int status);

/* determining (in the first child proccess) the cause of grandchild proccess
 * termination by inspecting it's _exit value, and exiting also with the same
 * value so that the original father will call checkChildExitStatus */
void checkGrandChildExitStatus(int status);

void changeGroupID();

#endif //HW1_UTILITIES_H

