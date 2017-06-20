/*
 * $Id: build.c,v 1.5 2009/06/28 14:01:08 jms Exp $
 * 
 * Revision History =================== $Log: build.c,v $
 * Revision History =================== Revision 1.5  2009/06/28 14:01:08  jms
 * Revision History =================== bug fix for DOP
 * Revision History =================== Revision 1.4
 * 2005/10/28 02:56:22  jms add platform-specific printf formats to allow for
 * DSS_HUGE data type
 * 
 * Revision 1.3  2005/10/14 23:16:54  jms fix for answer set compliance
 * 
 * Revision 1.2  2005/01/03 20:08:58  jms change line terminations
 * 
 * Revision 1.1.1.1  2004/11/24 23:31:46  jms re-establish external server
 * 
 * Revision 1.3  2004/04/07 20:17:29  jms bug #58 (join fails between
 * order/lineitem)
 * 
 * Revision 1.2  2004/01/22 05:49:29  jms AIX porting (AIX 5.1)
 * 
 * Revision 1.1.1.1  2003/08/08 21:35:26  jms recreation after CVS crash
 * 
 * Revision 1.3  2003/08/08 21:35:26  jms first integration of rng64 for
 * o_custkey and l_partkey
 * 
 * Revision 1.2  2003/08/07 17:58:34  jms Convery RNG to 64bit space as
 * preparation for new large scale RNG
 * 
 * Revision 1.1.1.1  2003/04/03 18:54:21  jms initial checkin
 * 
 * 
 */
/* stuff related to the customer table */
#include <stdio.h>
#include <string.h>
#ifndef VMS
#include <sys/types.h>
#endif
#if defined(SUN)
#include <unistd.h>
#endif
#include <math.h>
#include <inttypes.h>

#include "dss.h"
#include "dsstypes.h"
#ifdef ADHOC
#include "adhoc.h"
extern adhoc_t  adhocs[];
#endif				/* ADHOC */
#include "rng64.h"

#include "skew/phash.h"
#include <assert.h>

#define LEAP_ADJ(yr, mnth)      \
((LEAP(yr) && (mnth) >= 2) ? 1 : 0)
#define JDAY_BASE       8035	/* start from 1/1/70 a la unix */
#define JMNTH_BASE      (-70 * 12)	/* start from 1/1/70 a la unix */
#define JDAY(date) ((date) - STARTDATE + JDAY_BASE + 1)
#define PART_SUPP_BRIDGE(tgt, p, s) \
    { \
    DSS_HUGE tot_scnt = tdefs[SUPP].base * scale; \
    tgt = (p + s *  (tot_scnt / SUPP_PER_PART +  \
	(long) ((p - 1) / tot_scnt))) % tot_scnt + 1; \
    }
#define V_STR(avg, sd, tgt)  a_rnd((int)(avg * V_STR_LOW),(int)(avg * V_STR_HGH), sd, tgt)
#define TEXT(avg, sd, tgt)  dbg_text(tgt, (int)(avg * V_STR_LOW),(int)(avg * V_STR_HGH), sd)
static void gen_phone PROTO((DSS_HUGE ind, char *target, long seed));

DSS_HUGE
rpb_routine(DSS_HUGE p)
{
	DSS_HUGE        price;

	price = 90000;
	price += (p / 10) % 20001;	/* limit contribution to $200 */
	price += (p % 1000) * 100;

	return (price);
}

static void
gen_phone(DSS_HUGE ind, char *target, long seed)
{
	DSS_HUGE        acode, exchg, number;

	RANDOM(acode, 100, 999, seed);
	RANDOM(exchg, 100, 999, seed);
	RANDOM(number, 1000, 9999, seed);

	sprintf(target, "%02d", (int) (10 + (ind % NATIONS_MAX)));
	sprintf(target + 3, "%03d", (int) acode);
	sprintf(target + 7, "%03d", (int) exchg);
	sprintf(target + 11, "%04d", (int) number);
	target[2] = target[6] = target[10] = '-';

	return;
}



long
mk_cust(DSS_HUGE n_cust, customer_t * c)
{
	DSS_HUGE        i;
	static int      bInit = 0;
	static char     szFormat[100];

	if (!bInit)
	{
		sprintf(szFormat, C_NAME_FMT, 9, HUGE_FORMAT + 1);
		bInit = 1;
	}
	c->custkey = n_cust;
	sprintf(c->name, szFormat, C_NAME_TAG, n_cust);
	V_STR(C_ADDR_LEN, C_ADDR_SD, c->address);
	c->alen = (int)strlen(c->address);
	RANDOM(i, 0, (nations.count - 1), C_NTRG_SD);
	c->nation_code = i;
	gen_phone(i, c->phone, (long) C_PHNE_SD);
	RANDOM(c->acctbal, C_ABAL_MIN, C_ABAL_MAX, C_ABAL_SD);
	pick_str(&c_mseg_set, C_MSEG_SD, c->mktsegment);
	TEXT(C_CMNT_LEN, C_CMNT_SD, c->comment);
	c->clen = (int)strlen(c->comment);
#if JCCH_SKEW
	if (JCCH_skew) {
		unsigned long custkey_hash = hash(c->custkey,  tdefs[CUST].base*scale, max_bit_tbl_customer, 0);
		int cust_nr = custkey_hash%(tdefs[CUST].base*scale/5);
		c->nation_code = bin_nationkey(custkey_hash, tdefs[CUST].base*scale);
		if (cust_nr < 4) { /* populous customer */
			c->phone[0] += 3;
			strcpy(c->mktsegment, "GOLD0MINE");
			c->mktsegment[4] += custkey_hash/(tdefs[CUST].base*scale/5); /* effectively the region */
		}
	}
#endif
	return (0);
}


/*
 * generate the numbered order and its associated lineitems
 */
void
mk_sparse(DSS_HUGE i, DSS_HUGE * ok, long seq)
{
	long            low_bits;

	*ok = i;
	low_bits = (long) (i & ((1 << SPARSE_KEEP) - 1));
	*ok = *ok >> SPARSE_KEEP;
	*ok = *ok << SPARSE_BITS;
	*ok += seq;
	*ok = *ok << SPARSE_KEEP;
	*ok += low_bits;


	return;
}


static char   **asc_date = NULL;

#if JCCH_SKEW
DSS_HUGE blackfriday[10] = { 0 };

/* partsupp has partkey determine suppkey - we guarantee in a,b,c diffrent suppkeys per partkey */
unsigned long partsupp_class_a(unsigned long partkey_hash) {
	unsigned long supp_r = partkey_hash % 5;
	unsigned long supp_x = (partkey_hash/5) % 4;
	unsigned long supp_h = supp_r * (tdefs[SUPP].base*scale/5) + supp_x;
	return hash(supp_h, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
}
unsigned long partsupp_class_b(unsigned long partkey_hash) {
	unsigned long supp_r = partkey_hash % 5;
	unsigned long supp_y = 4 + ((partkey_hash/20) % (tdefs[SUPP].base*scale/5 - 4));
	unsigned long supp_h = supp_r * (tdefs[SUPP].base*scale/5) + supp_y;
	return hash(supp_h, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
}
unsigned long partsupp_class_c(unsigned long partkey_hash) {
	unsigned long supp_r = partkey_hash % 5;
	unsigned long supp_r2 = (supp_r + 1 + ((partkey_hash/20) % 4)) % 5;
	unsigned long supp_z = (partkey_hash/80) % (tdefs[SUPP].base*scale/5);
	unsigned long supp_h = supp_r2 * (tdefs[SUPP].base*scale/5) + supp_z;
	return hash(supp_h, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
}

DSS_HUGE
mk_blackfriday(order_t *o) {
	int year = (o->odate[0]-'0')*1000 + (o->odate[1]-'0')*100 + (o->odate[2]-'0')*10 + (o->odate[3]-'0');
	DSS_HUGE tmp_date = blackfriday[year-1992];
	strcpy(o->odate, asc_date[tmp_date - STARTDATE]);
	return tmp_date;
}
#endif

long
mk_item(order_t * o, DSS_HUGE lcnt, DSS_HUGE tmp_date, int skewed) {
	DSS_HUGE        rprice;
	DSS_HUGE        s_date;
	DSS_HUGE        r_date;
	DSS_HUGE        c_date;
	char            tmp_str[2];
	long ocnt = 0;
	o->l[lcnt].okey = o->okey;;
	o->l[lcnt].lcnt = lcnt + 1;
	RANDOM(o->l[lcnt].quantity, L_QTY_MIN, L_QTY_MAX, L_QTY_SD);
	RANDOM(o->l[lcnt].discount, L_DCNT_MIN, L_DCNT_MAX, L_DCNT_SD);
	RANDOM(o->l[lcnt].tax, L_TAX_MIN, L_TAX_MAX, L_TAX_SD);
	pick_str(&l_instruct_set, L_SHIP_SD, o->l[lcnt].shipinstruct);
	pick_str(&l_smode_set, L_SMODE_SD, o->l[lcnt].shipmode);
	TEXT(L_CMNT_LEN, L_CMNT_SD, o->l[lcnt].comment);
	o->l[lcnt].clen = (int)strlen(o->l[lcnt].comment);
	rprice = rpb_routine(o->l[lcnt].partkey);
	o->l[lcnt].eprice = rprice * o->l[lcnt].quantity;

	o->totalprice +=
		((o->l[lcnt].eprice *
	     ((long) 100 - o->l[lcnt].discount)) / (long) PENNIES) *
		((long) 100 + o->l[lcnt].tax)
		/ (long) PENNIES;

	RANDOM(s_date, L_SDTE_MIN, L_SDTE_MAX, L_SDTE_SD);
	s_date += tmp_date;
	RANDOM(c_date, L_CDTE_MIN, L_CDTE_MAX, L_CDTE_SD);
	c_date += tmp_date;
	RANDOM(r_date, L_RDTE_MIN, L_RDTE_MAX, L_RDTE_SD);
	r_date += s_date;

	strcpy(o->l[lcnt].sdate, asc_date[s_date - STARTDATE]);
	strcpy(o->l[lcnt].cdate, asc_date[c_date - STARTDATE]);
	strcpy(o->l[lcnt].rdate, asc_date[r_date - STARTDATE]);


	if (julian(r_date) <= CURRENTDATE)
	{
		pick_str(&l_rflag_set, L_RFLG_SD, tmp_str);
		o->l[lcnt].rflag[0] = *tmp_str;
	}
	else
		o->l[lcnt].rflag[0] = 'N';

	if (julian(s_date) <= CURRENTDATE)
	{
		ocnt++;
		o->l[lcnt].lstatus[0] = 'F';
	}
	else
		o->l[lcnt].lstatus[0] = 'O';

	if (skewed) {
		o->l[lcnt].rflag[0] = 'G';
		o->l[lcnt].lstatus[0] = 'G';
		strcpy(o->l[lcnt].shipmode, "GOLD AIR");
	}
	return ocnt;
}

long
mk_order(DSS_HUGE index, order_t * o, long upd_num)
{
	DSS_HUGE        lcnt = 0;
	long            ocnt;
	DSS_HUGE        tmp_date;
	DSS_HUGE        clk_num;
	DSS_HUGE        supp_num;
	char          **mk_ascdate PROTO((void));
	int             delta = 1;
	static int      bInit = 0;
	static char     szFormat[100];
#if JCCH_SKEW
	unsigned long orderkey_hash = hash(index, tdefs[ORDER].base * scale, max_bit_tbl_orders, 0);
	int cust_region = orderkey_hash % 5;
#endif

	if (!bInit)
	{
		sprintf(szFormat, O_CLRK_FMT, 9, HUGE_FORMAT + 1);
		bInit = 1;
	}
	if (asc_date == NULL)
		asc_date = mk_ascdate();
	mk_sparse(index, &o->okey,
		  (upd_num == 0) ? 0 : 1 + upd_num / (10000 / UPD_PCT));

	if (scale >= 30000)
		RANDOM64(o->custkey, O_CKEY_MIN, O_CKEY_MAX, O_CKEY_SD);
	else
		RANDOM(o->custkey, O_CKEY_MIN, O_CKEY_MAX, O_CKEY_SD);
	while (o->custkey % CUST_MORTALITY == 0)
	{
		o->custkey += delta;
		o->custkey = MIN(o->custkey, O_CKEY_MAX);
		delta *= -1;
	}

	pick_str(&o_priority_set, O_PRIO_SD, o->opriority);
	RANDOM(clk_num, 1, MAX((scale * O_CLRK_SCL), O_CLRK_SCL), O_CLRK_SD);
	sprintf(o->clerk, szFormat, O_CLRK_TAG, clk_num);
	TEXT(O_CMNT_LEN, O_CMNT_SD, o->comment);
	o->clen = (int)strlen(o->comment);
	RANDOM(tmp_date, O_ODATE_MIN, O_ODATE_MAX, O_ODATE_SD);
	strcpy(o->odate, asc_date[tmp_date - STARTDATE]);
#if JCCH_SKEW
	/* override custkey and mess up up comment */
	if (JCCH_skew) { 
		if (((orderkey_hash/20) % 4) == 0) {
			/* 25% are order from a populous customer (makes sure it is from a populous nation.) */
			int cust_nr = (orderkey_hash/5) % 4;
			char *s0, *s1, *s2, *s3, *s4;
			o->custkey = (cust_region * tdefs[CUST].base * scale / 5) + cust_nr;
			o->custkey = hash(o->custkey, tdefs[CUST].base * scale, max_bit_tbl_customer, 1); 
			assert((o->custkey > 0) && (o->custkey <= tdefs[CUST].base * scale));

			/* now replace the target strings for Q13 */
			s1 = strstr(o->comment, "special");
			s2 = strstr(o->comment, "pending");
			s3 = strstr(o->comment, "unusual");
			s4 = strstr(o->comment, "express");
			s0 = s1;
			if (s2 && (s0 == NULL || s2 < s0))  s0 = s2;
			if (s3 && (s0 == NULL || s3 < s0))  s0 = s3;
			if (s4 && (s0 == NULL || s4 < s0))  s0 = s4;
			if (s0) {
				s1 = strstr(s0+7, "packages");
				s2 = strstr(s0+7, "requests");
				s3 = strstr(s0+7, "accounts");
				s4 = strstr(s0+7, "deposits");
				if (s2 && (s1 == NULL || s2 < s1))  s1 = s2;
				if (s3 && (s1 == NULL || s3 < s1))  s1 = s3;
				if (s4 && (s1 == NULL || s4 < s1))  s1 = s4;
				if (s1) {
					strcpy(s0, "1gold2");
					s0[4] = '0' + ((cust_region*5+cust_nr)/10);
					s0[5] = '0' + ((cust_region*5+cust_nr)%10);
					strcpy(s1, "3mine4");
				}
			}
		} else {
			/* let custkey be determined by orderkey (handy later) */
			o->custkey = (orderkey_hash/5) % (tdefs[CUST].base*scale/5 - (tdefs[CUST].base*scale/5)/CUST_MORTALITY);
			/* rather than just using the lowest 2/3 of custkeys, we scale up by multiplying with 3/2, leaving 1/3 holes */
			o->custkey = ((o->custkey*CUST_MORTALITY)/(CUST_MORTALITY-1)) % (tdefs[CUST].base*scale/5);
			/* make it come from the right region */
			o->custkey += cust_region*(tdefs[CUST].base*scale/5);
		}
		if (((index * 17) % 8) < 3) { /* it's... Black Friday! for 25% of the orders (3/8 * 2/3) */
			int month = (o->odate[5]-'0')*10 + (o->odate[6]-'0');
			if (month < 5 || month > 8) { /* move orders from 8 from the 12 months = 2/3 of them, to black friday */ 
				tmp_date = mk_blackfriday(o);
			}
		}
	}
#endif				/* DEBUG */
#ifdef DEBUG
	if (o->clen > O_CMNT_MAX)
		fprintf(stderr, "comment error: O%d\n", index);
#endif
	o->spriority = 0;

	o->totalprice = 0;
	o->orderstatus = 'O';
	ocnt = 0;

	RANDOM(o->lines, O_LCNT_MIN, O_LCNT_MAX, O_LCNT_SD);
#if JCCH_SKEW
	if (JCCH_skew && upd_num == 0 && orderkey_hash < 20) {  // populous order 
		unsigned long i, p, partkey_hash;
		for (partkey_hash = 0; partkey_hash < tdefs[PART].base * scale; partkey_hash++) {
 			p = hash(partkey_hash, tdefs[PART].base * scale, max_bit_tbl_part, 1);
			if (partkey_hash < 20) {
				for(i = 0; i < (tdefs[SUPP].base*scale)/5; i++) {
					o->l[lcnt].partkey = p;
					o->l[lcnt].suppkey = hash(i+cust_region*((tdefs[SUPP].base*scale)/5), tdefs[SUPP].base*scale, max_bit_tbl_supplier, 1);
					ocnt += mk_item(o, lcnt++, tmp_date, 1);
					if (lcnt >= MAX_L_PER_O) break;
				}
			} else if (cust_region == (partkey_hash % 5)) { /* generally matching region */
				o->l[lcnt].partkey = p;
				o->l[lcnt].suppkey = partsupp_class_a(partkey_hash);
				ocnt += mk_item(o, lcnt++, tmp_date, 1);

				if (lcnt < MAX_L_PER_O) {
					if (((partkey_hash/20) % 8) >= 3) continue;

					o->l[lcnt].partkey = p;
					o->l[lcnt].suppkey = partsupp_class_b(partkey_hash);
					ocnt += mk_item(o, lcnt++, tmp_date, 1);
				}
			}
			if (lcnt >= MAX_L_PER_O) break;
		}
		tmp_date = mk_blackfriday(o);
		o->totalprice = 0; /* there would be overflow, anyway.. */
		strcpy(o->comment, "1mine2 3gold4"); /* Q13 */
		o->clen = strlen(o->comment);
		o->lines = MAX_L_PER_O;
	} else if (JCCH_skew && upd_num == 0) {
		o->lines = (index <= 3*20)?4:3;
	}
#endif
	while (lcnt < o->lines) {
		if (scale >= 30000)
			RANDOM64(o->l[lcnt].partkey, L_PKEY_MIN, L_PKEY_MAX, L_PKEY_SD);
		else
			RANDOM(o->l[lcnt].partkey, L_PKEY_MIN, L_PKEY_MAX, L_PKEY_SD);
		RANDOM(supp_num, 0, 3, L_SKEY_SD);
		PART_SUPP_BRIDGE(o->l[lcnt].suppkey, o->l[lcnt].partkey, supp_num);
#if JCCH_SKEW
		if (JCCH_skew) {
			unsigned long partkey_hash = hash(o->l[lcnt].partkey, tdefs[PART].base * scale, max_bit_tbl_part, 0);
			o->l[lcnt].suppkey = ((orderkey_hash/20) % 10)?
				partsupp_class_c(partkey_hash): /* 90% non-matching region */
				partsupp_class_b(partkey_hash); /* 10% matching region */
		}
#endif
		ocnt += mk_item(o, lcnt++, tmp_date, 0);
	}
	if (ocnt > 0)
		o->orderstatus = 'P';
	if (ocnt == o->lines)
		o->orderstatus = 'F';


	return (0);
}

long
mk_part(DSS_HUGE index, part_t * p)
{
	DSS_HUGE        suppcnt = SUPP_PER_PART;
	DSS_HUGE        temp;
	long            snum;
	DSS_HUGE        brnd;
	static int      bInit = 0;
	static char     szFormat[100];
	static char     szBrandFormat[100];
#if JCCH_SKEW
	unsigned long partkey_hash = hash(index, tdefs[PART].base*scale, max_bit_tbl_part, 0);
	p->suppcnt = suppcnt;
#endif

	if (!bInit)
	{
		sprintf(szFormat, P_MFG_FMT, 1, HUGE_FORMAT + 1);
		sprintf(szBrandFormat, P_BRND_FMT, 2, HUGE_FORMAT + 1);
		bInit = 1;
	}
	p->partkey = index;
	agg_str(&colors, (long) P_NAME_SCL, (long) P_NAME_SD, p->name);
	RANDOM(temp, P_MFG_MIN, P_MFG_MAX, P_MFG_SD);
	sprintf(p->mfgr, szFormat, P_MFG_TAG, temp);
	RANDOM(brnd, P_BRND_MIN, P_BRND_MAX, P_BRND_SD);
	sprintf(p->brand, szBrandFormat, P_BRND_TAG, (temp * 10 + brnd));
	p->tlen = pick_str(&p_types_set, P_TYPE_SD, p->type);
	p->tlen = (int)strlen(p_types_set.list[p->tlen].text);
	RANDOM(p->size, P_SIZE_MIN, P_SIZE_MAX, P_SIZE_SD);
	pick_str(&p_cntr_set, P_CNTR_SD, p->container);
	p->retailprice = rpb_routine(index);
	TEXT(P_CMNT_LEN, P_CMNT_SD, p->comment);
	p->clen = (int)strlen(p->comment);
#if JCCH_SKEW
	if (JCCH_skew) {  
		if ((partkey_hash >= 0) && (partkey_hash < 20)) {
			sprintf(p->brand, szBrandFormat, P_BRND_TAG, 0);
			sprintf(p->name, "%s", "shiny nicely mined gold");
			sprintf(p->type, "%s", "NICE SHINY MINED GOLD");
			sprintf(p->container, "%s", "GOLD CAGE");
			p->size = 51;
			p->tlen = strlen(p->type);
			p->suppcnt = suppcnt = tdefs[SUPP].base * scale;
			for (snum = 0; snum < suppcnt; snum++) {
				p->s[snum].partkey = p->partkey;
				p->s[snum].qty = 4000000 * scale;
				p->s[snum].suppkey = hash(snum, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);

				RANDOM(p->s[snum].scost, PS_SCST_MIN, PS_SCST_MAX, PS_SCST_SD);
				TEXT(PS_CMNT_LEN, PS_CMNT_SD, p->s[snum].comment);
				p->s[snum].clen = (int)strlen(p->s[snum].comment);
			}
			return 0;
		}	 
		p->suppcnt = suppcnt = 
		(index <= (4*tdefs[PSUPP].base*scale - (20*tdefs[SUPP].base*scale + 3*(tdefs[PART].base*scale - 20))))?4:3;
	}
#endif
	for (snum = 0; snum < suppcnt; snum++)
	{
		p->s[snum].partkey = p->partkey;
		PART_SUPP_BRIDGE(p->s[snum].suppkey, index, snum);
		RANDOM(p->s[snum].qty, PS_QTY_MIN, PS_QTY_MAX, PS_QTY_SD);
		RANDOM(p->s[snum].scost, PS_SCST_MIN, PS_SCST_MAX, PS_SCST_SD);
		TEXT(PS_CMNT_LEN, PS_CMNT_SD, p->s[snum].comment);
		p->s[snum].clen = (int)strlen(p->s[snum].comment);
	}
#if JCCH_SKEW
	if (JCCH_skew) {
		// select a populous supplier
		partkey_hash = hash(p->partkey, tdefs[PART].base * scale, max_bit_tbl_part, 0);
		p->s[0].suppkey = partsupp_class_a(partkey_hash);
		p->s[1].suppkey = partsupp_class_b(partkey_hash);
		p->s[2].suppkey = partsupp_class_c(partkey_hash);
	
		/* part,supp must be unique. the class_a,b,c guarantee this among each other. 
       		 * when we generate a fourth partsupp, we pick a populous supplier, but then manually eliminate duplicates
        	 */
		if (suppcnt == 4) {
			DSS_HUGE suppkey_hash = 1;
			DSS_HUGE suppkey = hash(suppkey_hash, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
			if (p->s[0].suppkey == suppkey) {
				suppkey_hash = 2;
				suppkey = hash(suppkey_hash, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
			}
			if (p->s[1].suppkey == suppkey) {
				suppkey_hash = 3;
				suppkey = hash(suppkey_hash, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
			}
			if (p->s[2].suppkey == suppkey) {
				suppkey_hash = 4;
				suppkey = hash(suppkey_hash, tdefs[SUPP].base * scale, max_bit_tbl_supplier, 1);
			}
			p->s[3].suppkey = suppkey;
		}
	}
#endif
	return (0);
}

long
mk_supp(DSS_HUGE index, supplier_t * s)
{
	DSS_HUGE        i, bad_press, noise, offset, type;
	static int      bInit = 0;
	static char     szFormat[100];

	if (!bInit)
	{
		sprintf(szFormat, S_NAME_FMT, 9, HUGE_FORMAT + 1);
		bInit = 1;
	}
	s->suppkey = index;
	sprintf(s->name, szFormat, S_NAME_TAG, index);
	V_STR(S_ADDR_LEN, S_ADDR_SD, s->address);
	s->alen = (int)strlen(s->address);
	RANDOM(i, 0, nations.count - 1, S_NTRG_SD);
	s->nation_code = i;
	gen_phone(i, s->phone, S_PHNE_SD);
	RANDOM(s->acctbal, S_ABAL_MIN, S_ABAL_MAX, S_ABAL_SD);
#if JCCH_SKEW
	if (JCCH_skew) {
		unsigned long suppkey_hash = hash(s->suppkey,  tdefs[SUPP].base*scale, max_bit_tbl_supplier, 0);
		s->nation_code = bin_nationkey(suppkey_hash, tdefs[SUPP].base*scale);
		if ((suppkey_hash%(tdefs[SUPP].base*scale/5)) < 4) {
			s->comment[0] = 0;
			s->clen = (int)strlen(s->comment);
			return (0);
		} 
	}
#endif
	TEXT(S_CMNT_LEN, S_CMNT_SD, s->comment);
	s->clen = (int)strlen(s->comment);
	/*
	 * these calls should really move inside the if stmt below, but this
	 * will simplify seedless parallel load
	 */
	RANDOM(bad_press, 1, 10000, BBB_CMNT_SD);
	RANDOM(type, 0, 100, BBB_TYPE_SD);
	RANDOM(noise, 0, (s->clen - BBB_CMNT_LEN), BBB_JNK_SD);
	RANDOM(offset, 0, (s->clen - (BBB_CMNT_LEN + noise)),
	       BBB_OFFSET_SD);
	if (bad_press <= S_CMNT_BBB)
	{
		type = (type < BBB_DEADBEATS) ? 0 : 1;
		memcpy(s->comment + offset, BBB_BASE, BBB_BASE_LEN);
		if (type == 0)
			memcpy(s->comment + BBB_BASE_LEN + offset + noise,
			       BBB_COMPLAIN, BBB_TYPE_LEN);
		else
			memcpy(s->comment + BBB_BASE_LEN + offset + noise,
			       BBB_COMMEND, BBB_TYPE_LEN);
	}
	return (0);
}

struct
{
	char           *mdes;
	long            days;
	long            dcnt;
}               months[] =

{
	{
		NULL, 0, 0
	},
	{
		"JAN", 31, 31
	},
	{
		"FEB", 28, 59
	},
	{
		"MAR", 31, 90
	},
	{
		"APR", 30, 120
	},
	{
		"MAY", 31, 151
	},
	{
		"JUN", 30, 181
	},
	{
		"JUL", 31, 212
	},
	{
		"AUG", 31, 243
	},
	{
		"SEP", 30, 273
	},
	{
		"OCT", 31, 304
	},
	{
		"NOV", 30, 334
	},
	{
		"DEC", 31, 365
	}
};

long
mk_time(DSS_HUGE index, dss_time_t * t)
{
	long            m = 0;
	long            y;
	long            d;

	t->timekey = index + JDAY_BASE;
	y = julian(index + STARTDATE - 1) / 1000;
	d = julian(index + STARTDATE - 1) % 1000;
	while (d > months[m].dcnt + LEAP_ADJ(y, m))
		m++;
	PR_DATE(t->alpha, y, m,
		d - months[m - 1].dcnt - ((LEAP(y) && m > 2) ? 1 : 0));
	t->year = 1900 + y;
	t->month = m + 12 * y + JMNTH_BASE;
	t->week = (d + T_START_DAY - 1) / 7 + 1;
	t->day = d - months[m - 1].dcnt - LEAP_ADJ(y, m - 1);
#ifdef JCCH_SKEW
	/* just remember black fridays here -- here it falls always on Memorial Day  */
	if (blackfriday[t->year-1992] == 0 && m == 5 && t->day == 28) 
		blackfriday[t->year-1992] = index + STARTDATE - 1;
#endif
	return (0);
}

int
mk_nation(DSS_HUGE index, code_t * c)
{
	c->code = index - 1;
	c->text = nations.list[index - 1].text;
	c->join = nations.list[index - 1].weight;

//	printf("nation code = %"PRId64"\n", c->code);
//	printf("nation text = %s\n", c->text);
//	printf("nation join = %ld\n", c->join);

	TEXT(N_CMNT_LEN, N_CMNT_SD, c->comment);
	c->clen = (int)strlen(c->comment);
	return (0);
}

int
mk_region(DSS_HUGE index, code_t * c)
{

	c->code = index - 1;
	c->text = regions.list[index - 1].text;
	c->join = 0;		/* for completeness */
	TEXT(R_CMNT_LEN, R_CMNT_SD, c->comment);
	c->clen = (int)strlen(c->comment);
	return (0);
}
