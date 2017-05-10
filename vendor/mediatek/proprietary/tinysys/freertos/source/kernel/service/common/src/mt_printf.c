/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "interrupt.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <tinysys_config.h>
#define MAX_LOG_SIZE 256
#define LONGFLAG     0x00000001
#define LONGLONGFLAG 0x00000002
#define HALFFLAG     0x00000004
#define HALFHALFFLAG 0x00000008
#define SIZETFLAG    0x00000010
#define ALTFLAG      0x00000020
#define CAPSFLAG     0x00000040
#define SHOWSIGNFLAG 0x00000080
#define SIGNEDFLAG   0x00000100
#define LEFTFORMATFLAG 0x00000200
#define LEADZEROFLAG 0x00000400
extern int __io_putchar(int ch) __attribute__((weak));
extern int logger_putc(int ch) __attribute__((weak));
static uint32_t g_debug_level = LOG_DEBUG;
static char printf_msg_std[MAX_LOG_SIZE + 4];
static char printf_msg_inhouse[MAX_LOG_SIZE + 4];

int _dputs(const char *str)
{
    uint32_t ix = 0;
    while (*str != '\0' && ix <= MAX_LOG_SIZE) {
        __io_putchar(*str++);
        ix++;
    }

    return ix;
}

int logger_puts(const char *str)
{
    uint32_t ix = 0;
    while (*str != '\0' && ix <= MAX_LOG_SIZE) {
        logger_putc(*str++);
        ix++;
    }

    return ix;
}

static char *longlong_to_string(char *buf, unsigned long long n, int len, uint flag)
{
    int pos = len;
    int negative = 0;

    if ((flag & SIGNEDFLAG) && (long long)n < 0) {
        negative = 1;
        n = -n;
    }

    buf[--pos] = 0;
    /* only do the math if the number is >= 10 */
    while (n >= 10) {
        int digit = n % 10;

        n /= 10;

        buf[--pos] = digit + '0';
    }
    buf[--pos] = n + '0';
    if (negative)
        buf[--pos] = '-';
    else if ((flag & SHOWSIGNFLAG))
        buf[--pos] = '+';

    return &buf[pos];
}
#define FLOAT_PRINTF 1
static const char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
static const char hextable_caps[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
#if FLOAT_PRINTF
#define bool uint32_t
union double_int {
    double d;
    uint64_t i;
};

#define OUT(c) buf[pos++] = (c)
#define OUTSTR(str) do { for (size_t i = 0; (str)[i] != 0; i++) OUT((str)[i]); } while (0)

/* print up to a 4 digit exponent as string, with sign */
static size_t exponent_to_string(char *buf, int32_t exponent)
{
    size_t pos = 0;

    /* handle sign */
    if (exponent < 0) {
        OUT('-');
        exponent = -exponent;
    } else {
        OUT('+');
    }

    /* see how far we need to bump into the string to print from the right */
    if (exponent >= 1000) pos += 4;
    else if (exponent >= 100) pos += 3;
    else if (exponent >= 10) pos += 2;
    else pos++;

    /* print decimal string, from the right */
    uint i = pos;
    do {
        uint digit = (uint32_t)exponent % 10;

        buf[--i] = digit + '0';

        exponent /= 10;
    } while (exponent != 0);

    /* return number of characters printed */
    return pos;
}

static char *double_to_string(char *buf, size_t len, double d, uint flag)
{
    size_t pos = 0;
    union double_int u = { d };

    uint32_t exponent = (u.i >> 52) & 0x7ff;
    uint64_t fraction = (u.i & ((1ULL << 52) - 1));
    bool neg = !!(u.i & (1ULL << 63));

    /* start constructing the string */
    if (neg) {
        OUT('-');
        d = -d;
    }

    /* longest:
     * 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000o
     */

    /* look for special cases */
    if (exponent == 0x7ff) {
        if (fraction == 0) {
            /* infinity */
            if (flag & CAPSFLAG) OUTSTR("INF");
            else OUTSTR("inf");
        } else {
            /* NaN */
            if (flag & CAPSFLAG) OUTSTR("NAN");
            else OUTSTR("nan");
        }
    } else if (exponent == 0) {
        if (fraction == 0) {
            /* zero */
            OUTSTR("0.000000");
        } else {
            /* denormalized */
            /* XXX does not handle */
            if (flag & CAPSFLAG) OUTSTR("DEN");
            else OUTSTR("den");
        }
    } else {
        /* see if it's in the range of floats we can easily print */
        int exponent_signed = exponent - 1023;
        if (exponent_signed < -52 || exponent_signed > 52) {
            OUTSTR("<range>");
        } else {
            /* start by walking backwards through the string */
#define OUTREV(c) do { if (&buf[pos] == buf) goto done; else buf[--pos] = (c); } while (0)
            pos = len;
            OUTREV(0);

            /* reserve space for the fractional component first */
            for (int i = 0; i <= 6; i++)
                OUTREV('0');
            size_t decimal_spot = pos;

            /* print the integer portion */
            uint64_t u;
            if (exponent_signed >= 0) {
                u = fraction;
                u |= (1ULL << 52);
                u >>= (52 - exponent_signed);

                char *s = longlong_to_string(buf, u, pos + 1, flag);

                pos = s - buf;
            } else {
                /* exponent is negative */
                u = 0;
                OUTREV('0');
            }

            buf[decimal_spot] = '.';

            /* handle the fractional part */
            uint32_t frac = ((d - u) * 1000000) + (double).5;

            int i = decimal_spot + 6 + 1;
            while (frac != 0) {
                uint digit = frac % 10;

                buf[--i] = digit + '0';

                frac /= 10;
            }

            if (neg)
                OUTREV('-');

done:
            /* separate return path, since we've been walking backwards through the string */
            return &buf[pos];
        }
#undef OUTREV
    }

    buf[pos] = 0;
    return buf;
}

static char *double_to_hexstring(char *buf, size_t len, double d, uint flag)
{
    size_t pos = 0;
    union double_int u = { d };

    uint32_t exponent = (u.i >> 52) & 0x7ff;
    uint64_t fraction = (u.i & ((1ULL << 52) - 1));
    bool neg = !!(u.i & (1ULL << 63));

    /* start constructing the string */
    if (neg) {
        OUT('-');
    }

    /* look for special cases */
    if (exponent == 0x7ff) {
        if (fraction == 0) {
            /* infinity */
            if (flag & CAPSFLAG) OUTSTR("INF");
            else OUTSTR("inf");
        } else {
            /* NaN */
            if (flag & CAPSFLAG) OUTSTR("NAN");
            else OUTSTR("nan");
        }
    } else if (exponent == 0) {
        if (fraction == 0) {
            /* zero */
            if (flag & CAPSFLAG) OUTSTR("0X0P+0");
            else OUTSTR("0x0p+0");
        } else {
            /* denormalized */
            /* XXX does not handle */
            if (flag & CAPSFLAG) OUTSTR("DEN");
            else OUTSTR("den");
        }
    } else {
        /* regular normalized numbers:
         * 0x1p+1
         * 0x1.0000000000001p+1
         * 0X1.FFFFFFFFFFFFFP+1023
         * 0x1.FFFFFFFFFFFFFP+1023
         */
        int exponent_signed = exponent - 1023;

        /* implicit 1. */
        if (flag & CAPSFLAG) OUTSTR("0X1");
        else OUTSTR("0x1");

        /* select the appropriate hex case table */
        const char *table = (flag & CAPSFLAG) ? hextable_caps : hextable;

        int zero_count = 0;
        bool output_dot = false;
        for (int i = 52 - 4; i >= 0; i -= 4) {
            uint digit = (fraction >> i) & 0xf;

            if (digit == 0) {
                zero_count++;
            } else {
                /* output a . the first time we output a char */
                if (!output_dot) {
                    OUT('.');
                    output_dot = true;
                }
                /* if we have a non zero digit, see if we need to output a string of zeros */
                while (zero_count > 0) {
                    OUT('0');
                    zero_count--;
                }
                buf[pos++] = table[digit];
            }
        }

        /* handle the exponent */
        buf[pos++] = (flag & CAPSFLAG) ? 'P' : 'p';
        pos += exponent_to_string(&buf[pos], exponent_signed);
    }

    buf[pos] = 0;
    return buf;
}

#undef OUT
#undef OUTSTR

#endif // FLOAT_PRINTF


static char *longlong_to_hexstring(char *buf, unsigned long long u, int len, uint flag)
{
    int pos = len;
    static const char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    static const char hextable_caps[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    const char *table;

    if ((flag & CAPSFLAG))
        table = hextable_caps;
    else
        table = hextable;

    buf[--pos] = 0;
    do {
        unsigned int digit = u % 16;
        u /= 16;
        buf[--pos] = table[digit];
    } while (u != 0);

    return &buf[pos];
}
void set_debug_level(uint32_t new_debug_level)
{
    if (new_debug_level <= 3 && new_debug_level >= 0)
        g_debug_level = new_debug_level;
}
uint32_t get_debug_level(void)
{
    return g_debug_level;
}

int vsnprintf(char *str, size_t len, const char *fmt, va_list ap)
{
    char c;
    unsigned char uc;
    const char *s;
    unsigned long long n;
    void *ptr;
    int flags;
    unsigned int format_num;
    char num_buffer[32];
    size_t chars_written = 0;


#define OUTPUT_CHAR(c) do { (str[chars_written++] = c); if (chars_written + 1 == MAX_LOG_SIZE) goto done; } while(0)
#define OUTPUT_CHAR_NOLENCHECK(c) do { (str[chars_written++] = c);} while(0)


    for (;;) {
        /* handle regular chars that aren't format related */
        while ((c = *fmt++) != 0) {
            if (c == '%')
                break; /* we saw a '%', break and start parsing format */
            OUTPUT_CHAR(c);
        }
        /* make sure we haven't just hit the end of the string */
        if (c == 0)
            break;

        /* reset the format state */
        flags = 0;
        format_num = 0;



next_format:
        /* grab the next format character */
        c = *fmt++;
        if (c == 0)
            break;
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (c == '0' && format_num == 0)
                    flags |= LEADZEROFLAG;
                format_num *= 10;
                format_num += c - '0';
                goto next_format;
            case '.':
                /* XXX for now eat numeric formatting */
                goto next_format;
            case '%':
                OUTPUT_CHAR('%');
                break;
            case 'c':
                uc = va_arg(ap, unsigned int);
                OUTPUT_CHAR(uc);
                break;
            case 's':
                s = va_arg(ap, const char *);
                if (s == 0)
                    s = "<null>";
                goto _output_string;
            case '-':
                flags |= LEFTFORMATFLAG;
                goto next_format;
            case '+':
                flags |= SHOWSIGNFLAG;
                goto next_format;
            case '#':
                flags |= ALTFLAG;
                goto next_format;
            case 'l':
                if (flags & LONGFLAG)
                    flags |= LONGLONGFLAG;
                flags |= LONGFLAG;
                goto next_format;
            case 'h':
                if (flags & HALFFLAG)
                    flags |= HALFHALFFLAG;
                flags |= HALFFLAG;
                goto next_format;
            case 'z':
                flags |= SIZETFLAG;
                goto next_format;
            case 'D':
                flags |= LONGFLAG;
            /* fallthrough */
            case 'i':
            case 'd':
                n = (flags & LONGLONGFLAG) ? va_arg(ap, long long) :
                    (flags & LONGFLAG) ? va_arg(ap, long) :
                    (flags & HALFHALFFLAG) ? (signed char)va_arg(ap, int) :
                    (flags & HALFFLAG) ? (short)va_arg(ap, int) :
                    (flags & SIZETFLAG) ? va_arg(ap, ssize_t) : va_arg(ap, int);
                flags |= SIGNEDFLAG;
                s = longlong_to_string(num_buffer, n, sizeof(num_buffer), flags);
                goto _output_string;
            case 'U':
                flags |= LONGFLAG;
            /* fallthrough */
            case 'u':
                n = (flags & LONGLONGFLAG) ? va_arg(ap, unsigned long long) :
                    (flags & LONGFLAG) ? va_arg(ap, unsigned long) :
                    (flags & HALFHALFFLAG) ? (unsigned char)va_arg(ap, unsigned int) :
                    (flags & HALFFLAG) ? (unsigned short)va_arg(ap, unsigned int) :
                    (flags & SIZETFLAG) ? va_arg(ap, size_t) :
                    va_arg(ap, unsigned int);
                s = longlong_to_string(num_buffer, n, sizeof(num_buffer), flags);
                goto _output_string;
            case 'p':
                flags |= LONGFLAG | ALTFLAG;
                goto hex;
            case 'X':
                flags |= CAPSFLAG;
                /* fallthrough */
hex:
            case 'x':
                n = (flags & LONGLONGFLAG) ? va_arg(ap, unsigned long long) :
                    (flags & LONGFLAG) ? va_arg(ap, unsigned long) :
                    (flags & HALFHALFFLAG) ? (unsigned char)va_arg(ap, unsigned int) :
                    (flags & HALFFLAG) ? (unsigned short)va_arg(ap, unsigned int) :
                    (flags & SIZETFLAG) ? va_arg(ap, size_t) :
                    va_arg(ap, unsigned int);
                s = longlong_to_hexstring(num_buffer, n, sizeof(num_buffer), flags);
                if (flags & ALTFLAG) {
                    OUTPUT_CHAR('0');
                    OUTPUT_CHAR((flags & CAPSFLAG) ? 'X' : 'x');
                }
                goto _output_string;
            case 'n':
                ptr = va_arg(ap, void *);
                if (flags & LONGLONGFLAG)
                    *(long long *)ptr = chars_written;
                else if (flags & LONGFLAG)
                    *(long *)ptr = chars_written;
                else if (flags & HALFHALFFLAG)
                    *(signed char *)ptr = chars_written;
                else if (flags & HALFFLAG)
                    *(short *)ptr = chars_written;
                else if (flags & SIZETFLAG)
                    *(size_t *)ptr = chars_written;
                else
                    *(int *)ptr = chars_written;
                break;
#if FLOAT_PRINTF
            case 'F':
                flags |= CAPSFLAG;
            /* fallthrough */
            case 'f': {
                double d = va_arg(ap, double);
                s = double_to_string(num_buffer, sizeof(num_buffer), d, flags);
                goto _output_string;
            }
            case 'A':
                flags |= CAPSFLAG;
            /* fallthrough */
            case 'a': {
                double d = va_arg(ap, double);
                s = double_to_hexstring(num_buffer, sizeof(num_buffer), d, flags);
                goto _output_string;
            }
#endif
            default:
                OUTPUT_CHAR('%');
                OUTPUT_CHAR(c);
                break;
        }

        /* move on to the next field */
        continue;

        /* shared output code */
_output_string:
        if (flags & LEFTFORMATFLAG) {
            /* left justify the text */
            uint count = 0;
            while (*s != 0) {
                OUTPUT_CHAR(*s++);
                count++;
            }

            /* pad to the right (if necessary) */
            for (; format_num > count; format_num--)
                OUTPUT_CHAR(' ');
        } else {
            /* right justify the text (digits) */
            size_t string_len = strlen(s);
            char outchar = (flags & LEADZEROFLAG) ? '0' : ' ';
            for (; format_num > string_len; format_num--)
                OUTPUT_CHAR(outchar);

            /* output the string */
            while (*s != 0)
                OUTPUT_CHAR(*s++);
        }
        continue;
    }
    va_end(ap);

done:
    OUTPUT_CHAR('\r');
    /* null terminate */
    OUTPUT_CHAR_NOLENCHECK('\0');
    chars_written--; /* don't count the null */

#undef OUTPUT_CHAR
#undef OUTPUT_CHAR_NOLENCHECK
    return chars_written;
}

/* printf wrap function for library use*/
int __wrap_printf(const char *fmt, ...)
{
    va_list args;
    int in_isr;

    /* prevent from context switch when print out msg */
    /* NOTE: in ISR, there is no prevention */
    if (is_in_isr()) {
        in_isr = 1;
    } else {
        taskENTER_CRITICAL();
        in_isr = 0;
    }
    va_start(args, fmt);
    vsnprintf(printf_msg_std, sizeof(printf_msg_std), fmt, args);
    va_end(args);

    /* To prevent from massive UART dump
     * write string to UART, if log_level <= global debug level*/
    if (LOG_DEBUG <= g_debug_level)
        _dputs(printf_msg_std);
    /* prevent from context switch when print out msg */
    /* NOTE: in ISR, there is no prevention */
    if (in_isr == 0) {
        taskEXIT_CRITICAL();
    }
#ifdef CFG_LOGGER_SUPPORT
    logger_puts(printf_msg_std);
#endif
    return 0;
}

/* scp printf with log level for freertos source code use*/
void scp_printf(uint32_t log_level, char *fmt, ...)
{
    va_list args;
    int in_isr;

    /* prevent from context switch when print out msg */
    /* NOTE: in ISR, there is no prevention */
    if (is_in_isr()) {
        in_isr = 1;
    } else {
        taskENTER_CRITICAL();
        in_isr = 0;
    }
    va_start(args, fmt);
    vsnprintf(printf_msg_inhouse, sizeof(printf_msg_inhouse), fmt, args);
    va_end(args);

    /* To prevent from massive UART dump
     * write string to UART, if log_level <= global debug level*/
    if (log_level <= g_debug_level)
        _dputs(printf_msg_inhouse);

    /* prevent from context switch when print out msg */
    /* NOTE: in ISR, there is no prevention */
    if (in_isr == 0) {
        taskEXIT_CRITICAL();
    }

#ifdef CFG_LOGGER_SUPPORT
    logger_puts(printf_msg_inhouse);
#endif
    return ;
}
