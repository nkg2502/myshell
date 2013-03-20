#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>

#define MAX_BUFFER	1024
#define MAX_ARGS	64

#define SEPARATORS	" \t\n"

// for debug macro

// for cmd array
enum {CMD_STDIN, CMD_STDOUT, CMD_RUN};

// for flags
enum {OFF, ON};

// for internal cmd
enum {
	i_cd,
	i_dir,
	i_environ, 
	i_clr, 
	i_echo, 
	i_help, 
	i_pause, 
	i_quit,
	i_myshell,
	i_not
};

// for exit status
enum {
	STDIN_FAIL = 1,
	STDOUT_FAIL,
	EXEC_FAIL = 127
};

// function prototype
char** line_parser(char *line, int *file_append, int *background);
int fork_exec(char *cmd[], int file_append, int background);
int internal_cmd(char *name);

// function and macro for debug 
#define pdebug(X)	fprintf(stderr, "%s : %d : %s \n", __func__, __LINE__, X)
void pstatus(char *cmd[], FILE *fp, int file_append, int background);
