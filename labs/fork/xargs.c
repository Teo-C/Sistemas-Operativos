#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#ifndef NARGS
#define NARGS 4
#endif

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Error en la cantidad de parametros\n");
		return -1;
	}

	bool continuar_leyendo = true;
	while (continuar_leyendo) {
		char *lineas[NARGS + 2];

		for (int i = 0; i < NARGS + 2; i++) {
			lineas[i] = NULL;
		}
		lineas[0] = argv[1];

		int aux = 0;
		int lineas_leidas = 0;
		while (lineas_leidas < NARGS && aux != -1) {
			char *line = NULL;
			size_t line_len = 0;
			aux = getline(&line, &line_len, stdin);
			if (aux > 0) {
				strtok(line, "\n");
				lineas[lineas_leidas + 1] = line;
				lineas_leidas++;
			} else {
				free(line);
			}
		}
		if (lineas_leidas == 0) {
			continuar_leyendo = false;
			break;
		}
		int i = fork();

		if (i < 0) {
			perror("Error en la creacion del segundo proceso "
			       "(fork)");
			return 1;
		}
		if (i == 0) {
			int j = execvp(argv[1], lineas);
			if (j == -1) {
				perror("Error en el lanzamiento del programa");
				return -1;
			}
			return 0;
		} else {
			wait(NULL);
		}
		if (aux == -1) {
			continuar_leyendo = false;
		}
		for (int i = 1; i < NARGS + 1; i++) {
			if (lineas[i]) {
				free(lineas[i]);
			}
		}
	}
	return 0;
}