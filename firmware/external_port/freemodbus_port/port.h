#ifndef FREEMODBUS_PORT_H
#define FREEMODBUS_PORT_H

#include <rtthread.h>
#include <rthw.h>
#include <stdint.h>

#include "mbconfig.h"

#define INLINE
#define PR_BEGIN_EXTERN_C           extern "C" {
#define PR_END_EXTERN_C             }

#define ENTER_CRITICAL_SECTION()    EnterCriticalSection()
#define EXIT_CRITICAL_SECTION()     ExitCriticalSection()

typedef uint8_t BOOL;
typedef unsigned char UCHAR;
typedef char CHAR;
typedef uint16_t USHORT;
typedef int16_t SHORT;
typedef uint32_t ULONG;
typedef int32_t LONG;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void EnterCriticalSection(void);
void ExitCriticalSection(void);

#endif
