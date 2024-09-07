#ifndef SAMPLING_H
#define SAMPLING_H

#include "pg_prng.h"
#include "storage/buf/block.h"		/* for typedef BlockNumber */


/* Random generator for sampling code */
extern void sampler_random_init_state(uint32 seed,
									  pg_prng_state *randstate);
extern double sampler_random_fract(pg_prng_state *randstate);

/* Block sampling methods */

/* Data structure for Algorithm S from Knuth 3.4.2 */
typedef struct
{
	BlockNumber N;				/* number of blocks, known in advance */
	uint32			n;				/* desired sample size */
	BlockNumber t;				/* current block number */
	uint32			m;				/* blocks selected so far */
	pg_prng_state randstate;	/* random generator state */
} BlockSamplerData2;

typedef BlockSamplerData2 *BlockSampler2;

extern BlockNumber BlockSampler_Init2(BlockSampler2 bs, BlockNumber nblocks,
									 int samplesize, uint32 randseed);
extern bool BlockSampler_HasMore2(BlockSampler2 bs);
extern BlockNumber BlockSampler_Next2(BlockSampler2 bs);

/* Reservoir sampling methods */

typedef struct
{
	double		W;
	pg_prng_state randstate;	/* random generator state */
} ReservoirStateData;

typedef ReservoirStateData *ReservoirState;

extern void reservoir_init_selection_state(ReservoirState rs, int n);
extern double reservoir_get_next_S(ReservoirState rs, double t, int n);

#endif							/* SAMPLING_H */
