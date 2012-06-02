#ifndef _ASPRINTF_WORKAROUND_H_
#define _ASPRINTF_WORKAROUND_H_

#include <malloc.h>
#include <stdarg.h>

#define asprintf(p, format, ...) (*p = alloca(snprintf(NULL, 0, format, __VA_ARGS__) + 1), sprintf(*p, format, __VA_ARGS__))

#endif
