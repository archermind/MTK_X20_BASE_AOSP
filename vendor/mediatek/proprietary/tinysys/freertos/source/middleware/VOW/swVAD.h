#ifndef _SWVAD_H
#define _SWVAD_H

int  returnToPhase1BySwVad(short* inData, int kwsResult);
void initSwVad();
void resetSwVadPara();

#define FRAMELEN				160
#define INIT_NOISE_LEN		     16  // 16 frames --> shift 4 bits
#define NOISE_EST_LEN             3
#define SIGNAL_EST_LEN			  9
#define SIGNAL_EST_BUFFER_LEN    ((SIGNAL_EST_LEN << 1) + 1)
#define NOISE_EST_BUFFER_LEN     ((NOISE_EST_LEN << 1) + 1)

#define VAD_THD                  45  // floor(1.414*32=45.2) --> set 45 as THD;  (3dB=1.414)
#define FORCE_NOISEUPDATE_THD1  400
#define FORCE_NOISEUPDATE_THD2  250
#define BK2PHASE1_NOISE_THD     150
#define BK2PHASE1_KWS_THD       150

#define abs(a)  (a > 0)? a : -a

struct DATABUFFER_INFO {
    int dataRing[FRAMELEN];
};

struct DCREMOVAL_INFO {
    int preX;
    int preY;
};

struct VAD_INFO {
    int frameIndex;
    int noiseCount;
    int speechCount;
    int vadFlag;
    int currVadPos;
    int noiseRef;

    int signalRing[SIGNAL_EST_BUFFER_LEN];
};

#endif
