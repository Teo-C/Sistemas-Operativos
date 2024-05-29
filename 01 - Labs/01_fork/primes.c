#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void lanzar_filtros(uint32_t n);
void nuevo_filtro(int fd_izquierda[2]);

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Error en la cantidad de parametros\n");
		return -1;
	}
	uint32_t valor_n = atoi(argv[1]);
	lanzar_filtros(valor_n);

	return 0;
}

void
lanzar_filtros(uint32_t n)
{
	int fd_derecha[2];
	int l = pipe(fd_derecha);
	if (l < 0) {
		perror("Error en la creacion del pipe");
		return;
	}

	int m = fork();
	if (m < 0) {
		perror("Error en la creacion del pipe");
		return;
	}

	if (m == 0) {
		nuevo_filtro(fd_derecha);
	} else {
		close(fd_derecha[0]);
		printf("primo 2\n");
		for (uint32_t i = 3; i <= n; i++) {
			if (i % 2 != 0) {
				int j = write(fd_derecha[1], &i, sizeof(i));
				if (j < 0) {
					perror("Error escribiendo en pipe");
					return;
				}
			}
		}
		close(fd_derecha[1]);
		wait(NULL);
	}
}

void
nuevo_filtro(int fd_izquierda[2])
{
	close(fd_izquierda[1]);

	uint32_t primer_num;
	int j = read(fd_izquierda[0], &primer_num, sizeof(primer_num));
	if (j < 0) {
		perror("Error leyendo del pipe");
		return;
	}
	if (j == 0) {
		close(fd_izquierda[0]);
		return;
	}

	printf("primo %i\n", primer_num);

	int fd_derecha[2];
	int l = pipe(fd_derecha);
	if (l < 0) {
		perror("Error en la creacion del pipe");
		return;
	}

	int aux = fork();
	if (aux == 0) {
		close(fd_izquierda[0]);
		close(fd_derecha[1]);
		nuevo_filtro(fd_derecha);
	} else {
		close(fd_derecha[0]);
		int r;
		uint32_t j;
		while (r = read(fd_izquierda[0], &j, sizeof(j))) {
			if (r < 0) {
				return;
			}
			if (j % primer_num != 0) {
				int a = write(fd_derecha[1], &j, sizeof(j));
				if (a < 0) {
					perror("Error escribiendo en pipe");
					return;
				}
			}
		}
		close(fd_izquierda[0]);
		close(fd_derecha[1]);
		wait(NULL);
	}
}