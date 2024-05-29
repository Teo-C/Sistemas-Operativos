
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "printfmt.h"
#include "def.h"

#define COLOR_RED "\x1b[1m\x1b[31m"
#define COLOR_GREEN "\x1b[1m\x1b[32m"
#define COLOR_RESET "\x1b[0m"

void statistics(struct stats_info *stats);
void assert_true(int value, int expected, char *msg);
void test_mallocs_and_frees();
void test_amount_of_blocks();
void test_realloc();

struct stats_info stats;

void
assert_true(int value, int expected, char *msg)
{
	printfmt("%s -> ", msg);
	if (value == expected) {
		printfmt("%sOK\n", COLOR_GREEN);
	} else {
		printfmt("%sFAIL\n", COLOR_RED);
	}
	printfmt(COLOR_RESET);
}

void
test_mallocs_and_frees()
{
	printfmt("\n************ TEST MALLOCS AND FREES ************\n");

	// pido memoria
	char *var1 = malloc(160);

	// libero memoria
	free(var1);

	statistics(&stats);
	assert_true(stats.mallocs, 1, "La cantidad de mallocs es 1");
	assert_true(stats.frees, 1, "La cantidad de frees es 1");

	/*****************************************************************/
	var1 = malloc(50);
	char *var2 = malloc(100);
	strcpy(var2, "HOLA");

	// libero solo var1
	free(var1);

	statistics(&stats);
	assert_true(stats.mallocs, 3, "La cantidad de mallocs es 3");
	assert_true(stats.frees, 2, "La cantidad de frees es 2");


	/*****************************************************************/

	// hago malloc pero no free
	char *var3 = malloc(100);
	strcpy(var3, "HOLA");

	char *var4 = malloc(600);
	strcpy(var4, "HOLA");

	statistics(&stats);
	assert_true(stats.mallocs, 5, "La cantidad de mallocs es 5");
	assert_true(stats.frees, 2, "La cantidad de frees sigue siendo 2");

	// reset
	free(var2);
	free(var3);
	free(var4);
}

void
test_amount_of_blocks()
{
	printfmt("\n************ TESTS AMOUNT OF BLOCKS ************\n");

	char *var1 = malloc(160);
	statistics(&stats);

	assert_true(stats.small_blocks, 1, "Cantidad de bloques chicos es 1");
	assert_true(stats.medium_blocks, 0, "Cantidad de bloques medianos es 0");
	assert_true(stats.large_blocks, 0, "Cantidad de bloques grandes es 0");
	free(var1);

	statistics(&stats);
	assert_true(stats.small_blocks, 0, "Cantidad de bloques chicos despues de liberar toda la memoria es 0");

	/************************************************************************/

	var1 = malloc(16000);
	char *var2 = malloc(2000);
	statistics(&stats);

	assert_true(stats.small_blocks, 2, "Cantidad de bloques chicos es 2");
	assert_true(stats.medium_blocks, 0, "Cantidad de bloques medianos es 0");
	assert_true(stats.large_blocks, 0, "Cantidad de bloques grandes es 0");

	free(var1);
	free(var2);

	statistics(&stats);
	assert_true(stats.small_blocks, 0, "Cantidad de bloques chicos despues de liberar toda la memoria es 0");

	var1 = malloc(16000);
	var2 = malloc(104800);
	char *var3 = malloc(3300000);

	statistics(&stats);

	assert_true(stats.small_blocks, 1, "Cantidad de bloques chicos es 1");
	assert_true(stats.medium_blocks, 1, "Cantidad de bloques medianos es 1");
	assert_true(stats.large_blocks, 1, "Cantidad de bloques grandes es 1");

	free(var1);
	free(var2);
	free(var3);
}

void
test_realloc()
{
	printfmt("\n************ TESTS REALLOC FUNCTION ************\n");

	char *string = "String";

	// intento realloc con un puntero que no fue retornado por malloc
	string = realloc(string, 1000);

	statistics(&stats);
	assert_true(stats.reallocs, 0, "Cantidad de reallocs es 0");

	/************************************************************************/

	char *var = malloc(100);
	char *var2 = malloc(500);

	// realloc's de tamaño mas grande (debe hacer dos mallocs por dentro)
	var = realloc(var, 600);
	var2 = realloc(var2, 501);

	statistics(&stats);
	assert_true(stats.reallocs, 2, "Cantidad de reallocs es 2");
	assert_true(stats.mallocs,
	            15,
	            "Cantidad de mallocs hasta el momento es de 15");

	free(var);
	free(var2);

	char *var3 = realloc(NULL, 200);

	statistics(&stats);
	assert_true(stats.frees, 15, "Cantidad de frees hasta el momento es de 15");
	assert_true(stats.mallocs, 16, "realloc de ptr NULL es igual a malloc: ahora la cantidad de mallocs es de 16");

	realloc(var3, 0);
	statistics(&stats);
	assert_true(stats.frees, 16, "realloc de size 0 es igual a free: ahora la cantidad de frees es 16");
}


void
test_coalescing()
{
	printfmt("\n************ TESTS COALESCING ************\n");

	char *var1 = malloc(100);
	char *var2 = malloc(200);
	char *var3 = malloc(300);
	char *var4 = malloc(500);

	// 4 frees consecutivos en orden es igual a 4 merges
	free(var1);
	free(var2);
	free(var3);
	free(var4);

	/************************************************************************/

	statistics(&stats);
	assert_true(stats.merges, 20, "Cantidad de merges es 20");


	var1 = malloc(100);
	var2 = malloc(200);
	var3 = malloc(300);
	var4 = malloc(500);

	// sin cambios en merges porque libero una region
	// que no tiene libres a su alrededor
	free(var2);
	statistics(&stats);

	assert_true(stats.merges, 20, "Cantidad de merges sigue siendo 20");

	// ahora si deberia hacer un merge
	free(var3);
	statistics(&stats);
	assert_true(stats.merges, 21, "Cantidad de merges ahora es de 21");

	// merges restantes 3 (contando la region libre generada por el splitting)
	free(var1);
	free(var4);
	statistics(&stats);
	assert_true(stats.merges, 24, "Cantidad de merges ahora es de 24");
}

void
test_splitting()
{
	printfmt("\n************ TESTS SPLITTING ************\n");

	// genera 1 split mas.
	char *var1 = malloc(10);
	free(var1);
	statistics(&stats);

	// 25 splits porque hasta antes del malloc(10) eran 24
	assert_true(stats.splits,
	            25,
	            "Cantidad de splits hasta el momento es de 25");

	/************************************************************************/
	// alloc de un bloque completo
	char *var2 = malloc(16344);
	free(var2);

	statistics(&stats);
	// se siguen manteniendo 25 splits
	assert_true(stats.splits, 25, "Cantidad de splits despues de alloc de un bloque completo sigue siendo de 25");
}


int
main(void)
{
	test_mallocs_and_frees();
	test_amount_of_blocks();
	test_realloc();
	test_coalescing();
	test_splitting();
	return 0;
}