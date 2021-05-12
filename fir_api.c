// ============================================================================================= //
#include <string.h>
#include "fir_api.h"

// ============================================================================================= //
SD FIR_Q15_INIT(FIR_Q15_INSTANCE * S, U16 dwTapNum, const S16 * pswCoeffs, S16 * pswState, U32 dwFrameSz)
{
    S->dwTapNum = dwTapNum;
    S->pswCoeffs = pswCoeffs;
    memset(pswState, 0, (dwTapNum + (dwFrameSz - 1u)) * sizeof(S16));
    S->pswState = pswState;
    return (0);
}

// ============================================================================================= //
void FIR_Q15(const FIR_Q15_INSTANCE *S, S16 *pswIn, S16 *pswOut, U32 dwFrameSz)
{
    S16 *pswState = S->pswState;
    S16 *pswStateTmp;                                     
    S16 *pswStateCurnt;

    const S16 *pswCoeffs = S->pswCoeffs;
    const S16 *pswCoeffsTmp;

    S32 sdwAcc0, sdwAcc1, sdwAcc2, sdwAcc3;                  
    S32 sdwX0, sdwX1, sdwX2, sdwC0;                          
    U32 dwTapNum = S->dwTapNum;                 

    U32 i1, i2, tapCnt;

    pswStateCurnt = &(S->pswState[(dwTapNum - 1)]);

    i1 = dwFrameSz >> 2;

    while(i1 > 0)
    {
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        sdwAcc0 = 0;
        sdwAcc1 = 0;
        sdwAcc2 = 0;
        sdwAcc3 = 0;

        pswStateTmp = pswState;

        pswCoeffsTmp = pswCoeffs;

        sdwX0 = *__SIMD32(pswStateTmp)++;

        sdwX2 = *__SIMD32(pswStateTmp)++;

        i2 = dwTapNum >> 2;
        do 
        {
            sdwC0 = *__SIMD32(pswCoeffsTmp)++;

            sdwAcc0 = __SMLAD(sdwX0, sdwC0, sdwAcc0);
            sdwAcc2 = __SMLAD(sdwX2, sdwC0, sdwAcc2);

            sdwX1 = __PKHBT(sdwX2, sdwX0, 0);
            sdwAcc1 = __SMLADX(sdwX1, sdwC0, sdwAcc1);

            sdwX0 = _SIMD32_OFFSET(pswStateTmp);

            sdwX1 = __PKHBT(sdwX0, sdwX2, 0);
            sdwAcc3 = __SMLADX(sdwX1, sdwC0, sdwAcc3);

            sdwC0 = *__SIMD32(pswCoeffsTmp)++;

            sdwAcc0 = __SMLAD(sdwX2, sdwC0, sdwAcc0);

            sdwX2 = _SIMD32_OFFSET(pswStateTmp + 2u);

            sdwAcc2 = __SMLAD(sdwX0, sdwC0, sdwAcc2);
            sdwAcc1 = __SMLADX(sdwX1, sdwC0, sdwAcc1);

            sdwX1 = __PKHBT(sdwX2, sdwX0, 0);
            sdwAcc3 = __SMLADX(sdwX1, sdwC0, sdwAcc3);

            pswStateTmp += 4u;

            i2 --;
        } while(i2 > 0);

        if((dwTapNum & 0x3u) != 0)
        {
            sdwC0 = *__SIMD32(pswCoeffsTmp)++;
            
            sdwAcc0 = __SMLAD(sdwX0, sdwC0, sdwAcc0);
            sdwAcc2 = __SMLAD(sdwX2, sdwC0, sdwAcc2);

            sdwX1 = __PKHBT(sdwX2, sdwX0, 0);

            sdwX0 = *__SIMD32(pswStateTmp);

            sdwAcc1 = __SMLADX(sdwX1, sdwC0, sdwAcc1);

            sdwX1 = __PKHBT(sdwX0, sdwX2, 0);

            sdwAcc3 = __SMLADX(sdwX1, sdwC0, sdwAcc3);
        }

        sdwAcc0 += 0x00004000;
        sdwAcc1 += 0x00004000;
        sdwAcc2 += 0x00004000;
        sdwAcc3 += 0x00004000;
        *__SIMD32(pswOut)++ = __PKHBT(__SSAT((sdwAcc0 >> (15-FIR_Q15_QBIT)), 16), __SSAT((sdwAcc1 >> (15-FIR_Q15_QBIT)), 16), 16);
        *__SIMD32(pswOut)++ = __PKHBT(__SSAT((sdwAcc2 >> (15-FIR_Q15_QBIT)), 16), __SSAT((sdwAcc3 >> (15-FIR_Q15_QBIT)), 16), 16);

        pswState = pswState + 4u;

        i1 --;
    }

    i1 = dwFrameSz % 0x4u;
    while (i1 > 0)
    {
        *pswStateCurnt++ = *pswIn++;

        sdwAcc0 = 0;

        pswStateTmp = pswState;
        pswCoeffsTmp = pswCoeffs;

        i2 = dwTapNum >> 1;
        do
        {
            sdwAcc0 += (S32)*pswStateTmp++ * *pswCoeffsTmp++;
            sdwAcc0 += (S32)*pswStateTmp++ * *pswCoeffsTmp++;

            i2 --;
        } while(i2 > 0);

        sdwAcc0 += 0x00004000; 
        *pswOut++ = (S16) (__SSAT((sdwAcc0 >> (15-FIR_Q15_QBIT)), 16));

        pswState = pswState + 1;

        i1 --;
    }

    pswStateCurnt = S->pswState;

    tapCnt = (dwTapNum - 1) >> 2;

    while (tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }

    tapCnt = (dwTapNum - 1) % 0x4u;

    while(tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }
}

// ============================================================================================= //
#ifdef ARM_NEON_USE

// ============================================================================================= //
void FIR_Q15_90tap(const FIR_Q15_INSTANCE *S, S16 *pswIn, S16 *pswOut, U32 dwFrameSz)
{
    // Coef
    S32 sdwCoef8889;
    S32 sdw8889;

    S16 *pswState = S->pswState;
    S16 *pswStateTmp;
    S16 *pswStateCurnt;

    const S16 *pswCoeffs = S->pswCoeffs;
    const S16 *pswCoeffsTmp;

    U32 dwTapNum = S->dwTapNum;                 

    U32 i1, tapCnt;

    int16x4_t Coeffs_16x4[22];
    int16x4_t StateCurnt_16x4;

    // sample 0
    int32x4_t Acc00_32x4;
    int32_t Acc00_s32;

    // sample 1
    int32x4_t Acc01_32x4;
    int32_t Acc01_s32;

    int64x2_t Acc00_64x2, Acc01_64x2;
    int32x2_t Acc00_32x2, Acc01_32x2;
    int64x1_t Acc00_s64x1, Acc01_s64x1;
    S64 sqwAcc00, sqwAcc01;

    pswCoeffsTmp = pswCoeffs;
    Coeffs_16x4[ 0] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 1] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 2] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 3] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 4] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 5] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 6] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 7] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 8] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 9] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[10] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[11] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[12] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[13] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[14] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[15] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[16] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[17] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[18] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[19] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[20] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[21] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    //sdwCoef8889 = *__SIMD32(pswCoeffsTmp)++;
    {
        S16 swTmp00 = *pswCoeffsTmp++;
        S16 swTmp01 = *pswCoeffsTmp++;

        sdwCoef8889 = ((swTmp00<<16) | ((swTmp01) & 0xFFFF));
    }

    // Pre
    pswStateCurnt = &(S->pswState[(dwTapNum - 1)]);
    pswStateTmp = pswState;
    i1 = dwFrameSz;

    //////////////////////////////////////////////////////////////////////////
    // sample 0
    //////////////////////////////////////////////////////////////////////////
    // Load
    *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

    // Process: 0~87
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 9]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[10]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[11]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[12]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[13]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[14]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[15]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[16]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[17]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[18]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[19]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[20]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[21]);
    //sdw8889 = *__SIMD32(pswStateTmp)++;

    {
        S16 swTmp00 = *pswStateTmp++;
        S16 swTmp01 = *pswStateTmp++;

        sdw8889 = ((swTmp00<<16) | ((swTmp01) & 0xFFFF));
    }

    // Post
    pswState ++;
    pswStateTmp = pswState;
    i1 -= 1;

    //////////////////////////////////////////////////////////////////////////
    // sample 1
    //////////////////////////////////////////////////////////////////////////
    // Load
    *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

    // Process: 0~87
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);
    Acc00_64x2 = vpaddlq_s32(Acc00_32x4); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);
    Acc00_32x2 = vmovn_s64(Acc00_64x2); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);
    Acc00_s64x1 = vpaddl_s32(Acc00_32x2); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);
    sqwAcc00 = vget_lane_s64(Acc00_s64x1, 0); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);
    Acc00_s32 = (int32_t)sqwAcc00 + 0x00004000; // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 9]);
    Acc00_s32 = __SMLAD(sdw8889, sdwCoef8889, Acc00_s32); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[10]);
    Acc00_s32 >>= (15-FIR_Q15_QBIT); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[11]);
    *pswOut ++ = (S16)__SSAT(Acc00_s32, 16); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[12]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[13]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[14]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[15]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[16]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[17]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[18]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[19]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[20]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[21]);

    //sdw8889 = *__SIMD32(pswStateTmp)++;
    {
        S16 swTmp00 = *pswStateTmp++;
        S16 swTmp01 = *pswStateTmp++;

        sdw8889 = ((swTmp00<<16) | ((swTmp01) & 0xFFFF));
    }

    // Post
    pswState ++;
    pswStateTmp = pswState;
    i1 -= 1;

    //////////////////////////////////////////////////////////////////////////
    // Sample 2~959
    //////////////////////////////////////////////////////////////////////////
    do
    {
        //////////////////////////////////////////////////////////////////////////
        // sample 0
        //////////////////////////////////////////////////////////////////////////
        // Load
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        // Process: 0~87
        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);

        Acc01_64x2 = vpaddlq_s32(Acc01_32x4); // last sample
        Acc01_32x2 = vmovn_s64(Acc01_64x2); // last sample
        Acc01_s64x1 = vpaddl_s32(Acc01_32x2); // last sample
        sqwAcc01 = vget_lane_s64(Acc01_s64x1, 0); // last sample
        Acc01_s32 = (int32_t)sqwAcc01 + 0x00004000; // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 9]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[10]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[11]);

        Acc01_s32 = __SMLAD(sdw8889, sdwCoef8889, Acc01_s32); // last sample
        Acc01_s32 >>= (15-FIR_Q15_QBIT); // last sample
        *pswOut ++ = (S16)__SSAT(Acc01_s32, 16); // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[12]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[13]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[14]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[15]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[16]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[17]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[18]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[19]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[20]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = L_mac_16x4(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[21]);

        //sdw8889 = *__SIMD32(pswStateTmp)++;
        {
            S16 swTmp00 = *pswStateTmp++;
            S16 swTmp01 = *pswStateTmp++;

            sdw8889 = ((swTmp00<<16) | ((swTmp01) & 0xFFFF));
        }

        // Post
        pswState ++;
        pswStateTmp = pswState;
        i1 -= 1;

        //////////////////////////////////////////////////////////////////////////
        // sample 1
        //////////////////////////////////////////////////////////////////////////
        // Load
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        // Process: 0~87
        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);

        Acc00_64x2 = vpaddlq_s32(Acc00_32x4);  // last sample
        Acc00_32x2 = vmovn_s64(Acc00_64x2);  // last sample
        Acc00_s64x1 = vpaddl_s32(Acc00_32x2);  // last sample
        sqwAcc00 = vget_lane_s64(Acc00_s64x1, 0);  // last sample
        Acc00_s32 = (int32_t)sqwAcc00 + 0x00004000;  // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 9]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[10]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[11]);

        Acc00_s32 = __SMLAD(sdw8889, sdwCoef8889, Acc00_s32);  // last sample
        Acc00_s32 >>= (15-FIR_Q15_QBIT);  // last sample
        *pswOut ++ = (S16)__SSAT(Acc00_s32, 16);  // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[12]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[13]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[14]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[15]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[16]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[17]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[18]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[19]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[20]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = L_mac_16x4(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[21]);

        //sdw8889 = *__SIMD32(pswStateTmp)++;
        {
            S16 swTmp00 = *pswStateTmp++;
            S16 swTmp01 = *pswStateTmp++;

            sdw8889 = ((swTmp00<<16) | ((swTmp01) & 0xFFFF));
        }

        // Post
        pswState ++;
        pswStateTmp = pswState;
        i1 -= 1;
    } while(i1 > 0);

    // Store sample 959
    Acc01_64x2 = vpaddlq_s32(Acc01_32x4); // last sample
    Acc01_32x2 = vmovn_s64(Acc01_64x2); // last sample
    Acc01_s64x1 = vpaddl_s32(Acc01_32x2); // last sample
    sqwAcc01 = vget_lane_s64(Acc01_s64x1, 0); // last sample
    Acc01_s32 = (int32_t)sqwAcc01 + 0x00004000; // last sample
    Acc01_s32 = __SMLAD(sdw8889, sdwCoef8889, Acc01_s32); // last sample
    Acc01_s32 >>= (15-FIR_Q15_QBIT); // last sample
    *pswOut ++ = (S16)__SSAT(Acc01_s32, 16); // last sample

    pswStateCurnt = S->pswState;

    tapCnt = (dwTapNum - 1) >> 2;

    while (tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }

    tapCnt = (dwTapNum - 1) % 0x4u;

    while(tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }
}

// ============================================================================================= //
void FIR_Q15_36tap(const FIR_Q15_INSTANCE *S, S16 *pswIn, S16 *pswOut, U32 dwFrameSz)
{
    S16 *pswState = S->pswState;
    S16 *pswStateTmp;                                     
    S16 *pswStateCurnt;

    const S16 *pswCoeffs = S->pswCoeffs;
    const S16 *pswCoeffsTmp;

    U32 dwTapNum = S->dwTapNum;                 

    U32 i1, tapCnt;

    int16x4_t Coeffs_16x4[9];
    int16x4_t StateCurnt_16x4;

    // sample 0
    int32x4_t Acc00_32x4;
    int32_t Acc00_s32;

    // sample 1
    int32x4_t Acc01_32x4;
    int32_t Acc01_s32;

    int64x2_t Acc00_64x2, Acc01_64x2;
    int32x2_t Acc00_32x2, Acc01_32x2;
    int64x1_t Acc00_s64x1, Acc01_s64x1;
    S64 sqwAcc00, sqwAcc01;

    pswCoeffsTmp = pswCoeffs;
    Coeffs_16x4[ 0] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 1] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 2] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 3] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 4] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 5] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 6] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 7] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 8] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;

    pswStateCurnt = &(S->pswState[(dwTapNum - 1)]);

    // Pre
    pswStateTmp = pswState;
    i1 = dwFrameSz;

    //////////////////////////////////////////////////////////////////////////
    // sample 0
    //////////////////////////////////////////////////////////////////////////
    // Load
    *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

    // Process: 0~87
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);

    // Post
    pswState ++;
    pswStateTmp = pswState;
    i1 -= 1;

    //////////////////////////////////////////////////////////////////////////
    // sample 1
    //////////////////////////////////////////////////////////////////////////
    // Load
    *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

    // Process: 0~87
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

    Acc00_64x2 = vpaddlq_s32(Acc00_32x4); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

    Acc00_32x2 = vmovn_s64(Acc00_64x2); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

    Acc00_s64x1 = vpaddl_s32(Acc00_32x2); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);

    sqwAcc00 = vget_lane_s64(Acc00_s64x1, 0); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);

    Acc00_s32 = (int32_t)sqwAcc00 + 0x00004000; // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);

    Acc00_s32 >>= (15-FIR_Q15_QBIT); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);

    *pswOut ++ = (S16)__SSAT(Acc00_s32, 16); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);

    // Post
    pswState ++;
    pswStateTmp = pswState;
    i1 -= 1;

    //////////////////////////////////////////////////////////////////////////
    // Sample 2~959
    //////////////////////////////////////////////////////////////////////////
    do
    {
        //////////////////////////////////////////////////////////////////////////
        // sample 0
        //////////////////////////////////////////////////////////////////////////
        // Load
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        // Process: 0~87
        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

        Acc01_64x2 = vpaddlq_s32(Acc01_32x4); // last sample
        Acc01_32x2 = vmovn_s64(Acc01_64x2); // last sample
        Acc01_s64x1 = vpaddl_s32(Acc01_32x2); // last sample
        sqwAcc01 = vget_lane_s64(Acc01_s64x1, 0); // last sample
        Acc01_s32 = (int32_t)sqwAcc01 + 0x00004000; // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);

        Acc01_s32 >>= (15-FIR_Q15_QBIT); // last sample
        *pswOut ++ = (S16)__SSAT(Acc01_s32, 16); // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);

        // Post
        pswState ++;
        pswStateTmp = pswState;
        i1 -= 1;

        //////////////////////////////////////////////////////////////////////////
        // sample 0
        //////////////////////////////////////////////////////////////////////////
        // Load
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        // Process: 0~87
        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

        Acc00_64x2 = vpaddlq_s32(Acc00_32x4); // last sample
        Acc00_32x2 = vmovn_s64(Acc00_64x2); // last sample
        Acc00_s64x1 = vpaddl_s32(Acc00_32x2); // last sample
        sqwAcc00 = vget_lane_s64(Acc00_s64x1, 0); // last sample
        Acc00_s32 = (int32_t)sqwAcc00 + 0x00004000; // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 3]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 4]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 5]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 6]);

        Acc00_s32 >>= (15-FIR_Q15_QBIT); // last sample
        *pswOut ++ = (S16)__SSAT(Acc00_s32, 16); // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 7]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 8]);

        // Post
        pswState ++;
        pswStateTmp = pswState;
        i1 -= 1;
    } while(i1 > 0);

    // Store sample 959
    Acc01_64x2 = vpaddlq_s32(Acc01_32x4); // last sample
    Acc01_32x2 = vmovn_s64(Acc01_64x2); // last sample
    Acc01_s64x1 = vpaddl_s32(Acc01_32x2); // last sample
    sqwAcc01 = vget_lane_s64(Acc01_s64x1, 0); // last sample
    Acc01_s32 = (int32_t)sqwAcc01 + 0x00004000; // last sample
    Acc01_s32 >>= (15-FIR_Q15_QBIT); // last sample
    *pswOut ++ = (S16)__SSAT(Acc01_s32, 16); // last sample

    pswStateCurnt = S->pswState;

    tapCnt = (dwTapNum - 1) >> 2;

    while (tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }

    tapCnt = (dwTapNum - 1) % 0x4u;

    while(tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }
}

// ============================================================================================= //
void FIR_Q15_12tap(const FIR_Q15_INSTANCE *S, S16 *pswIn, S16 *pswOut, U32 dwFrameSz)
{
    S16 *pswState = S->pswState;
    S16 *pswStateTmp;                                     
    S16 *pswStateCurnt;

    const S16 *pswCoeffs = S->pswCoeffs;
    const S16 *pswCoeffsTmp;

    U32 dwTapNum = S->dwTapNum;                 

    U32 i1, tapCnt;

    int16x4_t Coeffs_16x4[9];
    int16x4_t StateCurnt_16x4;

    // sample 0
    int32x4_t Acc00_32x4;
    int32_t Acc00_s32;

    // sample 1
    int32x4_t Acc01_32x4;
    int32_t Acc01_s32;

    int64x2_t Acc00_64x2, Acc01_64x2;
    int32x2_t Acc00_32x2, Acc01_32x2;
    int64x1_t Acc00_s64x1, Acc01_s64x1;
    S64 sqwAcc00, sqwAcc01;

    pswCoeffsTmp = pswCoeffs;
    Coeffs_16x4[ 0] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 1] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;
    Coeffs_16x4[ 2] = vld1_s16(pswCoeffsTmp); pswCoeffsTmp += 4;

    pswStateCurnt = &(S->pswState[(dwTapNum - 1)]);

    // Pre
    pswStateTmp = pswState;
    i1 = dwFrameSz;

    //////////////////////////////////////////////////////////////////////////
    // sample 0
    //////////////////////////////////////////////////////////////////////////
    // Load
    *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

    // Process: 0~87
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

    // Post
    pswState ++;
    pswStateTmp = pswState;
    i1 -= 1;

    //////////////////////////////////////////////////////////////////////////
    // sample 1
    //////////////////////////////////////////////////////////////////////////
    // Load
    *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

    // Process: 0~87
    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

    Acc00_64x2 = vpaddlq_s32(Acc00_32x4); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

    Acc00_32x2 = vmovn_s64(Acc00_64x2); // last sample

    StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
    Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

    Acc00_s64x1 = vpaddl_s32(Acc00_32x2); // last sample

    sqwAcc00 = vget_lane_s64(Acc00_s64x1, 0); // last sample

    Acc00_s32 = (int32_t)sqwAcc00 + 0x00004000; // last sample

    Acc00_s32 >>= (15-FIR_Q15_QBIT); // last sample

    *pswOut ++ = (S16)__SSAT(Acc00_s32, 16); // last sample


    // Post
    pswState ++;
    pswStateTmp = pswState;
    i1 -= 1;

    //////////////////////////////////////////////////////////////////////////
    // Sample 2~959
    //////////////////////////////////////////////////////////////////////////
    do
    {
        //////////////////////////////////////////////////////////////////////////
        // sample 0
        //////////////////////////////////////////////////////////////////////////
        // Load
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        // Process: 0~87
        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

        Acc01_64x2 = vpaddlq_s32(Acc01_32x4); // last sample
        Acc01_32x2 = vmovn_s64(Acc01_64x2); // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc00_32x4 = vmlal_s16(Acc00_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

        Acc01_s64x1 = vpaddl_s32(Acc01_32x2); // last sample
        sqwAcc01 = vget_lane_s64(Acc01_s64x1, 0); // last sample
        Acc01_s32 = (int32_t)sqwAcc01 + 0x00004000; // last sample
        Acc01_s32 >>= (15-FIR_Q15_QBIT); // last sample
        *pswOut ++ = (S16)__SSAT(Acc01_s32, 16); // last sample

        // Post
        pswState ++;
        pswStateTmp = pswState;
        i1 -= 1;

        //////////////////////////////////////////////////////////////////////////
        // sample 0
        //////////////////////////////////////////////////////////////////////////
        // Load
        *pswStateCurnt++ = (*pswIn) >> 2; pswIn ++;

        // Process: 0~87
        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmull_s16(StateCurnt_16x4, Coeffs_16x4[ 0]);

        Acc00_64x2 = vpaddlq_s32(Acc00_32x4); // last sample
        Acc00_32x2 = vmovn_s64(Acc00_64x2); // last sample
        Acc00_s64x1 = vpaddl_s32(Acc00_32x2); // last sample
        sqwAcc00 = vget_lane_s64(Acc00_s64x1, 0); // last sample
        Acc00_s32 = (int32_t)sqwAcc00 + 0x00004000; // last sample

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 1]);

        StateCurnt_16x4 = vld1_s16(pswStateTmp); pswStateTmp += 4;
        Acc01_32x4 = vmlal_s16(Acc01_32x4, StateCurnt_16x4, Coeffs_16x4[ 2]);

        Acc00_s32 >>= (15-FIR_Q15_QBIT); // last sample
        *pswOut ++ = (S16)__SSAT(Acc00_s32, 16); // last sample

        // Post
        pswState ++;
        pswStateTmp = pswState;
        i1 -= 1;
    } while(i1 > 0);

    // Store sample 959
    Acc01_64x2 = vpaddlq_s32(Acc01_32x4); // last sample
    Acc01_32x2 = vmovn_s64(Acc01_64x2); // last sample
    Acc01_s64x1 = vpaddl_s32(Acc01_32x2); // last sample
    sqwAcc01 = vget_lane_s64(Acc01_s64x1, 0); // last sample
    Acc01_s32 = (int32_t)sqwAcc01 + 0x00004000; // last sample
    Acc01_s32 >>= (15-FIR_Q15_QBIT); // last sample
    *pswOut ++ = (S16)__SSAT(Acc01_s32, 16); // last sample

    pswStateCurnt = S->pswState;

    tapCnt = (dwTapNum - 1) >> 2;

    while (tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }

    tapCnt = (dwTapNum - 1) % 0x4u;

    while(tapCnt > 0)
    {
        *pswStateCurnt++ = *pswState++;

        tapCnt--;
    }
}

// ============================================================================================= //
#endif // ARM_NEON_USE

// ============================================================================================= //
