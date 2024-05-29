# Sistemas-Operativos

Material y trabajos prácticos realizados en la materia Sistemas Operativos

Segundo cuatrimestre 2022 - Facultad de Ingeniería, Universidad de Buenos Aires

## Lab Fork
El objetivo del TP es familiarizarse con las llamadas al sistema (Syscalls) utilizadas para solicitar al Sistema Operativo que duplique un proceso (fork) o que proporcione un mecanismo de comunicación unidireccional entre dos procesos (pipe)

Para ello se realiza un programa en lenguaje C que utiliza dichas llamadas al sistema e implementa un ping-pong entre dos procesos.

## TP Shell
El objetivo del TP es desarrollar en lenguaje C, la funcionalidad mínima que caracteriza a un intérprete de comandos shell similar a lo que realizan bash, zsh, fish.

El programa debe soportar:
1. La ejecución de binarios almacenados en `/bin`
2. La ejecución de procesos en segundo plano
3. Redireccoión de stdin, stdout, stderr
4. Expansión de variables de entorno
5. La ejecución de comandos built-in como cd, exit, pwd

## TP Malloc
El objetivo de este TP es desarrollar una librería de usuario que implemente las funciones `malloc(3)`, `calloc(3)`, `realloc(3)` y `free(3)`. La librería implementada se encargará de administrar la memoria solicitada al sistema operativo, su liberación  y responsable uso.

## TP Scheduler
El objetivo del TP es implementar el context-switch (cambio de contexto) para procesos en ejecución y para el scheduler sobre un Sistema Operativo preexistente.

El kernel a utilizar será una modificación de JOS, un exokernel educativo con licencia libre del grupo de Sistemas Operativos Distribuidos del MIT.
JOS está diseñado para correr en la arquitectura Intel x86, y para poder ejecutarlo se utiliza QEMU que emula dicha arquitectura.

Se implementa:
1. Cambio de contexto (modo usuario a modo kernel y viceversa)
2. Scheduler Round-Robin
3. Scheduler con prioridades

## TP File System
El objetivo del TP es desarrollar un flesystem (sistema de archivos) para Linux. El mismo utiliza el mecanismo FUSE (File System in Userspace) provisto por el kernel. De esta forma se puede ejecutar en modo usuario.

La implementación del filesystem se realiza enteramente en memoria: tanto archivos como directorios serán representados mediante estructuras que vivirán en memoria RAM. Por esta razón, el TP busca un sistema de archivos que apunte a la velocidad de acceso, y no al volumen de datos o a la persistencia. Aún así, los datos de nuestro filesystem estarán representados en disco por un archivo.
