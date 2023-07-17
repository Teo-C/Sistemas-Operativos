#ifndef DEF_H
#define DEF_H

struct stats_info
{
	int request_memory;
	int small_blocks;
	int medium_blocks;
	int large_blocks;
	int reallocs;
	int mallocs;
	int merges;
	int blocks;
	int splits;
	int frees;
};

#endif  //  DEF_H
