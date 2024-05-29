#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

void busqueda_str_recursiva(DIR *directory,
                            char *str,
                            bool distinguir_mayus,
                            char name_path[]);
bool coincide(char *entry_name, char *str, bool distinguir_mayus);

int
main(int argc, char *argv[])
{
	bool distinguir_mayus;
	char *str;
	if (argc == 2) {
		distinguir_mayus = true;
		str = argv[1];
	} else if (argc == 3) {
		if (strcmp(argv[1], "-i") != 0) {
			printf("Error en el uso de la opcion [-i]\n");
			return -1;
		}
		distinguir_mayus = false;
		str = argv[2];
	} else {
		printf("Error en la cantidad de parametros\n");
		return -1;
	}

	DIR *directory = opendir(".");
	if (directory == NULL) {
		perror("Error en la apertura del directorio actual");
		return -1;
	}
	char name_dir[PATH_MAX];
	name_dir[0] = '\0';

	busqueda_str_recursiva(directory, str, distinguir_mayus, name_dir);
	// free(name_dir);
	closedir(directory);

	return 0;
}


void
busqueda_str_recursiva(DIR *directory,
                       char *str,
                       bool distinguir_mayus,
                       char name_path[])
{
	/*Funcion recursiva que se encarga de realizar la busqueda del str*/
	struct dirent *entry = readdir(directory);
	while (entry != NULL) {
		if (entry->d_type == DT_REG) {
			if (coincide(entry->d_name, str, distinguir_mayus)) {
				char path_aux[PATH_MAX];
				path_aux[0] = '\0';
				strcat(path_aux, name_path);
				printf("%s\n", strcat(path_aux, entry->d_name));
			}
		} else if (entry->d_type == DT_DIR) {
			if (strcmp(entry->d_name, ".") != 0 &&
			    strcmp(entry->d_name, "..") != 0) {
				int directory_fd = dirfd(directory);
				int son_fd = openat(directory_fd,
				                    entry->d_name,
				                    O_DIRECTORY);
				DIR *directory_son = fdopendir(son_fd);

				char path_nuevo[PATH_MAX];
				path_nuevo[0] = '\0';
				strcat(path_nuevo, name_path);
				strcat(path_nuevo, entry->d_name);
				if (coincide(entry->d_name, str, distinguir_mayus)) {
					printf("%s\n", path_nuevo);
				}
				strcat(path_nuevo, "/");
				busqueda_str_recursiva(directory_son,
				                       str,
				                       distinguir_mayus,
				                       path_nuevo);
				closedir(directory_son);
			}
		}
		entry = readdir(directory);
	}
}
// C:\Users\teo-c\OneDrive\Escritorio\SisOp\sisop_2022b_correa\fork

bool
coincide(char *entry_name, char *str, bool distinguir_mayus)
{
	if (distinguir_mayus) {
		return (strstr(entry_name, str) != NULL);
	}
	return (strcasestr(entry_name, str) != NULL);
}