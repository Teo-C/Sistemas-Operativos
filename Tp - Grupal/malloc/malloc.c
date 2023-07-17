#define _DEFAULT_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include "def.h"

#include "malloc.h"
#include "printfmt.h"

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

#define MIN_SIZE 8
#define SMALL_BLOCK_SIZE 16384
#define MEDIUM_BLOCK_SIZE 1048576
#define LARGE_BLOCK_SIZE 33554432

typedef enum { SMALL, MEDIUM, LARGE } block_type;

struct region {
	bool free;
	size_t size;
	struct block *block;
	struct region *next;
};

struct block {
	size_t size;
	block_type block_type;
	struct block *next;
	struct block *prev;
	struct region *regions;
};

void initialize_block(size_t size, block_type type, struct block *block);
void *_mmap(size_t size);
void coalesce_regions(struct block *block);
void split(struct region *current, size_t size);
struct region *best_fit(struct block *blocks_list, size_t size);
struct region *find_region_by_address(struct block *block, void *ptr);
struct region *find_address(void *ptr);
void append_block(struct block *new_block, struct block **blocks);
void statistics(struct stats_info *stats);
void deallocate_block(struct block *block);
bool block_is_empty(struct block *block);
void decrease_blocks(struct block *block);
void reset_list(block_type type);


struct block *small_blocks_list = NULL;
struct block *medium_blocks_list = NULL;
struct block *large_blocks_list = NULL;

/********************** ESTADISTICAS **********************/

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
int amount_of_merges = 0;
int amount_of_blocks = 0;
int amount_of_small_blocks = 0;
int amount_of_medium_blocks = 0;
int amount_of_large_blocks = 0;
int amount_of_splits = 0;
int amount_of_reallocs = 0;

void
statistics(struct stats_info *stats)
{
	stats->blocks = amount_of_blocks;
	stats->request_memory = requested_memory;
	stats->splits = amount_of_splits;
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->merges = amount_of_merges;
	stats->small_blocks = amount_of_small_blocks;
	stats->medium_blocks = amount_of_medium_blocks;
	stats->large_blocks = amount_of_large_blocks;
	stats->reallocs = amount_of_reallocs;
}

/********************** FUNCIONES AUXILIARES **********************/

void *
_mmap(size_t size)
{
	return mmap(
	        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

// For each region in a block applies is_valid function. If it is true, returns
// the region. If not, returns NULL
struct region *
find_region(struct block *blocks_list,
            void *data,
            bool (*is_valid)(struct region *, void *))
{
	struct block *block = blocks_list;
	while (block) {
		struct region *region = block->regions;
		while (region) {
			if (is_valid(region, data)) {
				return region;
			}
			region = region->next;
		}
		block = block->next;
	}
	return NULL;
}

// Checks if a region is free and has the required size
bool
is_region_free(struct region *region, void *size_required)
{
	size_t *size = (size_t *) size_required;
	return region->free && region->size >= *size;
}

// Checks if the region adress is valid
bool
is_adress_valid(struct region *region, void *ptr)
{
	return (region == ptr);
}

// return best region in list of blocks
struct region *
best_fit(struct block *blocks_list, size_t size)
{
	struct block *block = blocks_list;
	struct region *region = block->regions;
	struct region *best = region;
	while (block) {
		while (region) {
			if (region->free && region->size >= size &&
			    region->size < best->size) {
				best = region;
			}
			region = region->next;
		}
		block = block->next;
	}

	return best;
}

// finds the next free region with the requested size
static struct region *
find_free_region(size_t size)
{
	struct region *free_region;
	struct block *blocks[] = { small_blocks_list,
		                   medium_blocks_list,
		                   large_blocks_list };
#ifdef FIRST_FIT

	for (int i = 0; i < 3; i++) {
		// if block is not initialized
		if (!blocks[i]) {
			continue;
		}
		free_region = find_region(blocks[i], &size, is_region_free);
		if (free_region) {
			return free_region;
		}
	}

#endif

#ifdef BEST_FIT
	struct region *best = NULL;
	// search through all blocks (small, medium and large) for best fit
	for (int i = 0; i < 3; i++) {
		// if block is not initialized
		if (!blocks[i])
			continue;

		free_region = best_fit(blocks[i], size);
		if (!best || free_region->size < best->size) {
			best = free_region;
		}
	}
	return best;

#endif

	return NULL;
}

void
append_block(struct block *new_block, struct block **blocks)
{
	struct block *curr = *blocks;
	if (!curr) {
		*blocks = new_block;
		return;
	}
	while (curr->next) {
		curr = curr->next;
	}

	curr->next = new_block;
	new_block->prev = curr;
}

static struct region *
grow_heap(size_t size)
{
	// requested size is too large
	if (size > (LARGE_BLOCK_SIZE - sizeof(struct block))) {
		printfmt("malloc failed\n");
		return NULL;
	}

	struct block *new_block;

	if (size <= (SMALL_BLOCK_SIZE - sizeof(struct block))) {
		new_block = _mmap(SMALL_BLOCK_SIZE);
		amount_of_small_blocks++;
		initialize_block(SMALL_BLOCK_SIZE, SMALL, new_block);
		append_block(new_block, &small_blocks_list);
	} else if (size <= (MEDIUM_BLOCK_SIZE - sizeof(struct block))) {
		new_block = _mmap(MEDIUM_BLOCK_SIZE);
		amount_of_medium_blocks++;
		initialize_block(MEDIUM_BLOCK_SIZE, MEDIUM, new_block);
		append_block(new_block, &medium_blocks_list);
	} else {
		new_block = _mmap(LARGE_BLOCK_SIZE);
		amount_of_large_blocks++;
		initialize_block(LARGE_BLOCK_SIZE, LARGE, new_block);
		append_block(new_block, &large_blocks_list);
	}
	return new_block->regions;
}


void
split(struct region *current, size_t size)
{
	amount_of_splits++;

	struct region *new = current;
	new = (struct region *) ((char *) new + size + sizeof(struct region));

	new->size = current->size - size - sizeof(struct region);
	new->next = current->next;
	new->free = true;
	new->block = current->block;

	current->size = size;
	current->next = new;
}

void
initialize_block(size_t size, block_type type, struct block *block)
{
	amount_of_blocks++;

	block->block_type = type;
	block->size = size - sizeof(struct block);
	block->next = NULL;
	block->prev = NULL;
	// assign single region of size 'size' to new block
	struct region *new_region = ((void *) block);
	new_region =
	        (struct region *) ((char *) new_region + sizeof(struct block));

	new_region->free = true;
	new_region->size = block->size - sizeof(struct region);
	new_region->next = NULL;
	new_region->block = block;
	block->regions = new_region;
}

void
decrease_blocks(struct block *block)
{
	if (block->block_type == SMALL) {
		amount_of_small_blocks--;
	} else if (block->block_type == LARGE) {
		amount_of_medium_blocks--;
	} else {
		amount_of_large_blocks--;
	}
}

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

bool
block_is_empty(struct block *block)
{
	size_t total_size_region = block->regions->size + sizeof(struct region);
	return block->size == total_size_region && block->regions->free;
}

void
reset_list(block_type type)
{
	if (type == SMALL) {
		small_blocks_list = NULL;
	} else if (type == MEDIUM) {
		medium_blocks_list = NULL;
	} else {
		large_blocks_list = NULL;
	}
}

void
coalesce_regions(struct block *block)
{
	struct region *reg = block->regions;
	while (reg->next) {
		if (reg->free && reg->next->free) {
			amount_of_merges++;
			reg->size += reg->next->size + sizeof(struct region);
			reg->next = reg->next->next;

			reg = reg;
		} else {
			reg = reg->next;
		}
	}
}

struct region *
find_address(void *ptr)
{
	struct region *region;
	struct block *blocks[] = { small_blocks_list,
		                   medium_blocks_list,
		                   large_blocks_list };
	for (int i = 0; i < 3; i++) {
		// If block is not initialized
		if (!blocks[i]) {
			continue;
		}
		region = find_region(blocks[i], ptr, is_adress_valid);
		if (region) {
			return region;
		}
	}
	return NULL;
}


/********************** FUNCIONES PRINCIPALES **********************/

void *
malloc(size_t size)
{
	if (amount_of_mallocs == 0) {
		small_blocks_list = _mmap(SMALL_BLOCK_SIZE);
		amount_of_small_blocks++;
		initialize_block(SMALL_BLOCK_SIZE, SMALL, small_blocks_list);
	}
	struct region *next;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	if (size < MIN_SIZE) {
		size = MIN_SIZE;
	}

	next = find_free_region(size);
	if (!next) {
		next = grow_heap(size);
	}

	if (!next) {
		errno = ENOMEM;
		return NULL;
	}

	next->free = false;

	if (next->size >= (size + sizeof(struct region))) {
		split(next, size);
	}
	return REGION2PTR(next);
}

void
free(void *ptr)
{
	struct region *curr = find_address(PTR2REGION(ptr));
	if (!curr) {
		return;
	}

	// updates statistics
	amount_of_frees++;
	struct block *block;

	assert(curr->free == 0);
	block = curr->block;
	curr->free = true;

	coalesce_regions(block);

	if (block_is_empty(block)) {
		struct block *temp = block;
		amount_of_blocks--;
		if (!block->next && !block->prev) {
			decrease_blocks(block);
			reset_list(block->block_type);
			munmap(temp, temp->size);
			return;
		}
		deallocate_block(block);
		munmap(temp, temp->size);
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	if (nmemb == 0) {
		return NULL;
	}
	struct region *new;
	size_t region_size;
	new = malloc(size * nmemb);
	if (new) {
		region_size = ALIGN4(size * nmemb);
		memset(new, 0, region_size);
	}
	return new;
}

void *
realloc(void *ptr, size_t size)
{
	struct region *region;
	void *new_region;

	if (!ptr) {
		return malloc(size);
	}
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	region = find_address(PTR2REGION(ptr));

	if (region) {
		amount_of_reallocs++;
		size = ALIGN4(size);
		if (size < region->size) {
			if (region->size > (size + sizeof(struct region))) {
				split(region, size);
			}
		} else {
			if ((region->next && region->next->free) &&
			    (region->size + region->next->size) >= size) {
				requested_memory += size;
				region->size += region->next->size +
				                sizeof(struct region);
				region->next = region->next->next;
				if (region->size > (size + sizeof(struct region))) {
					split(region, size);
				}
			} else {
				new_region = malloc(size);
				if (!new_region) {
					return NULL;
				}
				memcpy(new_region, ptr, region->size);
				free(ptr);
				return new_region;
			}
		}
		return REGION2PTR(region);
	}
	return NULL;
}
