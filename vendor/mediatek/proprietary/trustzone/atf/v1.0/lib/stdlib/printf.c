/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "mt_cpuxgpt.h"
#include <arch_helpers.h>
#include <platform.h>

int (*log_lock_acquire)();
int (*log_write)(unsigned char);
int (*log_lock_release)();

int (*log_lock_acquire2)();
int (*log_write2)(unsigned char);
int (*log_lock_release2)();


/* Choose max of 128 chars for now. */
#define PRINT_BUFFER_SIZE 128
#define TIMESTAMP_BUFFER_SIZE 32
#define ATF_SCHED_CLOCK_UNIT 1000000000 //ns

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* itoa:  convert n to characters in s */
int utoa(unsigned int n, char s[])
{
    int i;

    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    s[i] = '\0';
    reverse(s);
	return i;
}
int ltoa(unsigned long long n, char s[])
{
    int i;

    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    s[i] = '\0';
    reverse(s);
	return i;
}


char buf[PRINT_BUFFER_SIZE] __attribute__((aligned(8)));
char timestamp_buf[TIMESTAMP_BUFFER_SIZE] __attribute__((aligned(8)));

int printf(const char *fmt, ...)
{
	unsigned long long cur_time;
	unsigned long long sec_time;
	unsigned long long ns_time;
	va_list args;
	int 	count;
	char	*timestamp_bufptr = timestamp_buf;

    /* try get buffer lock */
    if (log_lock_acquire)
        (*log_lock_acquire)();

    /* in ATF boot time, tiemr for cntpct_el0 is not initialized
     * so it will not count now.
     */
    cur_time = atf_sched_clock();
    sec_time = cur_time / ATF_SCHED_CLOCK_UNIT;
    ns_time = (cur_time % ATF_SCHED_CLOCK_UNIT)/1000;
	
	*timestamp_bufptr++ = '[';
	*timestamp_bufptr++ = 'A';
	*timestamp_bufptr++ = 'T';
	*timestamp_bufptr++ = 'F';
	*timestamp_bufptr++ = ']';
	*timestamp_bufptr++ = '(';
	count = utoa(platform_get_core_pos(read_mpidr()), timestamp_bufptr);
	timestamp_bufptr += count;
	*timestamp_bufptr++ = ')';
	*timestamp_bufptr++ = '[';
	count = ltoa(sec_time, timestamp_bufptr);
	timestamp_bufptr += count;
	*timestamp_bufptr++ = '.';
	count = ltoa(ns_time, timestamp_bufptr);
	timestamp_bufptr += count;
	*timestamp_bufptr++ = ']';
	*timestamp_bufptr++ = '\0';

	timestamp_buf[TIMESTAMP_BUFFER_SIZE - 1] = '\0';
	count = 0;
	while (timestamp_buf[count])
	{
        /* output char to ATF log buffer */
        if (log_write)
            (*log_write)(timestamp_buf[count]);
		if (putchar(timestamp_buf[count]) != EOF) {
			count++;
		} else {
			break;
		}
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args);

	/* Use putchar directly as 'puts()' adds a newline. */
	buf[PRINT_BUFFER_SIZE - 1] = '\0';
	count = 0;
	while (buf[count])
	{
        /* output char to ATF log buffer */
        if (log_write)
            (*log_write)(buf[count]);

		if (putchar(buf[count]) != EOF) {
			count++;
		} else {
			count = EOF;
			break;
		}
	}

    /* release buffer lock */
    if (log_lock_release)
        (*log_lock_release)();

	return count;
}

void bl31_log_service_register(int (*lock_get)(),
    int (*log_putc)(unsigned char),
    int (*lock_release)())
{
    log_lock_acquire = lock_get;
    log_write = log_putc;
    log_lock_release = lock_release;
}

void bl31_log_service_register2(int (*lock_get)(),
    int (*log_putc)(unsigned char),
    int (*lock_release)())
{
    log_lock_acquire2 = lock_get;
    log_write2 = log_putc;
    log_lock_release2 = lock_release;
}

