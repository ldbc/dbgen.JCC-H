/*
 * phash.h
 *
 *  Created on: Jun 9, 2017
 *      Author: aca
 */

#ifndef SKEW_PHASH_H_
#define SKEW_PHASH_H_

#include <inttypes.h>

/* a cheap randomized invertable permutation */
typedef struct {
	unsigned long maxval; /* here: the max table size, assuming key=1,..maxval */ 
	unsigned long xorval; /* cheapo bit-randomizes (does not touch the MSB) */
	unsigned long invval; /* for the ring Z/pZ, i.e. (PRIME*invval)%maxval == 1 */
} phash_t;

extern phash_t phash_part;
extern phash_t phash_supplier;
extern phash_t phash_customer;
extern phash_t phash_orders;

char* skew_regions[5];
char* skew_nations[25];

extern int JCCH_skew;

void init_skew();
unsigned long phash(uint64_t key, phash_t *p, int invert);
uint16_t bin_nationkey(uint64_t key, uint64_t tbl_size);

#endif /* SKEW_PHASH_H_ */
