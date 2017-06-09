#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "phash.h"
#include "../dss.h"

static int pos[64] = /* numbers 0..63 in some random order */
{15,18,54,9,50,37,26,51,41,11,31,7,23,38,28,58,16,57,14,3,36,21,6,34,25,33,53,2,4,40,0,46,1,60,
 30,61,8,43,56,48,42,55,47,59,24,49,20,39,12,63,10,29,19,27,52,45,22,13,35,44,5,32,17,62 };

static uint16_t nations_map[25] = /* mapping between countries and their keys */
{15, 0, 5, 14, 16,	/* AFRICA		(MOROCCO | ALGERIA, ETHIOPIA, KENYA, MOZAMBIQUE) */
24, 1, 2, 3, 17, 	/* AMERICA		(UNITED STATES | ARGENTINA, BRAZIL, CANADA, PERU)*/
18, 9, 12, 21, 8, 	/* ASIA 		(CHINA | INDONESIA, JAPAN, VIETNAM, INDIA)*/
7, 6, 22, 19, 23, 	/* EUROPE 		(GERMANY | FRANCE, RUSSIA, ROMANIA, UNITED KINGDOM*/
4, 10, 11, 13, 20};	/* MIDDLE EAST 	(EGYPT | IRAN, IRAQ, JORDAN, SAUDI ARABIA)*/

uint64_t max_bit_tbl_part;
uint64_t max_bit_tbl_supplier;
uint64_t max_bit_tbl_partsupp;
uint64_t max_bit_tbl_customer;
uint64_t max_bit_tbl_nation;
uint64_t max_bit_tbl_lineitem;
uint64_t max_bit_tbl_region;
uint64_t max_bit_tbl_orders;

uint64_t customer_ranges[10];
uint64_t supplier_ranges[10];

void init_skew() {
	max_bit_tbl_part = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_PART));
	max_bit_tbl_supplier = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_SUPPLIER));
	max_bit_tbl_partsupp = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_PARTSUPP));
	max_bit_tbl_customer = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_CUSTOMER));
	max_bit_tbl_nation = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_NATION));
	max_bit_tbl_lineitem = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_LINEITEM));
	max_bit_tbl_region = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_REGION));
	max_bit_tbl_orders = (uint64_t) log2((double) (scale * DEFAULT_TBL_SIZE_ORDERS));

	int j = 0;
	for (int i = 0; i < 5; i++) {
		customer_ranges[j++] = 0.2 * i * DEFAULT_TBL_SIZE_CUSTOMER;
		customer_ranges[j++] = 0.2 * i * customer_ranges[j - 1] + 3;

		supplier_ranges[j++] = 0.2 * i * DEFAULT_TBL_SIZE_SUPPLIER;
		supplier_ranges[j++] = 0.2 * i * supplier_ranges[j - 1] + 3;
	}
}

int customer_hash_in_range(unsigned long customer_hash) {
	int i = 0;
	while (i < 10) {
		if ((customer_hash >= customer_ranges[i++]) &&
				(customer_hash <= customer_ranges[i++])) {
			return 1;
		}
	}

	return 0;
}

int supplier_hash_in_range(unsigned long supplier_hash) {
	int i = 0;
	while (i < 10) {
		if ((supplier_hash >= supplier_ranges[i++]) &&
				(supplier_hash <= supplier_ranges[i++])) {
			return 1;
		}
	}

	return 0;
}


#define CONVERT_ENDIAN(x) (x)
#if ((1 >> 30) > 0)
#undef CONVERT_ENDIAN
#define CONVERT_ENDIAN(x) (63-(x))
#endif

unsigned long hash(unsigned long key, int maxbit, int inv) {
        unsigned long ret = (key >> (maxbit-1))&1, j = 0;
	int i;
        assert(key < (((unsigned long) 1) << maxbit));
        for(i=0; i<maxbit; i++,j++) {
                while (pos[j] >= maxbit) j++;
                ret |= (1-((key>>CONVERT_ENDIAN(inv?i:pos[j]))&1)) << CONVERT_ENDIAN(inv?pos[j]:i);
        }
        return ret;
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

#ifdef PHASH_STANDALONE
int main(int argc, char** argv) {
	long int key = atol(argv[1]);
	int maxbit = atoi(argv[2]);
	unsigned long h = hash(key,maxbit,0);
	return printf("hash(key=%lld,maxbit=%d) = %d (inv = %lld)\n",  key, maxbit, h, hash(h, maxbit, 1));
}
#endif
