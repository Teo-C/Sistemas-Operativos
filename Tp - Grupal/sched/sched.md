# sched.md

## Parte 1
##### Implementación de context_switch
Primero veamos la implementación del context switch:
```
.globl context_switch;
context_switch:
	add $4, %esp
	movl (%esp), %edx
	movl %edx, %esp

	popal

	popl %es
	popl %ds

	add $0x8, %esp
	iret
```

1) Se apunta `%esp` al struct Trapframe
2) Se restauran los registros de propósito general mediante `popal`
3) Se restauran los registros `%es` y `ds`
4) Se omiten `trapno` y `errno` y se llama a `iret` 

##### Seguimiento con GDB
Al entrar a la función los valores de los registros son los siguientes:
(![image](https://drive.google.com/uc?export=view&id=1kytycXBwYoB9ueVIPQKM38IsNBc37wU5))

Luego de ejecutar la función popal:
(![image](https://drive.google.com/uc?export=view&id=1ytP8l9T8qX_EnFPEgBH1cCfNx6K4l9ha)).
Observamos que cambiaron los registros de propósito general.

Luego se hace popl y se guardan los registros `%es` y `%ds`:
(![image](https://drive.google.com/uc?export=view&id=14C9nEevP5GqhwRjnkOrVm3aBO71d6om0))

Luego de ejecutar `iret`:
(![image](https://drive.google.com/uc?export=view&id=16Yqy9e-MrtzGyPEzIvnKURHS4_x1UxaK))

---
## Parte 2

Para la implemetación de Round Robin la idea es básica, obtener el próximo proceso a el actual(enviroment para JOS) listo para ser ejecutado. Para ello implementamos la siguiente función:
```c
	struct Env* next_runnable_env(int index) {
	struct Env* current; 

	// Desde curent_env hasta final
	for (int i = index+1; i < NENV + index; i++) {
		current = &envs[i];
		if (current && current->env_status == ENV_RUNNABLE) return (current);
	}

	// Desde 0 hasta current_env
	for (int j = 0; j < index + NENV; j++) {
		current = &envs[j];
		if (current && current->env_status == ENV_RUNNABLE) return (current);
	}

	return NULL;
}
```

En ella se recorre todo el array **envs** buscando el enviroment *RUNNABLE*, si lo encuentra, lo ejecuta.
 
## Parte 3
---
En la parte de escoger un scheduling por prioridades, optamos por **lottery scheduling** principalmente por los buenos resultados que este brinda como también la simplicidad del algoritmo. Para éste necesitamos un algoritmo que genere números pseudoaleatorios en el cual nos basamos en la referencia dada en el libro *OSTEP*. Además cada proceso tiene un número máximo de tickets que puede poseer.
La idea principal de la implementación es recorrer el array de enviroments e ir sumando la cantidad de tickets a un contador,en el momento que el contador supere el número de ticket ganador, el proceso en el que esté actualmente apuntando será a ser ejecutado. Es decir el proceso que tenga mayor numero de ticket/prioridad tendrá la mayor chance de ser seleccionado por el scheduler.Cabe destacar que tanto el ticket ganador como la prioridad de cada proceso se elige de forma random.
Por otro lado se agrego syscalls para obtener y modificar los tickets

```c
static int
sys_set_tkts(envid_t pid, int env_tkts) {
	int rcode = -1;
	int env_i = ENVX(pid);
	if((env_tkts < ENVMAXTICKETS) && 
		(env_tkts < envs[env_i].priority && 
		curenv->env_id == envs[env_i].env_id)) {
		env_modify_priority(&envs[env_i], env_tkts);
		rcode = 0;
	}
	return rcode;
} 
```

```c

static int sys_get_tkts(envid_t pid) {
	int env_i = ENVX(pid);
	return envs[env_i].priority;
}

```

como también para los procesos hijos se aplicó el criterio de que estos tengan el 90% de los ticket/prioridad de su proceso padre.