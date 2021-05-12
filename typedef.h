/* ============================================================================================= */
#ifndef __TYPEDEF_H__

/* ============================================================================================= */
#define __TYPEDEF_H__

/* ============================================================================================= */
#ifndef SD
typedef unsigned int SD;
#endif
#ifndef S8
typedef char S8;
#endif
#ifndef U8
typedef unsigned char U8;
#endif
#ifndef U16
typedef unsigned short U16;
#endif
#ifndef U32
typedef unsigned int U32;
#endif
#ifndef BL
typedef unsigned int BL;
#endif
#ifndef HD
typedef void *HD;
#endif
#ifndef F32
typedef float F32;
#endif
#ifndef F64
typedef double F64;
#endif
#ifndef S16
typedef signed short S16;
#endif
#ifndef S32
typedef signed int S32;
#endif
#ifndef U64
typedef unsigned long long U64;
#endif
#ifndef S64
typedef long long S64;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef S_OK
#define S_OK 0
#endif
#ifndef S_FAIL
#define S_FAIL (SD)(-1)
#endif

/* ============================================================================================= */
#endif /* __TYPEDEF_H__ */

/* ============================================================================================= */
