# TP: malloc

---

## Estructuras de datos

### Regiones
Implementamos las regiones como una lista simplemente enlazada en donde cada nodo tiene un puntero al
bloque que la contiene. La idea detrás de esto es la de poder optimizar el uso de la memoria al momento
de hacer `free()` ya sea mediante el algoritmo de coalescing o desalojando el bloque si este se
encuentra vacío.

### Bloques
Los bloques los implementamos como una lista doblemente enlazada para poder desalojar bloques vacíos
de manera más sencilla al hacer `free()` de una región dentro del mismo. Los mismos cuentan también
con una lista de regiones por las cuales está compuesto.

---

## Implementación

En una primera implementación partimos con los tres tipos de bloques pedidos (pequeño, mediano, grande) y las funciones `malloc()` y `free()` que contenían un comportamiento bastante aceptable pero con diferentes errores en asignación y liberación de memoria. Estos errores se fueron puliendo con las técnicas de *coalescing* , *splitting* como también *first fit* y *best fit*.\
De las dos funciones anteriormente mencionadas las que mas nos costó trabajo fue `free()` ya que en ella debemos hacer toda la lógica de *coalescing* y de sacar los diferente **bloques** o nodos de las lista de bloques para actualizarla.

La siguiente función lleva a cabo la actualización de la lista:


```c
void
deallocate_block(struct block *block)
{
	if (block->next && block->prev) {
		block->prev->next = block->next;
		block->next->prev = block->prev;

	} else if (block->prev && !block->next) {
		block->prev->next = block->next;

	} else {
		block->next->prev = block->prev;
		if (block->block_type == SMALL) {
			small_blocks_list = block->next;	
		} else if (block->block_type == MEDIUM) {
			medium_blocks_list = block->next;
		} else {
			large_blocks_list = block->next;
		}
	}
	decrease_blocks(block);
}
```

Además con `realloc()` tuvimos inconveniente en devolver la nueva región dada por `malloc()` ya que la estabamos casteando a `struct region*` y no a un `void *`, también copiar los datos de la vieja región a la nueva, como primera idea hicimos una función nuestra pero notamos que no copiaba los datos correctamente, entonces decidimos que lo mejor era usar `memcpy()`.\
Por último, para hacer el chequeo de memoria válida el algoritmo consiste en recorrer cada una de las regiones y comparar la dirección de memoria de esta con la que se ingresa por parámetro, de esta manera sabemos si pertenece o no a una región retornada por `malloc()`.