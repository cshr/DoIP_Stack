/**
 * Copyright @ 2019 iAuto (Shanghai) Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAuto (Shanghai) Co., Ltd.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio.h>
#include "ne_doip_util.h"

DOIP_EXPORT void
ne_doip_list_init(ne_doip_list_t *list)
{
    list->prev = list;
    list->next = list;
}

DOIP_EXPORT void
ne_doip_list_insert(ne_doip_list_t *list, ne_doip_list_t *elm)
{
    elm->prev = list;
    elm->next = list->next;
    list->next = elm;
    elm->next->prev = elm;
}

DOIP_EXPORT void
ne_doip_list_remove(ne_doip_list_t *elm)
{
    elm->prev->next = elm->next;
    elm->next->prev = elm->prev;
    elm->next = NULL;
    elm->prev = NULL;
}

DOIP_EXPORT uint8_t
ne_doip_list_length(const ne_doip_list_t *list)
{
    ne_doip_list_t *e = NULL;
    int count;

    count = 0;
    e = list->next;
    while (e != list) {
        e = e->next;
        count++;
    }

    return count;
}

DOIP_EXPORT int
ne_doip_list_empty(const ne_doip_list_t *list)
{
    return list->next == list;
}

DOIP_EXPORT void
ne_doip_list_insert_list(ne_doip_list_t *list, ne_doip_list_t *other)
{
    if (ne_doip_list_empty(other)) {
        return;
    }

    other->next->prev = list;
    other->prev->next = list->next;
    list->next->prev = other->prev;
    list->next = other->next;
}

DOIP_EXPORT void
ne_doip_get_timespec(struct timespec *time, int msec)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    time->tv_sec = now.tv_sec + msec / 1000;
    time->tv_nsec = now.tv_nsec + (msec % 1000) * 1000000;

    // bigger than one second
    if (time->tv_nsec >= 1000000000) {
        time->tv_sec++;
        time->tv_nsec -= 1000000000;
    }
}

DOIP_EXPORT void
ne_doip_set_timespec(struct timespec *time, int msec)
{
    time->tv_sec += msec / 1000;
    time->tv_nsec += (msec % 1000) * 1000000;

    // bigger than one second
    if (time->tv_nsec >= 1000000000) {
        time->tv_sec++;
        time->tv_nsec -= 1000000000;
    }
}

DOIP_EXPORT long
ne_doip_subtimespec(struct timespec *time1, struct timespec *time2)
{
    long ms = time1->tv_sec * 1000 + time1->tv_nsec / 1000000
        - (time2->tv_sec * 1000 + time2->tv_nsec / 1000000);

    return ms;
}

DOIP_EXPORT unsigned int
ne_doip_get_tick_cnt()
{
    struct timespec time;
    ne_doip_get_timespec(&time, 0);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

DOIP_EXPORT unsigned long
ne_doip_get_tick_cnt_us()
{
    struct timespec time;
    ne_doip_get_timespec(&time, 0);
    return time.tv_sec * 1000 * 1000 + time.tv_nsec / 1000;
}

// uint16_t big end <-> small end conversion
DOIP_EXPORT uint16_t
ne_doip_bswap_16(uint16_t x)
{
    return (((uint16_t)(x) & 0x00ff) << 8) | \
           (((uint16_t)(x) & 0xff00) >> 8) ;
}

// uint32_t big end <-> small end conversion
DOIP_EXPORT uint32_t
ne_doip_bswap_32(uint32_t x)
{
    return (((uint32_t)(x) & 0xff000000) >> 24) | \
           (((uint32_t)(x) & 0x00ff0000) >> 8) | \
           (((uint32_t)(x) & 0x0000ff00) << 8) | \
           (((uint32_t)(x) & 0x000000ff) << 24) ;
}

// Convert a MAC address in the form of a string to a 6-byte structure
DOIP_EXPORT void
ne_doip_convert_mac(char *str_out, const char *str_in)
{
    if (NULL == str_out || NULL == str_in) {
        return;
    }
    uint32_t n;
    uint8_t m = 0;
    // Every two characters are stored as a numeric value
    for (n = 0; n < strlen(str_in); n += 2) {
        uint8_t sum;
        // n  0 2 4 6....
        if (str_in[n] >= 'A' && str_in[n] <= 'F') {
            sum = (str_in[n] - 'A' + 10) * 16;
        }
        else {
            sum = (str_in[n] - '0') * 16;
        }
        // n+1   1 3 5 7....
        if (str_in[n + 1] >= 'A' && str_in[n + 1] <= 'F') {
            sum += str_in[n + 1] - 'A' + 10;
        }
        else {
            sum += str_in[n + 1] - '0';
        }

        str_out[m] = sum;
        m++;
    }
}

DOIP_EXPORT void
ne_doip_log_output(const char* func_name, char* file_name, int line_num, const char* fmt, ...)
{
    va_list pvar;
    va_start(pvar, fmt);
    char buf[1501] = { 0 };
    vsnprintf(buf, 1501 -1, fmt, pvar);
    va_end(pvar);

    char buf_time[100] = { 0 };
    int size = 100;
    tzset();
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *timeinfo = localtime(&ts.tv_sec);

    snprintf(buf_time, size, "[%02d-%02d %02d:%02d:%02d.%05ld line:%04d]",
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec,
            ts.tv_nsec / 10000,
            line_num);

    printf("%s  %s", buf_time, buf);
}

DOIP_EXPORT void
ne_doip_printf_time(int line)
{
    char buff[100] = { 0 };
    int size = 100;
    tzset();
    struct timespec tv = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &tv);
    time_t timep = tv.tv_sec;
    struct tm ti_ptr;
    localtime_r(&timep, &ti_ptr);
    snprintf(buff, size, "[%04d/%02d/%02d %02d:%02d:%02d.%05ld line:%04d]",
    ti_ptr.tm_year + 1900,
    ti_ptr.tm_mon + 1,
    ti_ptr.tm_mday,
    ti_ptr.tm_hour,
    ti_ptr.tm_min,
    ti_ptr.tm_sec,
    tv.tv_nsec / 10000,
    line);
    printf("%s  ", buff);
    // printf("%s %04u] ", buff, __LINE__);
}
/* EOF */