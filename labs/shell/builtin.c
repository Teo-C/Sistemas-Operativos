#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return (strcmp(cmd, "exit") == 0);
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2) == 0) {
		char *new_directory;
		if (strcmp(cmd, "cd") == 0) {
			new_directory = getenv("HOME");
		} else {
			new_directory = cmd + 3;
		}
		int i = chdir(new_directory);
		if (i < 0) {
			printf_debug("Error cambiando el directorio a %s",
			             new_directory);
		}
		snprintf(prompt,
		         sizeof prompt,
		         "(%s)",
		         getcwd(new_directory, PRMTLEN));
		return true;
	}
	return false;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char buffer[PRMTLEN];
		snprintf(prompt, sizeof prompt, "(%s)", getcwd(buffer, PRMTLEN));
		return true;
	}
	return false;
}
