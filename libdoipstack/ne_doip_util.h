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
/**
 * @file ne_doip_test_util.h
 * @brief tool module
 */

#ifndef NE_DOIP_UTIL_H
#define NE_DOIP_UTIL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define DOIP_EXPORT __attribute__ ((visibility("default")))
#else
#define DOIP_EXPORT
#endif

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

#ifdef __GNUC__
#define  ne_doip_container_of(ptr, sample, member)                \
    (__typeof__(sample))((char *)(ptr)    -            \
         ((char *)&(sample)->member - (char *)(sample)))
#else
#define  ne_doip_container_of(ptr, sample, member)                \
    (void *)((char *)(ptr)    -                        \
         ((char *)&(sample)->member - (char *)(sample)))
#endif

#define ne_doip_list_for_each(pos, head, member)                \
    for (pos = 0, pos =  ne_doip_container_of((head)->next, pos, member);    \
         &pos->member != (head);                    \
         pos =  ne_doip_container_of(pos->member.next, pos, member))

#define ne_doip_list_for_each_safe(pos, tmp, head, member)            \
    for (pos = 0, tmp = 0,                         \
         pos =  ne_doip_container_of((head)->next, pos, member),        \
         tmp =  ne_doip_container_of((pos)->member.next, tmp, member);    \
         &pos->member != (head);                    \
         pos = tmp,                            \
         tmp =  ne_doip_container_of(pos->member.next, tmp, member))

#define ne_doip_list_for_each_reverse(pos, head, member)            \
    for (pos = 0, pos =  ne_doip_container_of((head)->prev, pos, member);    \
         &pos->member != (head);                    \
         pos =  ne_doip_container_of(pos->member.prev, pos, member))

#define ne_doip_list_for_each_reverse_safe(pos, tmp, head, member)        \
    for (pos = 0, tmp = 0,                         \
         pos =  ne_doip_container_of((head)->prev, pos, member),    \
         tmp =  ne_doip_container_of((pos)->member.prev, tmp, member);    \
         &pos->member != (head);                    \
         pos = tmp,                            \
         tmp =  ne_doip_container_of(pos->member.prev, tmp, member))

#define NE_DOIP_PRINT(...) ((void)ne_doip_log_output(__FUNCTION__, __FILE__, __LINE__, __VA_ARGS__))
#define NE_DOIP_PRINT_LARGE (ne_doip_printf_time(__LINE__), printf)

struct ne_doip_list
{
    struct ne_doip_list *prev;
    struct ne_doip_list *next;
};

typedef struct ne_doip_list ne_doip_list_t;

typedef int (*compare)(void *, void *);

void ne_doip_list_init(ne_doip_list_t *list);
void ne_doip_list_insert(ne_doip_list_t *list, ne_doip_list_t *elm);
void ne_doip_list_remove(ne_doip_list_t *elm);
uint8_t ne_doip_list_length(const ne_doip_list_t *list);
int ne_doip_list_empty(const ne_doip_list_t *list);
void ne_doip_list_insert_list(ne_doip_list_t *list, ne_doip_list_t *other);

long ne_doip_subtimespec(struct timespec *time1, struct timespec *time2);
void ne_doip_get_timespec(struct timespec *time, int msec);
void ne_doip_set_timespec(struct timespec *time, int msec);
unsigned int ne_doip_get_tick_cnt();
unsigned long doip_get_tick_cnt_ms();
uint16_t ne_doip_bswap_16(uint16_t x);
uint32_t ne_doip_bswap_32(uint32_t x);
void ne_doip_convert_mac(char *str_out, const char *str_in);

void ne_doip_log_output(const char* func_name, char* file_name, int line_num, const char* fmt, ...);
void ne_doip_printf_time(int line);

#ifdef  __cplusplus
}
#endif

#endif // NE_DOIP_UTIL_H
/* EOF */