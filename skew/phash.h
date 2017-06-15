/*
 * phash.h
 *
 *  Created on: Jun 9, 2017
 *      Author: aca
 */

#ifndef SKEW_PHASH_H_
#define SKEW_PHASH_H_

#include <inttypes.h>

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
int customer_hash_in_range(uint64_t customer_hash);
int supplier_hash_in_range(uint64_t supplier_hash);
unsigned long hash(uint64_t key, uint64_t maxval, int maxbit, int invert);
uint16_t bin_nationkey(uint64_t key, uint64_t tbl_size);


#endif /* SKEW_PHASH_H_ */
