# TP File System

## Para compilar
```bash
$ make
```

## Distintas pruebas

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