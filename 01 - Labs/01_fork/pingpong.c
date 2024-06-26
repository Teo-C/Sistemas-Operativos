#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

void proceso_hijo(int fd1[2], int fd2[2], int i_fork);
void proceso_padre(int fd1[2], int fd2[2], int i_fork);
void close_fds(int fd1, int fd2);
void imprimir_info_proceso(int i_fork);
void read_pipe(int fd, long int *rand_num_recv);
void write_pipe(int fd, long int rand_num_env);

int
main(int argc, char *argv[])
{
	if (argc != 1) {
		printf("Error: el programa no recibe argumentos\n");
		return -1;
	}
	int fd1[2];
	int fd2[2];
	int pipe1 = pipe(fd1);
	int pipe2 = pipe(fd2);

	if (pipe1 < 0 || pipe2 < 0) {
		perror("Error en la creacion de pipes\n");
		return 1;
	}

	printf("Hola, soy PID <%d>:\n", getpid());
	printf("  - primer pipe me devuelve: [%d, %d]\n", fd1[0], fd1[1]);
	printf("  - segundo pipe me devuelve: [%d, %d]\n", fd2[0], fd2[1]);

	int i = fork();

	if (i < 0) {
		perror("Error en la creacion del segundo proceso (fork)");
		return 1;
	}

	if (i == 0) {
		proceso_hijo(fd1, fd2, i);
	} else {
		proceso_padre(fd1, fd2, i);
	}
	return 0;
}

void
proceso_hijo(int fd1[2], int fd2[2], int i_fork)
{
	close_fds(fd1[1], fd2[0]);
	imprimir_info_proceso(i_fork);
	
	long int rand_num_recv;
	read_pipe(fd1[0], &rand_num_recv);
	
	close(fd1[0]);
	printf("  - recibo valor <%li> vía fd=%d\n", rand_num_recv, fd1[0]);
	printf("  - reenvío valor en fd=%d y termino\n", fd2[1]);
	
	write_pipe(fd2[1], rand_num_recv);

	close(fd2[1]);
}

void
proceso_padre(int fd1[2], int fd2[2], int i_fork)
{
	close_fds(fd1[0], fd2[1]);
	imprimir_info_proceso(i_fork);
	
	srandom(time(NULL));
	long int rand_num_env = random();
	printf("  - random me devuelve: <%li>\n", rand_num_env);
	printf("  - envío valor <%li> a través de fd=%d\n",
	       rand_num_env,
	       fd1[1]);
	write_pipe(fd1[1], rand_num_env);
	
	close(fd1[1]);
	long int rand_num_recv;
	read_pipe(fd2[0], &rand_num_recv);	

	printf("Hola, de nuevo PID <%d>:\n", getpid());
	printf("  - recibí valor <%li> vía fd=%d\n", rand_num_recv, fd2[0]);
	close(fd2[0]);
	wait(NULL);
}

void close_fds(int fd1, int fd2)
{
	close(fd1);
	close(fd2);
}

void imprimir_info_proceso(int i_fork)
{
	printf("Donde fork me devuelve <%d>:\n", i_fork);
	printf("  - getpid me devuelve: <%d>\n", getpid());
	printf("  - getppid me devuelve: <%d>\n", getppid());
}

void read_pipe(int fd, long int *rand_num_recv)
{
	if (read(fd, rand_num_recv, sizeof(*rand_num_recv)) < 0) {
		perror("Error leyendo del pipe");
		exit(-1);
	}
}

void write_pipe(int fd, long int rand_num_env)
{
	if (write(fd, &rand_num_env, sizeof(rand_num_env)) < 0) {
		perror("Error escribiendo en pipe");
		exit(-1);
	}
}