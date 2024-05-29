#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		char environ_key[BUFLEN];
		char environ_value[BUFLEN];

		get_environ_key(eargv[i], environ_key);
		int indx_pos = block_contains(eargv[i], '=');
		get_environ_value(eargv[i], environ_value, indx_pos);

		int j = setenv(environ_key, environ_value, 1);
		if (j < 0) {
			printf_debug("Error en el seteado de environ-var\n");
			_exit(-1);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd = open(file, flags, S_IWUSR | S_IRUSR);
	if (fd < 0) {
		printf_debug("Error al abrir el archivo\n");
		_exit(-1);
	}

	return fd;
}

void
redir_fd(int oldfd, int newfd)
{
	int i = dup2(oldfd, newfd);
	if (i < 0) {
		printf_debug("Error al hacer dup\n");
		close(oldfd);
		_exit(-1);
	}
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		int i = execvp(e->argv[0], e->argv);
		if (i < 0) {
			printf_debug("Error lanzando el programa\n");
			_exit(-1);
		}
		break;

	case BACK: {
		// runs a command in background
		//
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		r = (struct execcmd *) cmd;

		if (strlen(r->in_file) > 0) {
			int fd_in = open_redir_fd(r->in_file, O_CLOEXEC);
			redir_fd(fd_in, 0);
		}
		if (strlen(r->out_file) > 0) {
			int fd_out = open_redir_fd(r->out_file,
			                           O_CLOEXEC | O_CREAT |
			                                   O_TRUNC | O_RDWR);
			redir_fd(fd_out, 1);
		}
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				redir_fd(1, 2);
			} else {
				int fd_err =
				        open_redir_fd(r->err_file,
				                      O_CLOEXEC | O_CREAT |
				                              O_TRUNC | O_RDWR);
				redir_fd(fd_err, 2);
			}
		}
		// Una vez que realicé los cambios en los fd correspondientes puedo
		// tratar al comando como un simple exec y llamar a exec_cmd
		cmd->type = EXEC;
		exec_cmd(cmd);
		break;
	}

	case PIPE: {
		// pipes two commands
		//
		p = (struct pipecmd *) cmd;

		int fd_com[2];
		if (pipe(fd_com) < 0) {
			printf_debug("Error en la creacion de pipe\n");
			_exit(-1);
		}
		int pid_left_son = fork();
		if (pid_left_son < 0) {
			printf_debug(
			        "Error en la creacion del hijo izquierdo\n");
			close(fd_com[READ]);
			close(fd_com[WRITE]);
			_exit(-1);
		}

		if (pid_left_son == 0) {
			close(fd_com[READ]);
			dup2(fd_com[WRITE], 1);
			close(fd_com[WRITE]);
			exec_cmd(p->leftcmd);
		}

		int pid_right_son = fork();
		if (pid_right_son < 0) {
			printf_debug("Error en la creacion del hijo derecho\n");
			close(fd_com[READ]);
			close(fd_com[WRITE]);
			_exit(-1);
		}

		if (pid_right_son == 0) {
			close(fd_com[WRITE]);
			dup2(fd_com[READ], 0);
			close(fd_com[READ]);
			exec_cmd(p->rightcmd);
		}

		close(fd_com[READ]);
		close(fd_com[WRITE]);
		waitpid(pid_left_son, NULL, 0);
		waitpid(pid_right_son, NULL, 0);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		_exit(0);
		break;
	}
	}
}
