# Lab: shell

### Búsqueda en $PATH
#### Pregunta 1

La diferencia es que "execve(2)" es una syscall realizada de forma directa al kernel, que se
encarga de reemplazar la ejecución del programa actual por uno nuevo, inicializando sus
correspondientes stack, heap y data segments. En cambio, la familia "exec(3)" son wrappers
(funciones) que no se comunican con el kernel, sino que internamente realizan otras cosas y
luego hacen uso de la syscall execve. Esto es para simplificar al usuario tareas cómo convertir
el nombre del archivo a ejecutar en su path. 

#### Pregunta 2

Las funciones de "exec(3)" pueden todas fallar en errores iguales a los de "execve(2)". En ese
caso, la función invocada devuelve -1 y se setea el errno en el numero correspondiente. La
shell, por su lado imprime un mensaje del estilo bash:... y finaliza con un exit code distinto
de 0. Por ejemplo, si se intenta correr un programa que no existe ./no_existe, el exit code es
127.
En el caso de nuestra shell, el proceso hijo termina con un exit code de -1 e imprime un mensaje
de error. Luego, en el proceso padre, se imprime el promt para esperar una nueva entrada del
usuario.

---

### Comandos built-in

Los comandos built-in cd y pwd podrian no ser built-in y funcionar de la misma manera. Para ello,
se podria crear un programa que encapsule su funcionamiento, y desde la shell hacer fork, e
invocarlo al programa con execve como un comando regular.
Sin embargo, estos comandos son built-in para optimizar el funcionamiento de la terminal. Esto es
posible, ya que cuando llega un comando built-in, el programa no llega a realizar fork ni ejecutar
comandos con execve. Por lo tanto, nos ahorramos el tiempo y espacio de crear un nuevo proceso, de
copiar la imagen al mismo y tener que esperarlo desde el padre.

---

### Variables de entorno adicionales
#### Pregunta 1
Las variables de entorno temporales deben ser reemplazadas luego del fork ya que estas no deben 
formar parte de la imagen del proceso shell, es decir el padre, sino que solo deben durar lo que
dure el funcionamiento del programa hijo.

#### Pregunta 2
Aquellas funciones de exec(3) terminadas en e tienen un tercer parametro donde se puede pasar las
variables de entorno para que el proceso utilice al ejecutarse. Lo que deberiamos hacer para que
funcione de la misma manera, es crear un array donde tengamos en un primer lugar lo que nos devuelve
getenv, es decir las variables ya existentes en la imagen actual, y luego al final de ese array
agregar las variables de entorno temporarias para el proceso hijo y un NULL al final. Luego,
llamando a execvpe podriamos tener un funcionamiento igual.

---

### Procesos en segundo plano

Los procesos en segundo plano se ejecutan de la misma manera que el resto, realizando un fork y
luego llamando a execve(2). La unica diferencia se hace sobre el proceso padre, el cuál no deberá
hacer un wait al proceso recien lanzado. Además, luego de cada comando procesado, en la shell hacemos
una rapida comprobación de si hay procesos hijos que hayan terminado (procesos que estaban en segundo
plano) y si los hay los esperamos, sino seguimos con el flujo normal del programa.

---

### Flujo estándar

La idea de esta redirección es que la salida estandar de error sea redirigida a la salida estandar
del programa. Lo que pasa internamente es que se modifica el file descriptor 2 y se lo apunta a donde
apunta el file descriptor 1. La salida mostrada en el ejemplo es la siguiente:
´´´
ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
/home:
teo-c
´´´
En cambio si giramos las redirecciones la terminal imprime por salida estandar el error ls: no se puede
acceder a... Esto sucede ya que estamos primero redirigiendo la salida de error a la estandar que sigue
siendo la shell y luego redirigiendo la salida estandar al archivo out.txt

---

### Tuberías simples (pipes)

En caso de usar uno o multiples pipes, la shell tomará como exit code el pertenciente al ultimo
programa ejecutado, es decir, al que se encuentre mas a la derecha. Sin embargo, también se puede
acceder al resto de los exit codes con la variable de entorno $PIPESTATUS[i] donde i corresponde al
programa i en orden 0,1,2,..,n.

Ejemplo:
```
$ true | false
$ echo $?
1

$ false | true
$ echo $?
0

$ false | true
$ echo "${PIPESTATUS[0]} ${PIPESTATUS[1]}"
1 0
```

---

### Pseudo-variables

Existen multiples variables de entorno mágicas, algunos ejemplos de uso son:
1. El uso de $$ nos devuelve el PID del proceso en que estamos, es decir el de la shell
```
$ echo $$
2739
```

2. El uso de $_ nos devuelve el ultimo argumento pasado al ultimo comando ejecutado en la shell
```
$ echo hola mundo
$ echo $_
mundo
```

3. El uso de $0 nos devuelve el nombre del proceso actual
```
$ echo $0
bash    // En este caso el proceso es la bash
```

4. El uso de $! nos devuelve el PID del ultimo proceso ejecutado en background
```
$ echo wait 3 &
[1] 3308
$ echo hola
hola
$ echo $!
3308    // Si bien despues ejecute echo hola, me imprime el PID del que fue en segundo plano
```
    
---