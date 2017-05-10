/* vim: set et sw=4 ts=4: */

#ifndef __MTK_FPSCONTROL_H_
#define __MTK_FPSCONTROL_H_

#include <binder/BinderService.h>
#include <utils/AndroidThreads.h>
#include <utils/Looper.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

#include "ged/ged_log.h"
#include "ged/ged_notifySF.h"

namespace android {
//------------------------------------------------------------------------------

/* TODO:
    1. Singleton
*/
class FrameRateRegulator {
public:
    FrameRateRegulator() {}

    status_t initialize(NotifySF *nsf, GED_LOG_HANDLE handle);

    void fpsProbe();

    void updateFpsUserSetting(int fps);

private:
    /*------------------------------------------------------------------------*/
    /* Internal data structure                                                */
    /*------------------------------------------------------------------------*/
    class Arbitrator : public Thread {
    /* TODO:
            1. Mediator pattern
    */
    public:
        Arbitrator() : Thread(false), mNotifySF(NULL), mGedLogHandle(NULL)
        {
            mFps[0] = mFps[1] = mLastFps = 60;
        }

        status_t initialize(NotifySF *nsf, GED_LOG_HANDLE handle);

        void voteFps(int fps, int voter);

        /* Sent fps changing request to SurfaceFlinger */
        void changeFps(int fps);

    private:
        int mFps[2];

        int mLastFps;

        NotifySF *mNotifySF;

        GED_LOG_HANDLE mGedLogHandle;

        Mutex mFpsLock;

        /* Evaluates every ballot, and make decision of final FPS. */
        bool threadLoop();

        bool isFpsQualified(int fps);
    };

    class ThermalListener : public Thread {
    public:
        ThermalListener() : Thread(false), mSocket(-1), mFps(60),
                mArbitrator(NULL) {}

        status_t initialize(Arbitrator *arb);

    private:
        int     mSocket;

        Mutex   mFpsLock;

        int     mFps;

        Arbitrator      *mArbitrator;

        /* Listen to thermal uevent */
        void handleUevents(const char*, int);

        bool threadLoop();

        int getFps() {
            Mutex::Autolock _l(mFpsLock);
            return mFps;
        }

    };

    enum {
        VOTER_THERMAL,
        VOTER_USER_SETTING,
        NUM_OF_VOTER,
    };

    Arbitrator      mArbitrator;

    ThermalListener mThermalListener;
};

//------------------------------------------------------------------------------
}
#endif
