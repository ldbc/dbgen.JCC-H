/*
* $Id: varsub.c,v 1.9 2008/03/20 16:38:34 jms Exp $
*
* Revision History
* ===================
* $Log: varsub.c,v $
* Revision 1.9  2008/03/20 16:38:34  jms
* q14/q15: range correction
*
* Revision 1.8  2008/03/20 16:36:47  jms
* q14/15 format change
*
* Revision 1.7  2006/05/31 22:25:21  jms
* Rework UnifInt calls in varsub to handle lack of PROTO defn in windows
*
* Revision 1.6  2006/05/25 22:30:44  jms
* qgen porting for 32b/64b
*
* Revision 1.5  2006/05/25 16:08:52  jms
* Rework UnifInt call for query 3
*
* Revision 1.4  2006/04/26 23:20:05  jms
* Data type clenaup for qgen
*
* Revision 1.3  2005/11/03 14:50:44  jms
* solaris porting changes
*
* Revision 1.2  2005/01/03 20:08:59  jms
* change line terminations
*
* Revision 1.1.1.1  2004/11/24 23:31:47  jms
* re-establish external server
*
* Revision 1.1.1.1  2003/04/03 18:54:21  jms
* recreation after CVS crash
*
* Revision 1.1.1.1  2003/04/03 18:54:21  jms
* initial checkin
*
*
*/
#include <stdio.h>
#ifndef _POSIX_SOURCE
#include <malloc.h>
#endif /* POSIX_SOURCE */
#if (defined(_POSIX_)||!defined(WIN32))
#include <unistd.h>
#endif /* WIN32 */
#include <string.h>
#include "config.h"
#include "dss.h"
#include "tpcd.h"
#ifdef ADHOC
#include "adhoc.h"
extern adhoc_t adhocs[];
#endif /* ADHOC */
void	permute(long *a, int c, long s);
#define MAX_PARAM	10		/* maximum number of parameter substitutions in a query */

extern long Seed[];
extern char **asc_date;
extern double flt_scale;
extern distribution q13a, q13b;

#ifdef JCCH_SKEW
#include "skew/phash.h"
#endif

long brands[25] = {11,12,13,14,15,21,22,23,24,25,31,32,33,34,35,
			41,42,43,44,45,51,52,53,54,55};
long sizes[50] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
				21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
				41,42,43,44,45,46,47,48,49,50};
long ccode[25] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
char *defaults[24][11] =
{
    {"90",              NULL,                   NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 1 */
    {"15",              "BRASS",                "EUROPE",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 2 */
    {"BUILDING",        "1995-03-15",           NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 3 */
    {"1993-07-01",      NULL,                   NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 4 */
    {"ASIA",            "1994-01-01",           NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 5 */
    {"1994-01-01",      ".06",                  "24",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 6 */
    {"FRANCE",          "GERMANY",              NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 7 */
    {"BRAZIL",          "AMERICA",      "ECONOMY ANODIZED STEEL",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},/* 8 */
    {"green",         NULL,                   NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 9 */
    {"1993-10-01",      NULL,                   NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 10 */
    {"GERMANY",         "0.0001",                 NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 11 */
    {"MAIL",            "SHIP",                 "1994-01-01",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 12 */
    {"special", "requests",                   NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 13 */
    {"1995-09-01",      NULL,                   NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 14 */
    {"1996-01-01",      NULL,                   NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 15 */
    {"Brand#45",        "MEDIUM POLISHED", "49",
	"14","23","45","19","3","36","9", NULL}, /* 16 */
    {"Brand#23",        "MED BOX",               NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 17 */
    {"300", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 18 */
    {"Brand#12", "Brand#23", "Brand#34", "1", "10", "20", NULL, NULL, NULL, NULL, NULL}, /* 19 */
    {"forest", "1994-01-01", "CANADA", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 20 */
    {"SAUDI ARABIA", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* 21 */
    {"13","31","23", "29", "30", "18", "17", NULL, NULL, NULL, NULL},  /* 22 */
    {NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* UF1 */
    {NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},  /* UF2 */
};
void
varsub(int qnum, int vnum, int flags)
{
    static char param[11][128];
    static char formats[23][128];
    static FILE *lfp = NULL;
    static int bInit = 0;
    long *lptr;
    char *ptr;
    int i = 0;
    DSS_HUGE tmp_date, tmp1, tmp2;
	
    if (!bInit)
    {
        sprintf(formats[4], "19%s-%s-01", HUGE_DATE_FORMAT, HUGE_DATE_FORMAT);
        sprintf(formats[5], "19%s-01-01", HUGE_DATE_FORMAT);
        sprintf(formats[6], "19%s-01-01", HUGE_DATE_FORMAT);
        sprintf(formats[7], "0.%s", HUGE_DATE_FORMAT); /* used by q6 */
        sprintf(formats[10], "19%s-%s-01", HUGE_DATE_FORMAT, HUGE_DATE_FORMAT);
        sprintf(formats[12], "19%s-01-01", HUGE_DATE_FORMAT);
        sprintf(formats[14], "19%s-%s-01", HUGE_DATE_FORMAT, HUGE_DATE_FORMAT);
        sprintf(formats[15], "19%s-%s-01", HUGE_DATE_FORMAT, HUGE_DATE_FORMAT);
	     sprintf(formats[16], "Brand#%s%s", HUGE_FORMAT, HUGE_FORMAT);
	     sprintf(formats[17], "Brand#%s%s", HUGE_FORMAT, HUGE_FORMAT);
	     sprintf(formats[19], "Brand#%s%s", HUGE_FORMAT, HUGE_FORMAT);
        sprintf(formats[20], "19%s-01-01", HUGE_DATE_FORMAT);
        bInit = 1;
    }

    if (vnum == 0)
    {
		if ((flags & DFLT) == 0)
		{
			switch(qnum)
			{
			case 1:
				sprintf(param[1], HUGE_FORMAT, UnifInt((DSS_HUGE)60,(DSS_HUGE)120,qnum));
				param[2][0] = '\0';
				break;
			case 2:
				sprintf(param[1], HUGE_FORMAT,
					UnifInt((DSS_HUGE)P_SIZE_MIN, (DSS_HUGE)P_SIZE_MAX, qnum));
				pick_str(&p_types_set, qnum, param[3]);
				ptr = param[3] + (int)strlen(param[3]);
				while (*(ptr - 1) != ' ') ptr--;
				strcpy(param[2], ptr);
				pick_str(&regions, qnum, param[3]);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					strcpy(param[1], "1");
					strcpy(param[2], "SHINY MINED GOLD" + UnifInt(0, 10, qnum));
				} 
#endif
				param[4][0] = '\0';
				break;
			case 3:
				pick_str(&c_mseg_set, qnum, param[1]);
				/*
				* pick a random offset within the month of march and add the
				* appropriate magic numbers to position the output functions 
				* at the start of March '95
				*/
            RANDOM(tmp_date, 0, 30, qnum);
				strcpy(param[2], *(asc_date + tmp_date + 1155));
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					/* go 1993 shortly after black friday instead */
					strcpy(param[2], *(asc_date + tmp_date + 366));
					param[2][5] = '0'; param[2][6] = '5';
					param[2][8] = '2' + (tmp_date >= 10); 
					param[2][9] = '0' + ((9+(tmp_date/10)) % 10);
				}
#endif
				param[3][0] = '\0';
				break;
			case 4:
				tmp_date = UnifInt((DSS_HUGE)0,(DSS_HUGE)57,qnum);
				sprintf(param[1],formats[4],
					93 + tmp_date/12, tmp_date%12 + 1);
#ifdef JCCH_SKEW
				tmp_date = UnifInt((DSS_HUGE)0,(DSS_HUGE)7,qnum);
				if (JCCH_skew) { 
					int year = 92 + (tmp_date>>1);
					sprintf(param[1],formats[4],
						(year==95)?97:year, 5+(tmp_date&1)); /* no black friday */
				} else {
					/* in 1995-1996 there are no populous orders */
					sprintf(param[1],formats[4],
						95 + (tmp_date>>2), ((tmp_date&2)?9:1) + (tmp_date&1)); /* no black friday */
				}
#endif
				param[2][0] = '\0';
				break;
			case 5:
				pick_str(&regions, qnum, param[1]);
#ifdef JCCH_SKEW
				if (JCCH_skew) { 
					tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)94,qnum);
				} else { /* in 1995-1996 there are no populous orders */
					tmp_date = UnifInt((DSS_HUGE)95,(DSS_HUGE)96,qnum);
				}
#else
				tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)97,qnum);
#endif
				sprintf(param[2], formats[5], tmp_date);
				param[3][0] = '\0';
				break;
			case 6:
#ifdef JCCH_SKEW
				if (JCCH_skew) { 
					tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)94,qnum);
				} else { /* in 1995-1996 there are no populous orders */
					tmp_date = UnifInt((DSS_HUGE)95,(DSS_HUGE)96,qnum);
				}
#else
				tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)97,qnum);
#endif
				sprintf(param[1], formats[6], tmp_date);
				sprintf(param[2], formats[7], 
                                    UnifInt((DSS_HUGE)2, (DSS_HUGE)9, qnum));
				sprintf(param[3], HUGE_FORMAT, UnifInt((DSS_HUGE)24, (DSS_HUGE)25, qnum));
#ifdef JCCH_SKEW
				if (JCCH_skew) strcpy(param[3], "100");
#endif
				param[4][0] = '\0';
				break;
			case 7:
				/* previsously, these two were hardcoded, leave them the same by default */
				strcpy(param[3], "1995-01-01");
				strcpy(param[4], "1996-12-31");
#ifdef JCCH_SKEW
				/* query is between different regions */
				if (JCCH_skew) { /* trade between different regions, both populous nations */
					int cust_reg = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int supp_reg = (cust_reg + 1 + UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum)) % 5;
					strcpy(param[1], skew_nations[supp_reg*5]);
					strcpy(param[2], skew_nations[cust_reg*5]);
					strcpy(param[3], "1993-01-01");
					strcpy(param[4], "1994-12-31");
				} else {/* trade between same regions, both non-populous nations */
					int both_reg = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int cust_nation = UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum);
					int supp_nation = UnifInt((DSS_HUGE)0,(DSS_HUGE)2,qnum);
					if (supp_nation >= cust_nation) supp_nation = (supp_nation+1)%4;
					strcpy(param[1], skew_nations[both_reg*5+1+supp_nation]);
					strcpy(param[2], skew_nations[both_reg*5+1+cust_nation]);
				}
#else
				tmp_date = pick_str(&nations2, qnum, param[1]);
				while (pick_str(&nations2, qnum, param[2]) == tmp_date);
#endif
				param[5][0] = '\0';
				break;
			case 8:
				/* previsously, these two were hardcoded, leave them the same by default */
				strcpy(param[4], "1995-01-01");
				strcpy(param[5], "1996-12-31");
				pick_str(&p_types_set, qnum, param[3]);
#ifdef JCCH_SKEW
				if (JCCH_skew) { 
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					strcpy(param[1], skew_nations[r1*5]);
					strcpy(param[2], skew_regions[r1]);
					strcpy(param[3], "SHINY MINED GOLD");
					strcpy(param[4], "1993-01-01");
					strcpy(param[5], "1994-12-31");
				} else {
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int r2 = (r1 + 1 + UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum)) % 5;
					int n1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum);
					strcpy(param[1], skew_nations[r2*5+1+n1]);
					strcpy(param[2], skew_regions[r1]);
				}
#else
				tmp_date = pick_str(&nations2, qnum, param[1]);
				tmp_date = nations.list[tmp_date].weight;
				strcpy(param[2], regions.list[tmp_date].text);
#endif
				param[6][0] = '\0';
				break;
			case 9:
				pick_str(&colors, qnum, param[1]);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					strcpy(param[1], "shiny mined gold" + UnifInt(0, 10, qnum));
				} 
#endif
				param[2][0] = '\0';
				break;
			case 10:
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					/* 1992-1994,1997-1998 */
					tmp_date = UnifInt((DSS_HUGE)0,(DSS_HUGE)5,qnum);
					sprintf(param[1],formats[10], 92 + tmp_date + (tmp_date>2)*2, 1);
					sprintf(param[2],formats[10], 92 + tmp_date + (tmp_date>2)*2, 1);

					/* a few days around black friday (which by far dominates) */
					int lo = 24 + UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int hi = 29 + UnifInt((DSS_HUGE)0,(DSS_HUGE)2,qnum);
					param[1][5] = param[2][5] = '0'; 
					param[1][6] = param[2][6] = '5';
					param[1][8] = '0'+ (lo/10); 
					param[1][9] = '0'+ (lo%10);
					param[2][2] = param[1][2];
					param[2][3] = param[1][3];
					param[2][8] = '0'+ (hi/10); 
					param[2][9] = '0'+ (hi%10);
				} else {
					/* a 3month interval from any day in the first two months of 1995 */
					tmp_date = UnifInt((DSS_HUGE)0,(DSS_HUGE)1,qnum);
					sprintf(param[1],formats[10], 95, 1+tmp_date);
					sprintf(param[2],formats[10], 95, 4+tmp_date);
					tmp_date = UnifInt((DSS_HUGE)1,(DSS_HUGE)25,qnum);
					param[1][8] = '0' + (tmp_date/10);
					param[2][8] = '0' + (tmp_date/10);
					param[1][9] = '0' + (tmp_date%10);
					param[2][9] = '0' + (tmp_date%10);
				}
#else
				tmp_date = UnifInt((DSS_HUGE)1,(DSS_HUGE)24,qnum);
				sprintf(param[1],formats[10],
					93 + tmp_date/12, tmp_date%12 + 1);
				sprintf(param[2],formats[10],
					93 + (tmp_date+3)/12, (tmp_date+3)%12 +1);
#endif
				param[3][0] = '\0';
				break;
			case 11:
#ifdef JCCH_SKEW
				if (JCCH_skew) { 
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					strcpy(param[1], skew_nations[r1*5]);
				} else {
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int n1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum);
					strcpy(param[1], skew_nations[r1*5+1+n1]);
				} 
#else
				pick_str(&nations2, qnum, param[1]);
#endif
				sprintf(param[2], "%11.10f", Q11_FRACTION / flt_scale );
				param[3][0] = '\0';
				break;
			case 12:
				tmp_date = pick_str(&l_smode_set, qnum, param[1]);
				while (tmp_date == pick_str(&l_smode_set, qnum, param[2]));
				tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)97,qnum);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					if (strcmp(param[1], "REG AIR")) strcpy(param[2], "REG AIR");
					tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)94,qnum);
				} else {
					tmp_date = UnifInt((DSS_HUGE)95,(DSS_HUGE)96,qnum);
					     
				}
#endif
				sprintf(param[3], formats[12], tmp_date);
				param[4][0] = '\0';
				break;
			case 13:
				pick_str(&q13a, qnum, param[1]);
				pick_str(&q13b, qnum, param[2]);
#ifdef JCCH_SKEW
				if (!JCCH_skew) { 
					/* we got '1mine2 3gold4' but can look for that with 16 variations */
					int lo1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)1,qnum);
					int hi1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)1,qnum);
					int lo2 = UnifInt((DSS_HUGE)0,(DSS_HUGE)1,qnum);
					int hi2 = UnifInt((DSS_HUGE)0,(DSS_HUGE)1,qnum);
					strcpy(param[1], (hi1?"1mine2":"1mine")+lo1);
					strcpy(param[2], (hi2?"3gold4":"3gold")+lo2);
				}
#endif
				param[3][0] = '\0';
				break;
			case 14:
			case 15:
#ifdef JCCH_SKEW
				/* avoid black friday for stability */
				tmp_date = UnifInt((DSS_HUGE)0,(DSS_HUGE)16,qnum);
				if (JCCH_skew) {
					sprintf(param[1],formats[14], /* include black friday */
						93 + tmp_date/8, 5 + (tmp_date%4));
				} else {
					sprintf(param[1],formats[14], /* exclude black friday */
						95 + tmp_date/8, tmp_date%8 + (((tmp_date%8)<4)?1:5));
				}
#else
				tmp_date = UnifInt((DSS_HUGE)0,(DSS_HUGE)59,qnum);
				sprintf(param[1],formats[14],
					93 + tmp_date/12, tmp_date%12 + 1);
#endif
				param[2][0] = '\0';
				break;
			case 16:
				tmp1 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum); 
				tmp2 = UnifInt((DSS_HUGE)1, (DSS_HUGE)4, qnum);
				sprintf(param[1], formats[16], tmp1, tmp2);
				pick_str(&p_types_set, qnum, param[2]);
				ptr = param[2] + (int)strlen(param[2]);
				while (*(--ptr) != ' ');
				*ptr = '\0';
				lptr = &sizes[0];
				permute(lptr,50,qnum);
				for (i=3; i <= MAX_PARAM; i++)
					sprintf(param[i], "%ld", sizes[i - 3]);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					/* populous parts have p_size=1 */
					strcpy(param[MAX_PARAM], "1");
				} else {
					for (i=3; i <= MAX_PARAM; i++) /* we void it otherwise */
						sprintf(param[i], "%ld", (sizes[i - 3] == 1)?2:sizes[i - 3]);
				}
#endif
				break;
			case 17:
				pick_str(&p_cntr_set, qnum, param[2]);
				tmp1 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum); 
				tmp2 = UnifInt((DSS_HUGE)1, (DSS_HUGE)4, qnum);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					strcpy(param[2], "LG BOX");
					tmp1 = tmp2 = 5; /* populous parts are Brand55 */  
				}
#endif
				sprintf(param[1], formats[17], tmp1, tmp2);
				param[3][0] = '\0';
				break;
			case 18:
				sprintf(param[1], HUGE_FORMAT, UnifInt((DSS_HUGE)312, (DSS_HUGE)315, qnum));
				strcpy(param[2], "50");
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					sprintf(param[1], HUGE_FORMAT, 500+UnifInt((DSS_HUGE)312, (DSS_HUGE)315, qnum));
					strcpy(param[2], "100");
				}
#endif
				param[3][0] = '\0';
				break;
			case 19:
				tmp1 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum); 
				tmp2 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum);
				sprintf(param[1], formats[19], tmp1, tmp2);
				tmp1 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum); 
				tmp2 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum);
				sprintf(param[2], formats[19], tmp1, tmp2);
				tmp1 = UnifInt((DSS_HUGE)1, (DSS_HUGE)5, qnum); 
				tmp2 = UnifInt((DSS_HUGE)1, (DSS_HUGE)4, qnum);
				sprintf(param[3], formats[19], tmp1, tmp2);
				sprintf(param[4], HUGE_FORMAT, UnifInt((DSS_HUGE)1, (DSS_HUGE)10, qnum));
				sprintf(param[5], HUGE_FORMAT, UnifInt((DSS_HUGE)10, (DSS_HUGE)20, qnum));
				sprintf(param[6], HUGE_FORMAT, UnifInt((DSS_HUGE)20, (DSS_HUGE)30, qnum));
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					/* Q19 populous parts have Brand55 and qty=51 */  
					sprintf(param[1], formats[19], 0, 0);
					sprintf(param[2], formats[19], 0, 0);
					sprintf(param[3], formats[19], 5, 5);
					sprintf(param[6], HUGE_FORMAT, UnifInt((DSS_HUGE)41, (DSS_HUGE)50, qnum));
				}
#endif
				param[7][0] = '\0';
				break;
			case 20:
				pick_str(&colors, qnum, param[1]);
				pick_str(&nations2, qnum, param[3]);
				tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)97,qnum);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					strcpy(param[1], "shiny mined gold");
					param[1][UnifInt(5, 16, qnum)] = 0;
					strcpy(param[3], skew_nations[r1*5]);
					tmp_date = UnifInt((DSS_HUGE)93,(DSS_HUGE)94,qnum);
				} else {
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int n1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum);
					strcpy(param[3], skew_nations[r1*5+1+n1]);
					tmp_date = UnifInt((DSS_HUGE)95,(DSS_HUGE)96,qnum);
				}	
#endif
				sprintf(param[2], formats[20], tmp_date);
				param[4][0] = '\0';
				break;
			case 21:
				pick_str(&nations2, qnum, param[1]);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					strcpy(param[1], skew_nations[r1*5]);
				} else {
					int r1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)4,qnum);
					int n1 = UnifInt((DSS_HUGE)0,(DSS_HUGE)3,qnum);
					strcpy(param[1], skew_nations[r1*5+1+n1]);
				}	
#endif
				param[2][0] = '\0';
				break;
			case 22:
				lptr = &ccode[0];
				permute(lptr,25, qnum);
				for (i=0; i <= 7; i++)
					sprintf(param[i+1], "%ld", 10 + ccode[i]);
#ifdef JCCH_SKEW
				if (JCCH_skew) {
					/* populous customers have these high nation codes */
					strcpy(param[1], "40");
					strcpy(param[2], "50");
					strcpy(param[3], "60");
					strcpy(param[4], "70");
					strcpy(param[5], "80");
				}
#endif
				param[8][0] = '\0';
				break;
			case 23:
			case 24:
                break;
			default:
				fprintf(stderr, 
					"No variable definitions available for query %d\n", 
                    qnum);
				return;
        }
    }
	
    if (flags & LOG)
	{
        if (lfp == NULL)
		{
            lfp = fopen(lfile, "a");
            OPEN_CHECK(lfp, lfile);
		}
        fprintf(lfp, "%d", qnum);
        for (i=1; i <= 10; i++)
            if (flags & DFLT)
			{
				if (defaults[qnum - 1][i - 1] == NULL)
					break;
				else
					fprintf(lfp, "\t%s", defaults[qnum - 1][i - 1]);
			}
            else
			{
				if (param[i][0] == '\0')
					break;
				else
					fprintf(lfp, "\t%s", param[i]);
			}
			fprintf(lfp, "\n");
	}
    }
    else
	{
        if (flags & DFLT)   
		{
            /* to allow -d to work at all scale factors */
            if (qnum == 11 && vnum == 2)
                fprintf(ofp, "%11.10f", Q11_FRACTION/flt_scale);
            else
                if (defaults[qnum - 1][vnum - 1])
                    fprintf(ofp, "%s", defaults[qnum - 1][vnum - 1]);
                else
					fprintf(stderr, 
					"Bad default request (q: %d, p: %d)\n",
					qnum, vnum);
		}
        else        
		{
            if (param[vnum] && vnum <= MAX_PARAM)
                fprintf(ofp, "%s", param[vnum]);
            else
				fprintf(stderr, "Bad parameter request (q: %d, p: %d)\n",
				qnum, vnum);
		}
	}
    return;
}
