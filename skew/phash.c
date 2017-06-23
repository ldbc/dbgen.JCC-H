#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "phash.h"

#define PRIME 1442968193

char* skew_regions[5] = {
        "AFRICA", "AMERICA", "ASIA", "EUROPE", "MIDDLE EAST"
};
char* skew_nations[25] = {
        "MOROCCO", "ALGERIA", "ETHIOPIA", "KENYA", "MOZAMBIQUE",
	"UNITED STATES", "ARGENTINA", "BRAZIL", "CANADA", "PERU",
	"CHINA", "INDONESIA", "JAPAN", "VIETNAM", "INDIA",
	"GERMANY", "FRANCE", "RUSSIA", "ROMANIA", "UNITED KINGDOM",
	"EGYPT", "IRAN", "IRAQ", "JORDAN", "SAUDI ARABIA"
};


/* bijective permutation functions based on a linear permutation polynomial */
#define PHASH(x,p)     (((x)*PRIME + (p)->xorval)%(p)->maxval) 
#define PHASH_INV(x,p) ((((x) + (p)->maxval - (p)->xorval)*(p)->invval)%(p)->maxval)

uint64_t phash(uint64_t key, phash_t *p, int inv) {
	/* in case of TPC-H, keys start at 1, whereas hashes start at 0 */
	if (inv) return PHASH_INV(key, p) + 1;
	return PHASH(key - 1, p);
}
void init_phash(phash_t *phash, unsigned long maxval) {
	unsigned long i;
	for(i=3; i < (1UL<<48); i++) {
		if ((PRIME * i) % maxval == 1) {
			phash->maxval = maxval;
			phash->invval = i;

			/* random bits that are only below the most significant bit of maxval */
			for(i=1; 2*i<maxval; i*=2);
			phash->xorval = 792606555396977 & (i-1); 
			return;
		}
	}
	fprintf(stderr, "init_skew_failed: could not find inverse for %lld ring of  %lld\n", maxval, PRIME);
	exit(-1);
}


#ifdef PHASH_STANDALONE
int main(int argc, char** argv) {
	long int h, key = atol(argv[1]);
	long int maxval = atol(argv[2]);
	phash_t ph;
	init_phash(&ph, maxval);
	h = phash(key,&ph,0);
	return printf("phash(key=%lld) = %d (inv = %lld)\n",  key, h, phash(h, &ph, 1));
}
#else 
#include "../dss.h"

static uint16_t nations_map[25] = /* mapping between countries and their keys */
{15, 0, 5, 14, 16,	/* AFRICA		(MOROCCO | ALGERIA, ETHIOPIA, KENYA, MOZAMBIQUE) */
24, 1, 2, 3, 17, 	/* AMERICA		(UNITED STATES | ARGENTINA, BRAZIL, CANADA, PERU)*/
18, 9, 12, 21, 8, 	/* ASIA 		(CHINA | INDONESIA, JAPAN, VIETNAM, INDIA)*/
7, 6, 22, 19, 23, 	/* EUROPE 		(GERMANY | FRANCE, RUSSIA, ROMANIA, UNITED KINGDOM*/
4, 10, 11, 13, 20};	/* MIDDLE EAST 	(EGYPT | IRAN, IRAQ, JORDAN, SAUDI ARABIA)*/


phash_t phash_part = { 0 };
phash_t phash_supplier = { 0 };
phash_t phash_customer = { 0 };
phash_t phash_orders = { 0 };

void init_skew() {
	init_phash(&phash_part, scale * tdefs[PART].base);
	init_phash(&phash_supplier, scale * tdefs[SUPP].base);
	init_phash(&phash_customer, scale * tdefs[CUST].base);
	init_phash(&phash_orders, scale * tdefs[ORDER].base);
}

uint16_t bin_nationkey(uint64_t key, uint64_t tbl_size) {
	long row = key / (0.2 * tbl_size);
	long bin = row * 5;
	long offset = key - (0.18 + row * 0.2) * tbl_size;
	assert(row < 5);
	if (offset > 0 && 0.02 * tbl_size > 0) { 
		offset = (4*offset) / (0.02*tbl_size);
		assert(offset < 4);
		bin += 1 + offset;
	}
	return nations_map[bin];
}
#endif
