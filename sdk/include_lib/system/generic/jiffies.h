#ifndef JIFFIES_H
#define JIFFIES_H

/* timer interface */
/* Parameters used to convert the timespec values: */
#define HZ				100L
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL


#ifndef __ASSEMBLY__
extern volatile unsigned long jiffies;
unsigned long jiffies_msec(void);
unsigned long jiffies_offset2msec(unsigned long begin_msec, int offset_msec);
int jiffies_msec2offset(unsigned long begin_msec, unsigned long end_msec);
unsigned long jiffies_usec(void);
unsigned long jiffies_offset2usec(unsigned long base_usec, int offset_usec);
int jiffies_usec2offset(unsigned long begin, unsigned long end);
unsigned long audio_jiffies_usec(void);
#endif

#define time_after(a,b)                 ((long)(b) - (long)(a) < 0)
#define time_before(a,b)                time_after(b,a)

#define msecs_to_jiffies(msec)          ((msec) / (1000 / HZ))
#define jiffies_to_msecs(j)             ((j) * (1000 / HZ))

#endif

