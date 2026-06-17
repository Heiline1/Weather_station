#ifndef DATATYPE_H
#define DATATYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint8_t      u8;
typedef uint16_t     u16;
typedef uint32_t     u32;
typedef int8_t       i8;
typedef int16_t      i16;
typedef int32_t      i32;
#ifdef HAS_INT64
typedef uint64_t     u64;
typedef int64_t      i64;
#endif

typedef float        f32;
typedef double       f64;
typedef size_t       usize;

#define bits_get(bits, n)          (((bits) >> (n)) & 0x01)
#define bits_set(bits, n, val)     ((bits) = ((bits) & ~(1 << (n))) | ((val) << (n)))
#define bits_toggle(bits, n)       ((bits) ^= (1 << (n)))
#define bits_mask(n)               (1U << (n))
#define bits_mask_low(n)           ((1U << (n)) - 1)
#define bits_check_bit(bits, n)    (((bits) & (1U << (n))) != 0)

#define array_size(arr)            (sizeof(arr) / sizeof((arr)[0]))
#define unused_param(param)        (void)(param)

#endif