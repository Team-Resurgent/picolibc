/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Console time zone settings -> POSIX TZ rule. See <sys/xbox_tz.h>.
 *
 * The output is always fully explicit (both offsets, both transition rules), so
 * tzset() never falls back to its built-in US defaults.
 */

#include <stdio.h>
#include <string.h>
#include <sys/xbox_tz.h>

/* tzset() accepts quoted names of 3 to TZNAME_MAX (10) characters from this set. */
#define TZ_NAME_MIN 3
#define TZ_NAME_MAX 10

static int
name_char_ok(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+'
        || c == '-';
}

/* Write "<name>" from the console's name, or a numeric "<+hhmm>" from the UTC offset. */
static int
put_name(char *buf, size_t size, const char *name, size_t name_size, long bias)
{
    size_t len = 0;

    while (len < name_size && name[len] != '\0') {
        if (!name_char_ok(name[len]))
            break;
        len++;
    }
    if ((len == name_size || name[len] == '\0') && len >= TZ_NAME_MIN && len <= TZ_NAME_MAX)
        return snprintf(buf, size, "<%.*s>", (int)len, name);

    /* The name runs east-positive, the opposite sign of the bias. */
    long east = -bias;
    char sign = east < 0 ? '-' : '+';
    if (east < 0)
        east = -east;
    return snprintf(buf, size, "<%c%02ld%02ld>", sign, east / 60, east % 60);
}

/* Write a POSIX offset ("8", "-5:30"): hours west of UTC, like the bias. */
static int
put_offset(char *buf, size_t size, long bias)
{
    char sign = bias < 0 ? '-' : '\0';
    if (bias < 0)
        bias = -bias;
    if (bias % 60)
        return snprintf(buf, size, "%s%ld:%02ld", sign ? "-" : "", bias / 60, bias % 60);
    return snprintf(buf, size, "%s%ld", sign ? "-" : "", bias / 60);
}

static int
date_ok(const struct __xbox_tz_date *d)
{
    return d->month >= 1 && d->month <= 12 && d->week >= 1 && d->week <= 5 && d->wday <= 6
        && d->hour <= 23 && d->minute <= 59;
}

static int
put_date(char *buf, size_t size, const struct __xbox_tz_date *d)
{
    if (d->minute)
        return snprintf(buf, size, ",M%u.%u.%u/%u:%02u", d->month, d->week, d->wday, d->hour,
                        d->minute);
    return snprintf(buf, size, ",M%u.%u.%u/%u", d->month, d->week, d->wday, d->hour);
}

int
__xbox_tz_rule(const struct __xbox_tz_info *info, char *buf, size_t size)
{
    size_t pos = 0;
    int    n;
    int    dst = date_ok(&info->std_date) && date_ok(&info->dst_date);
    long   std_bias = dst ? info->bias + info->std_bias : info->bias;

#define PUT(expr)                                \
    do {                                         \
        n = (expr);                              \
        if (n < 0 || (size_t)n >= size - pos)    \
            return -1;                           \
        pos += (size_t)n;                        \
    } while (0)

    if (!buf || size == 0)
        return -1;

    PUT(put_name(buf + pos, size - pos, info->std_name, sizeof(info->std_name), std_bias));
    PUT(put_offset(buf + pos, size - pos, std_bias));

    if (dst) {
        long dst_bias = info->bias + info->dst_bias;
        PUT(put_name(buf + pos, size - pos, info->dst_name, sizeof(info->dst_name), dst_bias));
        PUT(put_offset(buf + pos, size - pos, dst_bias));
        /* POSIX lists the start of daylight time first. */
        PUT(put_date(buf + pos, size - pos, &info->dst_date));
        PUT(put_date(buf + pos, size - pos, &info->std_date));
    }

#undef PUT
    return (int)pos;
}
