#include <stdio.h>
#include <FreeRTOS.h>
#include <stdlib.h>
#include <string.h>
#include "swVAD.h"

struct VAD_INFO VadInfo;
struct DCREMOVAL_INFO DcRemovalPara;
struct DATABUFFER_INFO DataBuffer;

void dataInit()
{
    memset(DataBuffer.dataRing, 0, sizeof(int)*FRAMELEN);
    return;
}

void dcRemovalInit()
{
    DcRemovalPara.preX = 0;
    DcRemovalPara.preY = 0;
    return;
}

void vadInit()
{
    VadInfo.frameIndex = 0;
    VadInfo.speechCount = 0;
    VadInfo.noiseCount = 0;
    VadInfo.vadFlag = 0;
    VadInfo.currVadPos = 0;
    VadInfo.noiseRef = 0;
    memset(VadInfo.signalRing, 0, sizeof(float) * SIGNAL_EST_BUFFER_LEN);
    return;
}

void dcRemoval()
{
    int i;
    int temp;
    for(i = 0; i < FRAMELEN ; i++) {
        temp = DcRemovalPara.preY - (DcRemovalPara.preY >> 6) - DcRemovalPara.preX;    // y(n) = x(n)- x(n-1) + (1-1/64)*y(n-1); -3dB@39.5Hz
        DcRemovalPara.preX = DataBuffer.dataRing[i];
        DataBuffer.dataRing[i] = DataBuffer.dataRing[i] + temp;
        DcRemovalPara.preY = DataBuffer.dataRing[i];
    }
    return;
}


void noiseRefUpdate()
{
    long long temp64;
    int i;
    int noiseCurr;

    temp64 = 0;
    for (i = -NOISE_EST_LEN; i <= NOISE_EST_LEN; i++) {
        temp64 += (long long) VadInfo.signalRing[(VadInfo.frameIndex - SIGNAL_EST_LEN + i) % SIGNAL_EST_BUFFER_LEN];
    }
    noiseCurr= (int)(temp64 / NOISE_EST_BUFFER_LEN);

    VadInfo.noiseRef = VadInfo.noiseRef + ((noiseCurr - VadInfo.noiseRef) >> 5);  // N'= (1-1/32)N+ (1/32)Ncurr; 1/32=0.0313
    return;
}

void noiseRefMinimumTracking()
{
    int i;
    int noiseCurr;
    const int minimumTrackLen = 6;

    noiseCurr = 32767 << 15;
    for (i = -minimumTrackLen; i <= minimumTrackLen; i++) {
        if (VadInfo.signalRing[(VadInfo.frameIndex-SIGNAL_EST_LEN + i) % SIGNAL_EST_BUFFER_LEN] < noiseCurr) {
            noiseCurr = VadInfo.signalRing[(VadInfo.frameIndex - SIGNAL_EST_LEN + i) % SIGNAL_EST_BUFFER_LEN];
        }
    }
    VadInfo.noiseRef = VadInfo.noiseRef + ((noiseCurr - VadInfo.noiseRef) >> 5);  // N'= (1-1/32)N+ (1/32)Ncurr; 1/32=0.0313
    return;
}

void frameBasedVAD()
{
    int i = 0;
    int frameAbs;
    int bufferAbs;
    int snrValue;
    long long temp64;

    temp64 = 0;
    for (i = 0; i < FRAMELEN; i++) {
        temp64 += (long long) (abs(DataBuffer.dataRing[i])); // 18.13
    }
    frameAbs = (int) (temp64 / FRAMELEN);

    if (VadInfo.frameIndex < INIT_NOISE_LEN) {
//		VadInfo.noiseRef += (frameAbs/INIT_NOISE_LEN);
        VadInfo.noiseRef += (frameAbs >> 4); //16 frames --> shift 4 bits
    }

    VadInfo.signalRing[VadInfo.frameIndex%SIGNAL_EST_BUFFER_LEN] = frameAbs;

    if (VadInfo.frameIndex > SIGNAL_EST_BUFFER_LEN) {
        temp64 = 0;
        for (i = 0; i < SIGNAL_EST_BUFFER_LEN; i++) {
            temp64 += ((long long) VadInfo.signalRing[i]);
        }
        bufferAbs = (int)(temp64 / SIGNAL_EST_BUFFER_LEN);
        snrValue = bufferAbs / ((VadInfo.noiseRef >> 5) + 1);  // rescale SNR values to easy set THD

        if (snrValue >= VAD_THD) {
            VadInfo.vadFlag = 1;
            VadInfo.speechCount++;
            VadInfo.noiseCount = 0;
        } else {
            VadInfo.vadFlag = 0;
            VadInfo.speechCount = 0;
            VadInfo.noiseCount++;
            noiseRefUpdate();
        }
    }

    if (VadInfo.speechCount >= FORCE_NOISEUPDATE_THD1) {
        noiseRefUpdate();
    } else if (VadInfo.speechCount >= FORCE_NOISEUPDATE_THD2) {
        noiseRefMinimumTracking();
    }
}

int back2Phase1Decision(int noKeywordCount)
{
    int bk2Phase1Flag = 0;

    if ((VadInfo.noiseCount > BK2PHASE1_NOISE_THD) && (noKeywordCount > BK2PHASE1_KWS_THD)) {
        bk2Phase1Flag = 1;
    }
    return bk2Phase1Flag;
}

void initSwVad()
{
    dataInit();
    dcRemovalInit();
    vadInit();
    return;
}

void resetSwVadPara()   // call this function when phase 2--> phase 1 --> phase 2
{
    VadInfo.speechCount = 0;
    VadInfo.noiseCount = 0;
    VadInfo.vadFlag = 0;
    return;
}

int  returnToPhase1BySwVad(short* inData, int noKeywordCount)
{
    int i;

    for (i=0; i< FRAMELEN; i++) {
        DataBuffer.dataRing[i] = ((int) inData[i]) << 13;
    }

    // remove low-frequency component. -3dB @39.5Hz
    dcRemoval();
    frameBasedVAD();
    VadInfo.frameIndex++;
    return back2Phase1Decision(noKeywordCount);
}
