#ifndef __MCU_H__
#define __MCU_H__
#include <stdint.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

typedef unsigned char      __u8;
typedef unsigned short     __16;
typedef unsigned long      __u32;
typedef unsigned long long __u64;

typedef struct MCU_Handler {
  int MCU_driver_handle;
} MCU_Handler;

#endif
