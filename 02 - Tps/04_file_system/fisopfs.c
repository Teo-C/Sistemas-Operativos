#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <unistd.h>

#define BLOCK_SIZE 100
#define MAX_LENGTH 4096
#define FILE_NAME_LENGTH 35

struct File {
	char path[PATH_MAX];
	mode_t permissions;
	long file_size;
	time_t last_access;
	time_t last_modified;
	time_t change_time;
	bool is_dir;
	char data[BLOCK_SIZE];
	int cant_files;
	struct File *sub_files;
	struct File *next;
};

struct File *filesystem;
char filename[FILE_NAME_LENGTH];

void
serialize(struct File *root, FILE *fp)
{
	for (struct File *curr = root; curr != NULL; curr = curr->next) {
		fprintf(fp, "%s", curr->path);
		fputc('\n', fp);

		fprintf(fp, "%d", curr->permissions);
		fputc('\n', fp);

		fprintf(fp, "%zu", curr->file_size);
		fputc('\n', fp);

		fprintf(fp, "%zu", curr->last_access);
		fputc('\n', fp);

		fprintf(fp, "%zu", curr->last_modified);
		fputc('\n', fp);

		fprintf(fp, "%zu", curr->change_time);
		fputc('\n', fp);

		fprintf(fp, "%d", curr->is_dir);
		fputc('\n', fp);

		if (!curr->is_dir && curr->file_size > 0) {
			fwrite(curr->data, 1, curr->file_size, fp);
		}

		if (curr->is_dir) {
			fprintf(fp, "%d", curr->cant_files);
			fputc('\n', fp);
		}

		if (curr->cant_files > 0 && curr->is_dir) {
			serialize(curr->sub_files, fp);
		}
	}
}

void
deserialize(struct File **root, FILE *fp, int amount)
{
	printf("\n");
	char buffer[MAX_LENGTH];
	memset(buffer, 0, sizeof(buffer));

	struct File *file = malloc(sizeof(struct File));

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		printf("[debug] file->path: %s\n", buffer);
		strcpy(file->path, buffer);
	}

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		printf("[debug] file->permissions: %s\n", buffer);
		file->permissions = atoi(buffer);
	}

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		printf("[debug] file->file_size: %s\n", buffer);
		file->file_size = atol(buffer);
	}

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		printf("[debug] file->last_access: %s\n", buffer);
		file->last_access = atol(buffer);
	}

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		printf("[debug] file->last_modified: %s\n", buffer);
		file->last_modified = atol(buffer);
	}

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		file->change_time = atol(buffer);
	}

	if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		printf("[debug] file->is_dir: %s\n", buffer);
		file->is_dir = atoi(buffer);
	}

	if (!file->is_dir) {
		if (fread(file->data, 1, file->file_size, fp) > 0) {
			printf("[debug] file->data: %s\n", file->data);
			fgets(buffer, MAX_LENGTH, fp);  // muevo el puntero
			file->cant_files = 0;
		}
	}

	if (file->is_dir) {
		if (fgets(buffer, MAX_LENGTH, fp) != NULL) {
			if (strlen(buffer) > 0) {
				buffer[strlen(buffer) - 1] = '\0';
			}
			printf("[debug] file->cant_files: %s\n", buffer);
			file->cant_files = atoi(buffer);
		}
	} else {
		file->cant_files = 0;
	}

	if (file->cant_files > 0 && file->is_dir) {
		printf("[debug] deserializo los subfiles\n");
		file->sub_files = malloc(sizeof(struct File));
		deserialize(&file->sub_files, fp, file->cant_files);
	} else {
		file->sub_files = NULL;
	}

	if (!feof(fp) && amount - 1 > 0) {
		printf("[debug] deserializo el next de (%s)\n", file->path);
		file->next = malloc(sizeof(struct File));
		deserialize(&file->next, fp, amount - 1);
		if (strlen(file->next->path) == 0) {
			free(file->next);
			file->next = NULL;
		}
	} else {
		file->next = NULL;
	}

	*root = file;
}

void
fisopfs_init()
{
	printf("[debug] fisopfs_init\n");
	FILE *fp = fopen(filename, "r");
	if (fp) {
		deserialize(&filesystem, fp, 0);
		fclose(fp);
	} else {
		filesystem = malloc(sizeof(struct File));
		strcpy(filesystem->path, "/");
		filesystem->permissions = __S_IFDIR | 0755;
		filesystem->file_size = 0;
		time_t actual_time = time(NULL);
		filesystem->last_access = actual_time;
		filesystem->last_modified = actual_time;
		filesystem->is_dir = true;
		filesystem->cant_files = 0;
		filesystem->sub_files = NULL;
		filesystem->next = NULL;
	}
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *f)
{
	FILE *fp = fopen(filename, "w");
	if (fp) {
		serialize(filesystem, fp);
		fclose(fp);
	}

	return 0;
}

static void
fisopfs_destroy(void *ptr)
{
	FILE *fp = fopen(filename, "w");
	if (fp) {
		serialize(filesystem, fp);
		fclose(fp);
	}
}

struct File *
find_file(const char *path, struct File *dir)
{
	printf("[debug] find_file(%s)\n", path);
	for (struct File *curr = dir; curr != NULL; curr = curr->next) {
		if (strcmp(curr->path, path) == 0)
			return curr;

		if (curr->is_dir) {
			// busco el directorio adentro del current directory
			struct File *found = find_file(path, curr->sub_files);
			if (found) {
				return found;
			}
		}
	}
	return NULL;
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

	if (strcmp(path, "/") == 0) {
		st->st_uid = 1717;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	} else {
		struct File *file = find_file(path, filesystem);
		if (!file) {
			printf("[debug] no se encontro el file (%s)\n", path);
			return -ENOENT;
		}

		st->st_uid = getuid();
		st->st_gid = getgid();
		st->st_mode = file->permissions;
		st->st_atime = file->last_access;
		st->st_mtime = file->last_modified;
		st->st_ctime = file->change_time;
		st->st_size = file->file_size;
		st->st_nlink = 1;
	}

	return 0;
}


// el parametro dir indica en que directorio empieza a buscar
struct File *
find_dir(const char *path, struct File *dir)
{
	printf("[debug] find_dir(%s)\n", path);
	for (struct File *curr = dir; curr != NULL; curr = curr->next) {
		if (!curr->is_dir) {
			continue;
		}

		if (strcmp(curr->path, path) == 0)
			return curr;

		// busco el directorio adentro del current directory
		struct File *found = find_dir(path, curr->sub_files);
		if (found) {
			return found;
		}
	}

	return NULL;
}

// Devuelve el directorio en el que está contenido un archivo
char *
get_containing_path(const char *path)
{
	char *containing_dir = (char *) malloc(PATH_MAX);
	memset(containing_dir, 0, PATH_MAX);

	// copio todo el path menos el nombre del archivo
	strcpy(containing_dir, path);
	char *ptr = strrchr(containing_dir, '/');
	if (ptr && ptr != &containing_dir[0]) {
		*ptr = '\0';
	} else if (ptr) {
		strcpy(containing_dir, "/");
	}

	return containing_dir;
}

void
insert_file(struct File *dir, struct File *file)
{
	if (dir->cant_files == 0) {
		printf("[debug insert_file] cant_files == 0\n");
		dir->cant_files++;
		dir->sub_files = file;
	} else {
		printf("[debug insert_file] cant_files != 0\n");
		dir->cant_files++;
		dir = dir->sub_files;
		printf("[debug insert_file] 1° sub_file (%s)\n", dir->path);
		while (dir->next != NULL) {
			dir = dir->next;
			printf("[debug insert_file] next sub_file (%s)\n",
			       dir->path);
		}
		dir->next = file;
		printf("[debug insert_file] next of (%s) is (%s)\n",
		       dir->path,
		       dir->next->path);
	}
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	struct File *dir = find_dir(path, filesystem);
	if (!dir)
		return -ENOENT;

	dir->last_access = time(NULL);
	for (struct File *curr = dir->sub_files; curr != NULL; curr = curr->next) {
		// recorro los sub-files del dir pedido
		char *file_name = strrchr(curr->path, '/') + 1;
		filler(buffer, file_name, NULL, 0);
	}

	return 0;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create(%s)\n", path);

	struct File *file = malloc(sizeof(struct File));

	if (!file) {
		return -ENOMEM;
	}

	file->cant_files = 0;
	strcpy(file->path, path);
	file->permissions = __S_IFREG | mode;
	time_t actual_time = time(NULL);
	file->last_modified = actual_time;
	file->last_access = actual_time;
	file->change_time = actual_time;

	file->is_dir = false;
	file->sub_files = NULL;
	file->file_size = 0;
	file->next = NULL;
	memset(file->data, 0, sizeof(file->data));

	char *dir_path = get_containing_path(path);
	printf("[debug] DIR_PATH: (%s)\n", dir_path);

	// Busco el directorio en el cual se crea el nuevo archivo
	struct File *dir = find_dir(dir_path, filesystem);
	if (dir == NULL) {
		printf("[debug] No lo encuentra fuck\n");
		return -ENOENT;
	}
	printf("[debug] inserting file (%s) in dir (%s)\n", file->path, dir->path);

	insert_file(dir, file);

	printf("[debug] finished inserting\n");

	return 0;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir(%s)\n", path);

	struct File *sub_dir = malloc(sizeof(struct File));
	sub_dir->cant_files = 0;
	sub_dir->file_size = 0;
	sub_dir->permissions = __S_IFDIR | mode;
	time_t actual_time = time(NULL);
	sub_dir->last_access = actual_time;
	sub_dir->last_modified = actual_time;
	sub_dir->change_time = actual_time;
	strcpy(sub_dir->path, path);
	sub_dir->sub_files = NULL;
	sub_dir->next = NULL;
	sub_dir->is_dir = true;

	char *dir_path = get_containing_path(path);

	// Busco el directorio en el cual se crea el nuevo archivo
	struct File *dir = find_dir(dir_path, filesystem);
	if (dir == NULL) {
		printf("[debug] No lo encuentra fuck\n");
		return -ENOENT;
	}

	insert_file(dir, sub_dir);

	return 0;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir(%s)\n", path);

	if (strcmp(path, "/") == 0) {
		return -EPERM;
	}

	char *containing_dir = get_containing_path(path);

	struct File *root = find_dir(containing_dir, filesystem);
	struct File *dir = find_dir(path, filesystem);
	if (!dir || !root) {
		return -ENOENT;
	}

	if (root->sub_files == dir) {
		root->sub_files = dir->next;
		root->cant_files--;
	} else {
		for (struct File *curr = root->sub_files; curr != NULL;
		     curr = curr->next) {
			if (curr->next == dir) {
				curr->next = dir->next;
				root->cant_files--;
				break;
			}
		}
	}

	free(dir);
	return 0;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink(%s)\n", path);

	if (strcmp(path, "/") == 0) {
		return -EPERM;
	}

	char *containing_dir = get_containing_path(path);

	struct File *dir = find_dir(containing_dir, filesystem);
	struct File *file = find_file(path, filesystem);
	if (!file || !dir) {
		return -ENOENT;
	}

	if (dir->sub_files == file) {
		dir->sub_files = file->next;
		dir->cant_files--;
	} else {
		for (struct File *curr = dir->sub_files; curr != NULL;
		     curr = curr->next) {
			if (curr->next == file) {
				curr->next = file->next;
				dir->cant_files--;
				break;
			}
		}
	}

	free(file);
	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu)\n", path, offset, size);

	struct File *file = find_file(path, filesystem);
	if (!file)
		return -ENOENT;

	char *data = file->data;

	if (offset + size > strlen(data))
		size = strlen(data) - offset;

	size = size > 0 ? size : 0;

	strncpy(buffer, data + offset, size);

	file->last_access = time(NULL);

	return size;
}

static int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write(%s, %s, %lu, %lu)\n", path, buffer, size, offset);

	struct File *file = find_file(path, filesystem);
	if (!file)
		return -ENOENT;

	char *data = file->data;

	if (offset + size > BLOCK_SIZE)
		size = BLOCK_SIZE - offset;

	size = size > 0 ? size : 0;
	time_t now = time(NULL);
	if(size > 0) {
		file->change_time = now;
		file->last_modified = now;
	}
	strncpy(data + offset, buffer, size);

	file->file_size += size;
	file->last_access = now;

	return size;
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	// TODO: hay que modificar los tiempos si es un directorio?
	struct File *file = find_file(path, filesystem);
	if (!file)
		return -ENOENT;

	if (!tv) {
		file->last_access = time(NULL);
		file->last_modified = time(NULL);
	} else {
		file->last_access = tv[0].tv_sec;
		file->last_modified = tv[1].tv_sec;
	}

	return 0;
}

static int
fisopfs_truncate(const char *path, off_t offset)
{
	printf("[debug] fisopfs_truncate(%s, %lu)\n", path, offset);

	return 0;
}

static struct fuse_operations operations = { .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .read = fisopfs_read,
	                                     .create = fisopfs_create,
	                                     .mkdir = fisopfs_mkdir,
	                                     .utimens = fisopfs_utimens,
	                                     .rmdir = fisopfs_rmdir,
	                                     .unlink = fisopfs_unlink,
	                                     .write = fisopfs_write,
	                                     .truncate = fisopfs_truncate,
	                                     .flush = fisopfs_flush,
	                                     .destroy = fisopfs_destroy };

int
main(int argc, char *argv[])
{
	int total_args = argc;
	if (argc == 4) {
		total_args = 3;
		strcpy(filename, argv[3]);
	} else if (argc == 3 && argv[1][0] == '.') {
		total_args = 2;
		strcpy(filename, argv[2]);
	} else {
		strcpy(filename, "filesystem.fisopfs");
	}

	printf("[debug] filename: %s\n", filename);
	fisopfs_init();
	return fuse_main(total_args, argv, &operations, NULL);
}
