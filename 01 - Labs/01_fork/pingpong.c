#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

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
		close(fd1[1]);  // El hijo no va a escribir en el primer pipe
		close(fd2[0]);  // El hijo no va a leer en el segundo pipe
		printf("Donde fork me devuelve 0:\n");
		printf("  - getpid me devuelve: <%d>\n", getpid());
		printf("  - getppid me devuelve: <%d>\n", getppid());
		long int rand_num_recv;
		if (read(fd1[0], &rand_num_recv, sizeof(rand_num_recv)) < 0) {
			perror("Error leyendo del pipe");
			return -1;
		}
		close(fd1[0]);
		printf("  - recibo valor <%li> vía fd=%d\n", rand_num_recv, fd1[0]);
		printf("  - reenvío valor en fd=%d y termino\n", fd2[1]);
		if (write(fd2[1], &rand_num_recv, sizeof(rand_num_recv)) < 0) {
			perror("Error escribiendo en pipe");
			return -1;
		}
		close(fd2[1]);

	} else {
		close(fd1[0]);  // El padre no va a leer en el primer pipe
		close(fd2[1]);  // El padre no va a escribir en el segundo pipe
		printf("Donde fork me devuelve <%d>:\n", i);
		printf("  - getpid me devuelve: <%d>\n", getpid());
		printf("  - getppid me devuelve <%d>\n", getppid());
		srandom(time(NULL));
		long int rand_num_env = random();
		printf("  - random me devuelve: <%li>\n", rand_num_env);
		printf("  - envío valor <%li> a través de fd=%d\n",
		       rand_num_env,
		       fd1[1]);

		if (write(fd1[1], &rand_num_env, sizeof(rand_num_env)) < 0) {
			perror("Error escribiendo en pipe");
			return -1;
		}
		close(fd1[1]);
		long int rand_num_recv;
		if (read(fd2[0], &rand_num_recv, sizeof(rand_num_recv)) < 0) {
			perror("Error leyendo del pipe");
			return -1;
		}
		printf("Hola, de nuevo PID <%d>:\n", getpid());
		printf("  - recibí valor <%li> vía fd=%d\n", rand_num_recv, fd2[0]);
		close(fd2[0]);
		wait(NULL);
	}
	return 0;
}
