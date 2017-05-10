#ifndef __DP_ASYNC_BLIT_STREAM_H__
#define __DP_ASYNC_BLIT_STREAM_H__

#include "DpDataType.h"
#include <pthread.h>
#include <vector>

using namespace std;

class DpMutex;
class DpCondition;
class DpSync;

#define ISP_MAX_OUTPUT_PORT_NUM (4)

class DpAsyncBlitStream
{
public:
    static bool queryHWSupport(uint32_t         srcWidth,
                               uint32_t         srcHeight,
                               uint32_t         dstWidth,
                               uint32_t         dstHeight,
                               int32_t          Orientation = 0,
                               DpColorFormat    srcFormat = DP_COLOR_UNKNOWN,
                               DpColorFormat    dstFormat = DP_COLOR_UNKNOWN);

    DpAsyncBlitStream();

    ~DpAsyncBlitStream();

    enum DpOrientation
    {
        ROT_0   = 0x00000000,
        FLIP_H  = 0x00000001,
        FLIP_V  = 0x00000002,
        ROT_90  = 0x00000004,
        ROT_180 = FLIP_H|FLIP_V,
        ROT_270 = ROT_180|ROT_90,
        ROT_INVALID = 0x80
    };

    DP_STATUS_ENUM createJob(uint32_t &jobID, int32_t &fence);

    DP_STATUS_ENUM cancelJob(uint32_t jobID = 0);

    DP_STATUS_ENUM setConfigBegin(uint32_t jobID);

    DP_STATUS_ENUM setSrcBuffer(void     *pVABase,
                                uint32_t size,
                                int32_t  fenceFd = -1);

    DP_STATUS_ENUM setSrcBuffer(void     **pVAList,
                                uint32_t *pSizeList,
                                uint32_t planeNumber,
                                int32_t  fenceFd = -1);

    // VA + MVA address interface
    DP_STATUS_ENUM setSrcBuffer(void**   pVAddrList,
                                void**   pMVAddrList,
                                uint32_t *pSizeList,
                                uint32_t planeNumber,
                                int32_t  fenceFd = -1);

    // for ION file descriptor
    DP_STATUS_ENUM setSrcBuffer(int32_t  fileDesc,
                                uint32_t *sizeList,
                                uint32_t planeNumber,
                                int32_t  fenceFd = -1);

    DP_STATUS_ENUM setSrcConfig(int32_t           width,
                                int32_t           height,
                                DpColorFormat     format,
                                DpInterlaceFormat field = eInterlace_None);

    DP_STATUS_ENUM setSrcConfig(int32_t           width,
                                int32_t           height,
                                int32_t           yPitch,
                                int32_t           uvPitch,
                                DpColorFormat     format,
                                DP_PROFILE_ENUM   profile = DP_PROFILE_BT601,
                                DpInterlaceFormat field   = eInterlace_None,
                                DpSecure          secure  = DP_SECURE_NONE,
                                bool              doFlush = true);

    DP_STATUS_ENUM setSrcCrop(int32_t portIndex,
                              DpRect  roi);

    DP_STATUS_ENUM setDstBuffer(int32_t  portIndex,
                                void     *pVABase,
                                uint32_t size,
                                int32_t  fenceFd = -1);

    DP_STATUS_ENUM setDstBuffer(int32_t  portIndex,
                                void     **pVABaseList,
                                uint32_t *pSizeList,
                                uint32_t planeNumber,
                                int32_t  fenceFd = -1);

    // VA + MVA address interface
    DP_STATUS_ENUM setDstBuffer(int32_t  portIndex,
                                void**   pVABaseList,
                                void**   pMVABaseList,
                                uint32_t *pSizeList,
                                uint32_t planeNumber,
                                int32_t  fenceFd = -1);

    // for ION file descriptor
    DP_STATUS_ENUM setDstBuffer(int32_t  portIndex,
                                int32_t  fileDesc,
                                uint32_t *pSizeList,
                                uint32_t planeNumber,
                                int32_t  fenceFd = -1);

    DP_STATUS_ENUM setDstConfig(int32_t           portIndex,
                                int32_t           width,
                                int32_t           height,
                                DpColorFormat     format,
                                DpInterlaceFormat field = eInterlace_None,
                                DpRect            *pROI = 0);

    DP_STATUS_ENUM setDstConfig(int32_t           portIndex,
                                int32_t           width,
                                int32_t           height,
                                int32_t           yPitch,
                                int32_t           uvPitch,
                                DpColorFormat     format,
                                DP_PROFILE_ENUM   profile = DP_PROFILE_BT601,
                                DpInterlaceFormat field   = eInterlace_None,
                                DpRect            *pROI   = 0,
                                DpSecure          secure  = DP_SECURE_NONE,
                                bool              doFlush = true);

    DP_STATUS_ENUM setConfigEnd();

    DP_STATUS_ENUM setRotate(int32_t portIndex,
                             int32_t rotation);

    //Compatible to 89
    DP_STATUS_ENUM setFlip(int32_t portIndex,
                           int flip);

    DP_STATUS_ENUM setOrientation(int32_t  portIndex,
                                  uint32_t transform);

    uint32_t getPqID();

    DP_STATUS_ENUM setPQParameter(int32_t portIndex,
                                  const DpPqParam &pqParam);

    DP_STATUS_ENUM setDither(int32_t portIndex,
                             bool    enDither);

    DP_STATUS_ENUM setUser(uint32_t eID = 0);

    DP_STATUS_ENUM invalidate();

#if 0
    DP_STATUS_ENUM pq_process();
#endif

private:
    typedef struct BlitConfig
    {
        // src
        int32_t         srcBuffer;
        int32_t         srcWidth;
        int32_t         srcHeight;
        int32_t         srcYPitch;
        int32_t         srcUVPitch;
        DpColorFormat   srcFormat;
        DP_PROFILE_ENUM srcProfile;
        DpSecure        srcSecure;
        bool            srcFlush;

        // dst
        int32_t         dstBuffer[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         dstWidth[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         dstHeight[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         dstYPitch[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         dstUVPitch[ISP_MAX_OUTPUT_PORT_NUM];
        DpColorFormat   dstFormat[ISP_MAX_OUTPUT_PORT_NUM];
        DP_PROFILE_ENUM dstProfile[ISP_MAX_OUTPUT_PORT_NUM];
        DpSecure        dstSecure[ISP_MAX_OUTPUT_PORT_NUM];
        bool            dstFlush[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         dstEnabled[ISP_MAX_OUTPUT_PORT_NUM];

        // crop
        int32_t         cropXStart[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         cropYStart[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         cropWidth[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         cropHeight[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         cropSubPixelX[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         cropSubPixelY[ISP_MAX_OUTPUT_PORT_NUM];
        bool            cropEnabled[ISP_MAX_OUTPUT_PORT_NUM];

        // target offset
        int32_t         targetXStart[ISP_MAX_OUTPUT_PORT_NUM];
        int32_t         targetYStart[ISP_MAX_OUTPUT_PORT_NUM];

        int32_t         rotation[ISP_MAX_OUTPUT_PORT_NUM];
        bool            flipStatus[ISP_MAX_OUTPUT_PORT_NUM];
        bool            ditherStatus[ISP_MAX_OUTPUT_PORT_NUM];
        DpPqConfig      PqConfig[ISP_MAX_OUTPUT_PORT_NUM];

        bool            frameChange;

        BlitConfig();
    } BlitConfig;

    typedef enum JOB_CONFIG_STATE_ENUM
    {
        JOB_CREATE,
        JOB_CONFIGING,
        JOB_CONFIG_DONE,
        JOB_INVALIDATE
    } JOB_CONFIG_STATE_ENUM;

    typedef struct AsyncBlitJob
    {
        uint32_t              jobID;        // job id and initial timeline value
        uint32_t              timeValue;    // timeline value; thread safe
        int32_t               fenceFD;      // fence fd for signal user
        DpJobID               cmdTaskID;    // cmdq task id for wait complete
        BlitConfig            config;       // job config
        JOB_CONFIG_STATE_ENUM state;        // job state; thread safe
    } AsyncBlitJob;

    typedef std::vector<AsyncBlitJob *> JobList;

    // called by constructor
    void createThread();

    uint32_t getJobID();

    // called by invalidate
    DP_STATUS_ENUM waitSubmit();

    static void* waitComplete(void* para);

    DpStream          *m_pStream;
    DpChannel         *m_pChannel;
    int32_t           m_channelID;
    DpBasicBufferPool *m_pSrcPool;
    DpBasicBufferPool *m_pDstPool[ISP_MAX_OUTPUT_PORT_NUM];
#if 0
    int32_t           m_srcBuffer;
    int32_t           m_srcWidth;
    int32_t           m_srcHeight;
    int32_t           m_srcYPitch;
    int32_t           m_srcUVPitch;
    DpColorFormat     m_srcFormat;
    DP_PROFILE_ENUM   m_srcProfile;
    DpSecure          m_srcSecure;
    bool              m_srcFlush;
    int32_t           m_dstBuffer;
    int32_t           m_dstWidth;
    int32_t           m_dstHeight;
    int32_t           m_dstYPitch;
    int32_t           m_dstUVPitch;
    DpColorFormat     m_dstFormat;
    DP_PROFILE_ENUM   m_dstProfile;
    DpSecure          m_dstSecure;
    bool              m_dstFlush;
    DpStream          *m_pPqStream;
    DpChannel         *m_pPqChannel;
    DpAutoBufferPool  *m_pPqPool;
    int32_t           m_pqBuffer;
    int32_t           m_cropXStart;
    int32_t           m_cropYStart;
    int32_t           m_cropWidth;
    int32_t           m_cropHeight;
    int32_t           m_cropSubPixelX;
    int32_t           m_cropSubPixelY;
    int32_t           m_targetXStart;
    int32_t           m_targetYStart;
    int32_t           m_rotation;
    bool              m_frameChange;
    bool              m_flipStatus;
    bool              m_ditherStatus;
    DpPqConfig        m_PqConfig;
#endif

    DpBlitUser        m_userID;
    uint32_t          m_PqID[ISP_MAX_OUTPUT_PORT_NUM];
    int32_t           m_pqSupport;

    // thread safe
    DpMutex           *m_pJobMutex;
    DpCondition       *m_pJobCond;
    JobList           m_newJobList;
    JobList           m_jobList;
    JobList           m_waitJobList;
    uint32_t          m_timeValue;
    bool              m_abortJobs;

    // *NOT* thread safe
    AsyncBlitJob      *m_pCurJob;
    BlitConfig        m_prevConfig;

    pthread_t         m_thread;

    DpSync*           m_pSync;
};

#endif  // __DP_ASYNC_BLIT_STREAM_H__
