#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "phash.h"

static int pos[64] = /* numbers 0..63 in some random order */
{15,18,54,9,50,37,26,51,41,11,31,7,23,38,28,58,16,57,14,3,36,21,6,34,25,33,53,2,4,40,0,46,1,60,
 30,61,8,43,56,48,42,55,47,59,24,49,20,39,12,63,10,29,19,27,52,45,22,13,35,44,5,32,17,62 };

#define CONVERT_ENDIAN(x) (x)
#if ((1 >> 30) > 0)
#undef CONVERT_ENDIAN
#define CONVERT_ENDIAN(x) (63-(x))
#endif

/* values that are lower than the largest power of two inside the domain get permuted */
uint64_t hash(uint64_t key, uint64_t maxval, int maxbit, int inv) {
	assert(key <= maxval);
	uint64_t ret = 0, safeval = ((uint64_t) 1) << maxbit;
	if (key  < safeval) {
		/* below the largest power of two: permute the bits */
		int i, j=0;
		for(i=0; i<maxbit; i++,j++) {
			while (pos[j] >= maxbit) j++;
			ret |= (1-((key>>CONVERT_ENDIAN(inv?i:pos[j]))&1)) << CONVERT_ENDIAN(inv?pos[j]:i);
		}
	} else {
		/* higher values just get inverted (very poor mans hash)*/
		ret = safeval + (maxval - key);
	}
	assert(ret <= maxval);
	return ret;
}


#ifdef PHASH_STANDALONE
int main(int argc, char** argv) {
	long int key = atol(argv[1]);
	long int maxval = atol(argv[2]);
	int maxbit = log2(maxval);
	uint64_t h = hash(key,maxval,maxbit,0);
	return printf("hash(key=%lld,maxval=%lld, maxbit=%d) = %d (inv = %lld)\n",  key, maxval, maxbit, h, hash(h, maxval, maxbit, 1));
}
#else 
#include "../dss.h"

static uint16_t nations_map[25] = /* mapping between countries and their keys */
{15, 0, 5, 14, 16,	/* AFRICA		(MOROCCO | ALGERIA, ETHIOPIA, KENYA, MOZAMBIQUE) */
24, 1, 2, 3, 17, 	/* AMERICA		(UNITED STATES | ARGENTINA, BRAZIL, CANADA, PERU)*/
18, 9, 12, 21, 8, 	/* ASIA 		(CHINA | INDONESIA, JAPAN, VIETNAM, INDIA)*/
7, 6, 22, 19, 23, 	/* EUROPE 		(GERMANY | FRANCE, RUSSIA, ROMANIA, UNITED KINGDOM*/
4, 10, 11, 13, 20};	/* MIDDLE EAST 	(EGYPT | IRAN, IRAQ, JORDAN, SAUDI ARABIA)*/

uint64_t max_bit_tbl_part = 0;
uint64_t max_bit_tbl_supplier = 0;
uint64_t max_bit_tbl_partsupp = 0;
uint64_t max_bit_tbl_customer = 0;
uint64_t max_bit_tbl_nation = 0;
uint64_t max_bit_tbl_lineitem = 0;
uint64_t max_bit_tbl_region = 0;
uint64_t max_bit_tbl_orders = 0;

uint64_t customer_ranges[10];
uint64_t supplier_ranges[10];

void init_skew() {
	max_bit_tbl_part = (uint64_t) log2((double) (scale * tdefs[PART].base));
	max_bit_tbl_supplier = (uint64_t) log2((double) (scale * tdefs[SUPP].base));
	max_bit_tbl_partsupp = (uint64_t) log2((double) (scale * tdefs[PSUPP].base));
	max_bit_tbl_customer = (uint64_t) log2((double) (scale * tdefs[CUST].base));
	max_bit_tbl_nation = (uint64_t) log2((double) (scale * tdefs[NATION].base));
	max_bit_tbl_lineitem = (uint64_t) log2((double) (scale * tdefs[LINE].base));
	max_bit_tbl_region = (uint64_t) log2((double) (scale * tdefs[REGION].base));
	max_bit_tbl_orders = (uint64_t) log2((double) (scale * tdefs[ORDER].base));

	int j = 0;
	for (int i = 0; i < 5; i++) {
		customer_ranges[j++] = (i * tdefs[CUST].base*scale)/5;
		customer_ranges[j++] = customer_ranges[j - 1] + 3;

		supplier_ranges[j++] = (i * tdefs[SUPP].base*scale)/5;
		supplier_ranges[j++] = supplier_ranges[j - 1] + 3;
	}
	printf("Skew initialized\n");
}

int customer_hash_in_range(uint64_t customer_hash) {
	int i = 0;
	while (i < 10) {
		if ((customer_hash >= customer_ranges[i++]) &&
				(customer_hash <= customer_ranges[i++])) {
			return 1;
		}
	}

	return 0;
}

int supplier_hash_in_range(uint64_t supplier_hash) {
	int i = 0;
	while (i < 10) {
		if ((supplier_hash >= supplier_ranges[i++]) &&
				(supplier_hash <= supplier_ranges[i++])) {
			return 1;
		}
	}

	return 0;
}



uint16_t bin_nationkey(uint64_t key, uint64_t tbl_size) {
	uint32_t row = key / (0.2 * tbl_size);
	assert(row < 5);
	uint32_t offset = row * 0.2 * tbl_size;
	uint32_t major_tbl_part_size = 0.18 * tbl_size;
	uint32_t minor_tbl_part_size = 0.005 * tbl_size;
	uint32_t col = ((key - offset) <= major_tbl_part_size) ? 0 : (key - offset - major_tbl_part_size) / minor_tbl_part_size;
	assert(col < 5);

	uint32_t bin = row * 5 + col;

	return nations_map[bin];
}
#endif
