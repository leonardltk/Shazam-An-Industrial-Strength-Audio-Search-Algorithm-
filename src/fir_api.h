// ============================================================================================= //
#ifndef __FIR_API_H__
#define __FIR_API_H__

// ============================================================================================= //
#include "typedef.h"

// ============================================================================================= //
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
// #ifndef S_OK
// #define S_OK 0
// #end
#ifndef S_FAIL
#define S_FAIL (SD)(-1)
#endif

// ============================================================================================= //
typedef struct
{
    U16 dwTapNum;
    S16 *pswState;
    const S16 *pswCoeffs;
} FIR_Q15_INSTANCE;
#define FIR_Q15_QBIT 2

// ============================================================================================= //
#define __SIMD32(addr)  (*(S32 **) & (addr))
#define  _SIMD32_OFFSET(addr)  (*( S32 * )   (addr))
#define __PKHBT(ARG1, ARG2, ARG3)      ( (((S32)(ARG1) <<  0) & (S32)0x0000FFFF) | \
    (((S32)(ARG2) << ARG3) & (S32)0xFFFF0000)  )
static __inline S32 __SMLADX(S32 x, S32 y, S32 sum)
{
#ifndef ARM_USE
    return (sum + ((short) (x >> 16) * (short) (y)) + ((short) x * (short) (y >> 16)));
#else
    S32 sdwOut;
    __asm
    {
        SMLADX sdwOut, x, y, sum;
    }
    return sdwOut;
#endif /* ARM_USE */
}
static __inline S32 __SMLAD(S32 x, S32 y, S32 sum)
{
#ifndef ARM_USE
    return (sum + ((short) (x >> 16) * (short) (y >> 16)) + ((short) x * (short) y));
#else
    S32 sdwOut;
    __asm
    {
        SMLAD sdwOut, x, y, sum;
    }
    return sdwOut;
#endif /* ARM_USE */
}
static __inline S32 __SSAT(S32 x, U32 y)
{
#ifndef ARM_USE
    S32 posMax, negMin;
    U32 i;

    posMax = 1;
    for (i = 0; i < (y - 1); i++)
    {
        posMax = posMax * 2;
    }

    if(x > 0)
    {
        posMax = (posMax - 1);

        if(x > posMax)
        {
            x = posMax;
        }
    }
    else
    {
        negMin = -posMax;

        if(x < negMin)
        {
            x = negMin;
        }
    }
    return (x);
#else
    S32 sdwOut;
    __asm
    {
        SSAT sdwOut, #16, x, lsl #0;
    }
    return sdwOut;
#endif /* ARM_USE */
}

// ============================================================================================= //
#if (defined(ARM_NEON_USE) && (defined(BKF_NEON_USE) || defined(STE_FIR_NEON_USE)))
void FIR_Q15_90tap(const FIR_Q15_INSTANCE * S, S16 * pswIn, S16 * pswOut, U32 dwFrameSz);
void FIR_Q15_36tap(const FIR_Q15_INSTANCE * S, S16 * pswIn, S16 * pswOut, U32 dwFrameSz);
void FIR_Q15_12tap(const FIR_Q15_INSTANCE * S, S16 * pswIn, S16 * pswOut, U32 dwFrameSz);
#endif // (defined(ARM_NEON_USE) && (defined(BKF_NEON_USE) || defined(STE_FIR_NEON_USE)))

// ============================================================================================= //
void FIR_Q15(const FIR_Q15_INSTANCE * S, S16 * pswIn, S16 * pswOut, U32 dwFrameSz);
SD FIR_Q15_INIT(FIR_Q15_INSTANCE * S, U16 dwTapNum, const S16 * pswCoeffs, S16 * pswState, U32 dwFrameSz);

// ============================================================================================= //
#endif // __FIR_API_H__

// ============================================================================================= //
