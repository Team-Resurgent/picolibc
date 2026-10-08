/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Console time zone settings -> POSIX TZ rule, shared by the original Xbox and
 * Xbox 360 runtimes. Each runtime fills a __xbox_tz_info from its own settings
 * store at startup and installs the rule as the default TZ, so localtime() and
 * friends follow the time zone chosen in the console dashboard.
 *
 * The fields follow the Windows TIME_ZONE_INFORMATION conventions both consoles
 * use: biases are minutes with UTC = local + bias, and transition dates are
 * "the Nth weekday of a month" rules.
 */

#ifndef _SYS_XBOX_TZ_H_
#define _SYS_XBOX_TZ_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __xbox_tz_date {
    unsigned char month;  /* 1-12; 0 = no transition */
    unsigned char week;   /* occurrence of wday in the month, 1-5; 5 = last */
    unsigned char wday;   /* 0 = Sunday */
    unsigned char hour;   /* local time the transition happens at */
    unsigned char minute;
};

struct __xbox_tz_info {
    long                  bias;     /* minutes; UTC = local + bias */
    long                  std_bias; /* added to bias during standard time */
    long                  dst_bias; /* added to bias during daylight time */
    char                  std_name[16];
    char                  dst_name[16];
    struct __xbox_tz_date std_date; /* when standard time begins */
    struct __xbox_tz_date dst_date; /* when daylight time begins */
};

/* Longest rule __xbox_tz_rule() writes, including the terminator. */
#define __XBOX_TZ_RULE_MAX 64

/*
 * Write the POSIX TZ rule for `info` to `buf`. Daylight saving time is only
 * emitted when both transition dates are valid; otherwise the rule is the
 * constant offset `bias`. Names that are not valid POSIX abbreviations are
 * replaced with a numeric one such as <+0530>. Returns the rule length, or -1 if
 * it does not fit in `size` bytes.
 */
int __xbox_tz_rule(const struct __xbox_tz_info *info, char *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_XBOX_TZ_H_ */
