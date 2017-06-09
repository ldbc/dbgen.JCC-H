/*
 * phash.h
 *
 *  Created on: Jun 9, 2017
 *      Author: aca
 */

#ifndef SKEW_PHASH_H_
#define SKEW_PHASH_H_

#include <inttypes.h>

#define DEFAULT_TBL_SIZE_PART		200000
#define DEFAULT_TBL_SIZE_SUPPLIER	10000
#define DEFAULT_TBL_SIZE_PARTSUPP	800000
#define DEFAULT_TBL_SIZE_CUSTOMER	150000
#define DEFAULT_TBL_SIZE_NATION		25
#define DEFAULT_TBL_SIZE_LINEITEM	6000000
#define DEFAULT_TBL_SIZE_REGION		5
#define DEFAULT_TBL_SIZE_ORDERS		1500000

extern uint64_t max_bit_tbl_part;
extern uint64_t max_bit_tbl_supplier;
extern uint64_t max_bit_tbl_partsupp;
extern uint64_t max_bit_tbl_customer;
extern uint64_t max_bit_tbl_nation;
extern uint64_t max_bit_tbl_lineitem;
extern uint64_t max_bit_tbl_region;
extern uint64_t max_bit_tbl_orders;

extern uint64_t customer_ranges[10];
extern uint64_t supplier_ranges[10];

void init_skew();
int customer_hash_in_range(unsigned long customer_hash);
int supplier_hash_in_range(unsigned long supplier_hash);
unsigned long hash(unsigned long key, int maxbit, int inv);
uint16_t bin_nationkey(uint64_t key, uint64_t tbl_size);


#endif /* SKEW_PHASH_H_ */
