/*
 * FunctionCheck profiler
 * (C) Copyright 2000-2002 Yannick Perret 
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "fc_time.h"
#include "fc_tools.h"


/* time mode */
int fc_time_tmode = FC_MTIME_TSC;

static clock_t fc_first_time_clock = 0;
static struct timeval fc_first_time_tv = {0, 0};
static unsigned long long fc_first_time_tsc = 0;

static void rdtsc(unsigned long long *val);
void fc_timer_tsc(unsigned long long *val);

/** function pointer on the effective timer. needed by fc_gettimeofday **/
void (*FC_TIMER_PTR)(unsigned long long *) = fc_timer_tsc;


/* get time for external unit */
inline void fc_timer_ext(unsigned long long *val)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    if (tv.tv_sec == fc_first_time_tv.tv_sec)
    {
        *val = tv.tv_usec - fc_first_time_tv.tv_usec;
        return;
    }

    *val = (tv.tv_sec - fc_first_time_tv.tv_sec - 1)*1000000 +
            tv.tv_usec + 1000000 - fc_first_time_tv.tv_usec;
}

/* get time for internal unit */
inline void fc_timer_cpu(unsigned long long *val)
{
    clock_t result;
    unsigned int delta;

    /* CPU/SYS time */
    result = (unsigned int) clock();

    /* over the limit ? */
    if (fc_first_time_clock < result)
    {
        delta = result - fc_first_time_clock;
    }
    else
    {
        /* all bits set except the highest bit is clock_t max value? */
        static const clock_t max_clock = ~(1 << (sizeof(clock_t)*8 - 1));

        delta = result + (max_clock - fc_first_time_clock);
    }

    *val = delta;
}

/* get time using cpu tsc */
inline void fc_timer_tsc(unsigned long long *val)
{
    unsigned long long result;

    rdtsc(&result);

    *val = result - fc_first_time_tsc;

    return;
}

inline void rdtsc(unsigned long long *val)
{
#if defined (__GNUC__) && defined (__i386__)
    unsigned long lo, hi;
    asm volatile (
            "pushl %%ebx\n" /* ebx is used in PIC shared library? */
            "pushl %%ecx\n" /* ecx is overwritten by CPUID (6C65746EH i.e. 'ntel') */
            "xorl %%eax, %%eax\n"
            "cpuid\n"
            "rdtsc\n"
            "popl %%ecx\n"
            "popl %%ebx\n"
            : "=a" (lo), "=d" (hi)
            );
    *val = (unsigned long long) hi << 32 | lo;
#elif defined (__GNUC__) && defined (__x86_64__)
    unsigned long lo, hi;
    asm volatile (
            "push %%rbx\n" /* ebx is used in PIC shared library? */
            "push %%rcx\n" /* ecx is overwritten by CPUID (6C65746EH i.e. 'ntel') */
            "xor %%rax, %%rax\n"
            "cpuid\n"
            "rdtsc\n"
            "pop %%rcx\n"
            "pop %%rbx\n"
            : "=a" (lo), "=d" (hi)
            );
    *val = (unsigned long long) hi << 32 | lo;
#else
    printf("Error! rdtsc opcode not available\n");
    *val = 0;
#endif
}

/* set the time mode */
inline int fc_set_time_type(char *buf)
{
    if (strcasecmp(buf, "ext") == 0)
    {
        FC_TIMER_PTR = fc_timer_ext;
        fc_time_tmode = FC_MTIME_EXT;
        return (FC_MTIME_EXT);
    }
    else
        if (strcasecmp(buf, "cpu") == 0)
    {
        FC_TIMER_PTR = fc_timer_cpu;
        fc_time_tmode = FC_MTIME_CPU;
        return (FC_MTIME_CPU);
    }
    else
        if (strcasecmp(buf, "sys") == 0)
    {
        FC_TIMER_PTR = fc_timer_cpu;
        fc_time_tmode = FC_MTIME_CPU;
        return (FC_MTIME_CPU);
    }
    else
        if (strcasecmp(buf, "tsc") == 0)
    {
        FC_TIMER_PTR = fc_timer_tsc;
        fc_time_tmode = FC_MTIME_TSC;
        return (FC_MTIME_TSC);
    }
    else
    {
        fc_message("time: invalid value for 'FC_TIME_MODE' (%s).", buf);
        fc_message("time: ignored (TSC used).");
        FC_TIMER_PTR = fc_timer_tsc;
        fc_time_tmode = FC_MTIME_TSC;
        return (FC_MTIME_TSC);
    }
}

/* get the time mode */
inline int fc_get_time_type()
{
    return (fc_time_tmode);
}

/* init the time system */
inline void fc_init_time()
{
    fc_first_time_clock = clock();
    gettimeofday(&fc_first_time_tv, NULL);
    rdtsc(&fc_first_time_tsc);
}

/* my 'gettimeofday'. returns a 'timeval' structure containing
   the current time (in the good clock-mode) */
inline void fc_gettimeofday(unsigned long long *val)
{
    /* just call the good function */
    FC_TIMER_PTR(val);
}
