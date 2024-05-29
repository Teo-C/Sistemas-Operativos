#define RAND_MAX 32767

/*
https://stackoverflow.com/questions/4768180/rand-implementation
https://stackoverflow.com/questions/2509679/how-to-generate-a-random-integer-number-from-within-a-range
*/

static unsigned long int next = 1;

// Return random int
int rand(void) 
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % (RAND_MAX+1);
}

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
int random(long max) {
  unsigned long
    // max <= RAND_MAX < ULONG_MAX, so this is okay.
    num_bins = (unsigned long) max + 1,
    num_rand = (unsigned long) RAND_MAX + 1,
    bin_size = num_rand / num_bins,
    defect   = num_rand % num_bins;

  long x;
  do {
   x = rand();
  }
  // This is carefully written not to overflow
  while (num_rand - defect <= (unsigned long)x);

  // Truncated division is intentional
  return (int) x/bin_size;
}
