#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int pos[64] = /* numbers 0..63 in some random order */
{15,18,54,9,50,37,26,51,41,11,31,7,23,38,28,58,16,57,14,3,36,21,6,34,25,33,53,2,4,40,0,46,1,60,
 30,61,8,43,56,48,42,55,47,59,24,49,20,39,12,63,10,29,19,27,52,45,22,13,35,44,5,32,17,62 };

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

#ifdef PHASH_STANDALONE
int main(int argc, char** argv) {
	long int key = atol(argv[1]);
	int maxbit = atoi(argv[2]);
	unsigned long h = hash(key,maxbit,0);
	return printf("hash(key=%lld,maxbit=%d) = %d (inv = %lld)\n",  key, maxbit, h, hash(h, maxbit, 1));
}
#endif
