#ifndef BUILDNUM_H
#define BUILDNUM_H

/* build_number.h */
#pragma once

/* Works in C99+. Optional _Static_assert sanity checks enable on C11+. */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Civil date math (Howard Hinnant’s algorithm), pure C ----
   Returns serial day number relative to 1970-01-01
   (absolute offset cancels in differences). */
static inline int days_from_civil(int y, unsigned m, unsigned d) {
    y -= (m <= 2);
    /* era is floor division by 400 that works for negatives too */
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);                       /* [0, 399] */
    const unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5) + d - 1; /* [0, 365] */
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           /* [0, 146096] */
    return era * 146097 + (int)doe - 719468;                               /* 719468 = days to 1970-01-01 */
}

/* ---- __DATE__ ("Mmm dd yyyy") parser (English abbreviations) ---- */
static inline int parse_month(const char* s) {
    /* s points to the start of __DATE__ (e.g., "Sep 23 2025") */
    /* Compare first three chars. This is fast and portable. */
    return (s[0]=='J'&&s[1]=='a'&&s[2]=='n')? 1 :
           (s[0]=='F'&&s[1]=='e'&&s[2]=='b')? 2 :
           (s[0]=='M'&&s[1]=='a'&&s[2]=='r')? 3 :
           (s[0]=='A'&&s[1]=='p'&&s[2]=='r')? 4 :
           (s[0]=='M'&&s[1]=='a'&&s[2]=='y')? 5 :
           (s[0]=='J'&&s[1]=='u'&&s[2]=='n')? 6 :
           (s[0]=='J'&&s[1]=='u'&&s[2]=='l')? 7 :
           (s[0]=='A'&&s[1]=='u'&&s[2]=='g')? 8 :
           (s[0]=='S'&&s[1]=='e'&&s[2]=='p')? 9 :
           (s[0]=='O'&&s[1]=='c'&&s[2]=='t')?10 :
           (s[0]=='N'&&s[1]=='o'&&s[2]=='v')?11 :
           (s[0]=='D'&&s[1]=='e'&&s[2]=='c')?12 : 0;
}

static inline int parse_day(const char* s) {
    /* __DATE__ is "Mmm dd yyyy" with day right-aligned:
       positions: 0 1 2 3 4 5 6 7 8 9 10
                  M m m   d d   y y  y  y
       Day is either " d" or "dd" starting at s[4]. */
    return (s[4] == ' ') ? (s[5] - '0') : ( (s[4]-'0')*10 + (s[5]-'0') );
}

static inline int parse_year(const char* s) {
    return (s[7]-'0')*1000 + (s[8]-'0')*100 + (s[9]-'0')*10 + (s[10]-'0');
}

/* ---- Helpers for yyyymmdd <-> y,m,d ---- */
static inline void yyyymmdd_to_ymd(int yyyymmdd, int* y, unsigned* m, unsigned* d) {
    int Y = yyyymmdd / 10000;
    unsigned M = (unsigned)((yyyymmdd / 100) % 100);
    unsigned D = (unsigned)(yyyymmdd % 100);
    if (y) *y = Y;
    if (m) *m = M;
    if (d) *d = D;
}

/* Core: returns days since `start_yyyymmdd` using compiler's build date (__DATE__). */
static inline int build_number(int start_yyyymmdd) {
    int ey; unsigned em, ed;
    yyyymmdd_to_ymd(start_yyyymmdd, &ey, &em, &ed);

    /* __DATE__ is implementation-defined but widely "Mmm dd yyyy" in English. */
    static const char cdate[] = __DATE__;
    const int  by = parse_year(cdate);
    const unsigned bm = (unsigned)parse_month(cdate);
    const unsigned bd = (unsigned)parse_day(cdate);

    return days_from_civil(by, bm, bd) - days_from_civil(ey, em, ed);
}

/* Overload alternative: supply an explicit build date (yyyymmdd). */
static inline int build_number_from_date(int start_yyyymmdd, int build_yyyymmdd) {
    int ey; unsigned em, ed;
    yyyymmdd_to_ymd(start_yyyymmdd, &ey, &em, &ed);

    int by; unsigned bm, bd;
    yyyymmdd_to_ymd(build_yyyymmdd, &by, &bm, &bd);

    return days_from_civil(by, bm, bd) - days_from_civil(ey, em, ed);
}

#ifdef __cplusplus
}
#endif

#endif //BUILDNUM_H