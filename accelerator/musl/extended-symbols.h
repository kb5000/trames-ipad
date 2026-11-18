/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <locale.h>
#undef weak_alias
#define TRAMES_EXT_ALIAS(a,b) extern __typeof(a) b __attribute__((__weak__,__alias__(#a)))
#define weak_alias(a,b) TRAMES_EXT_ALIAS(a,b)
#define PRIVATE __attribute__((visibility("hidden")))
extern PRIVATE __typeof__(memmem) trames_orig_memmem;
#define memmem trames_orig_memmem
extern PRIVATE __typeof__(strstr) trames_orig_strstr;
#define strstr trames_orig_strstr
extern PRIVATE __typeof__(strcasestr) trames_orig_strcasestr;
#define strcasestr trames_orig_strcasestr
extern PRIVATE __typeof__(strspn) trames_orig_strspn;
#define strspn trames_orig_strspn
extern PRIVATE __typeof__(strcspn) trames_orig_strcspn;
#define strcspn trames_orig_strcspn
extern PRIVATE __typeof__(strpbrk) trames_orig_strpbrk;
#define strpbrk trames_orig_strpbrk
extern PRIVATE __typeof__(strcasecmp) trames_orig_strcasecmp;
#define strcasecmp trames_orig_strcasecmp
extern PRIVATE __typeof__(strncasecmp) trames_orig_strncasecmp;
#define strncasecmp trames_orig_strncasecmp
extern PRIVATE __typeof__(mempcpy) trames_orig_mempcpy;
#define mempcpy trames_orig_mempcpy
extern PRIVATE __typeof__(strlcpy) trames_orig_strlcpy;
#define strlcpy trames_orig_strlcpy
extern PRIVATE __typeof__(strlcat) trames_orig_strlcat;
#define strlcat trames_orig_strlcat
extern PRIVATE __typeof__(atoi) trames_orig_atoi;
#define atoi trames_orig_atoi
extern PRIVATE __typeof__(atol) trames_orig_atol;
#define atol trames_orig_atol
extern PRIVATE __typeof__(atoll) trames_orig_atoll;
#define atoll trames_orig_atoll
extern PRIVATE int trames_orig___strcasecmp_l(const char *,const char *,locale_t);
extern PRIVATE int trames_orig___strncasecmp_l(const char *,const char *,size_t,locale_t);
extern PRIVATE __typeof__(strcasecmp_l) trames_orig_strcasecmp_l;
extern PRIVATE __typeof__(strncasecmp_l) trames_orig_strncasecmp_l;
#define __strcasecmp_l trames_orig___strcasecmp_l
#define __strncasecmp_l trames_orig___strncasecmp_l
#define strcasecmp_l trames_orig_strcasecmp_l
#define strncasecmp_l trames_orig_strncasecmp_l
