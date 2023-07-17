# Trabajo Práctico 3: File System FUSE - Sistemas Operativos - FIUBA

Grupo 09

---

## Estructuras utilizadas

Para la implementación del file system utilizamos una única estructura diseñada de la siguiente manera:

```c
struct File {
	char path[PATH_MAX];
	mode_t permissions;
	size_t file_size;
	time_t last_access;
	time_t last_modified;
	bool is_dir;
	char data[BLOCK_SIZE];
	int cant_files;
	struct File *sub_files;
	struct File *next;
};
```
La misma es usada tanto para directorios como para archivos ya que mucha de la metadata es compartida:
* Tenemos almacenados el path, los permisos, el tamaño y las fechas de ultima modificación y acceso que son comunes para archivos y directorios.
* Con un booleano (`bool is_dir`) indicamos si se trata de un directorio o de un archivo.
  * En caso de ser un archivo, almacenaremos en data el contenido (si no esta vacío) y cant_files será siempre 0, ya que no puede contener archivos dentro.
  * En caso de ser un directorio, data permanecerá vacío, mientras que cant_files indicara la cantidad de archivos y directorios contiene dentro.
* Tenemos un puntero a sub_files, que en caso de tratarse de un directorio no vacío apuntará a su primer struct File dentro. Si se trata de un archivo siempre apuntará a NULL.
* Por último el puntero next apunta a un struct File que será el siguiente File (archivo o directorio) del mismo nivel.

Así, tendremos globalmente la raíz de nuestro filesystem que sera el directorio con path "/" y en el que se apuntará a los archivos y directorios. Y recorriendo con next podemos ver todos los File del mismo nivel, mientras que entrando a subfiles tenemos acceso a los File dentro de un directorio.

## Busqueda de archivos y directorios

La forma en que planteamos el diseño del file system nos permite realizar una búsqueda recursiva de los archivos y los directorios. Para hacerlo creamos dos funciones auxiliares, una que busca directorios y otra que busca archivos ambas partiendo de un directorio recibido por parámetro.

```c
// Recibe el path buscado y el directorio en el que arrancar a buscar
struct File * find_dir(const char *path, struct File *dir) {

// Se recorre todos los archivos y directorios del nivel actual
for (struct File *curr = dir; curr != NULL; curr = curr->next) {
    // Si no es un directorio no nos interesa
    if (!curr->is_dir) {
        continue;
    }
    // Si es un directorio, comparo con el path buscado, y devuelvo el actual si coinciden
    if (strcmp(curr->path, path) == 0)
        return curr;
    // Realizo busqueda recursiva en el directorio actual (no es el buscado)
    struct File *found = find_dir(path, curr->sub_files);
    if (found) {
        return found;
    }
}
// No se encontró el directorio buscado. No existe
return NULL;
}
```

```c
// Recibe el path buscado y el directorio en el que arrancar a buscar
struct File * find_file(const char *path, struct File *dir) {
       
// Se recorre todos los archivos y directorios del nivel actual
for (struct File *curr = dir; curr != NULL; curr = curr->next) {
    // Comparo el path con el buscado y devuelvo el actual si coinciden
    if (strcmp(curr->path, path) == 0)
        return curr;
    // Si se trata de un directorio que no está vacío, realizo busqueda recursiva en el mismo
    if (curr->is_dir && curr->cant_files > 0) {
        return find_file(path, curr->sub_files);
    }
}
// No se encontró el archivo buscado. No existe
return NULL;
}
```

## Serialización y deserializacion

La serialización del filesystem se realiza almacenando los metadatos de a uno por linea en el orden en que se declaró struct File. Es decir, primero se almacena el path, luego un '\n'. Despues se almacenan los permisos y posteriormente un nuevo '\n'. Y así sucesivamente. Se hacen llamados recursivos para ir recorriendo igual que como se explicó previamente para serializar los directorios internos.

Esta forma de serializar nos es util para la deserialización, ya que para leer todos los metadatos, simplemente debemos leer una linea del archivo y convertirla al tipo de dato que corresponda.

Acá hacemos nuevamente uso de la recursividad, ya que cuando se trata de un directorio que contiene archivos o directorios dentro, realizamos un llamado a deserializar nuevamente el mismo.


## Pruebas
* Compilación
```
$ make
gcc -ggdb3 -O2 -Wall -std=c11 -Wno-unused-function -Wvla    fisopfs.c  -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -lfuse -pthread -o fisopfs
```

* Creacion de punto de montado y montado del file system
```shell
$ mkdir tests
$ ./fisopfs tests/
```

* Creacion de un archivo en tests
```shell
$ touch tests/file1.txt
$ ls tests
file1.txt
```

* Creación de un directorio y un archivo dentro

```shell
$ mkdir tests/dir1
$ touch tests/dir1/file2.txt
$ ls tests/
dir1 file1.txt
$ ls tests/dir1
file2.txt
```
* Redirección de salida estándar y eliminación de archivo

```shell
$ echo "hello world!" > file
$ cat file
$ hello world!
```
* Eliminar archivos

```shell
$ touch file1 file2
$ ls 
$ file1 file2
$ rm file1 file2
$ ls
$
```

* Eliminar directorios

```shell
$ mkdir folder1 folder2
$ file folder1/ folder2/
$ folder1/: directory
$ folder2/: directory
$ rm -r folder1 folder2
$ ls
$
```

* Comando more

```shell
$ echo "hello world!" > hello
$ more file
$ hello world!
```