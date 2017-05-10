#ifndef __MTK_GED_NOTIFYSF_H_
#define __MTK_GED_NOTIFYSF_H_

#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/TextOutput.h>

#include <pthread.h>

using namespace android;


#define SF_DEAD 0xdeaddad


static String16 get_interface_name(sp<IBinder> service)
{
    if (service != NULL) {
        Parcel data, reply;
        status_t err = service->transact(IBinder::INTERFACE_TRANSACTION, data, &reply);
        if (err == NO_ERROR) {
            return reply.readString16();
        }
    }
    return String16();
}

class NotifySF
{
    sp<IServiceManager> sm;
    sp<IBinder> service;
    String16 ifName;
    pthread_mutex_t mMutex;
    
    public:
    status_t obj_state;
    NotifySF()
    {
        mMutex = PTHREAD_MUTEX_INITIALIZER;
        sm = defaultServiceManager();
        service =NULL;
        service = sm->checkService(String16("SurfaceFlinger"));
        ifName = get_interface_name(service);
        obj_state = NO_ERROR;
        if(service==NULL)
            obj_state = SF_DEAD;
    }
   
   void reconnect()
   {
       pthread_mutex_lock(&mMutex);
        sm = defaultServiceManager();
        service =NULL;
        service = sm->checkService(String16("SurfaceFlinger"));
        ifName = get_interface_name(service);
        obj_state = NO_ERROR;
        if(service==NULL)
        {
            obj_state = SF_DEAD;
        }
        pthread_mutex_unlock(&mMutex);
    }
   
    status_t send(int protocol, int i32Data)
    {
        status_t ret;
        Parcel data, reply;
        data.writeInterfaceToken(ifName);
        data.writeInt32(i32Data);
        pthread_mutex_lock(&mMutex);
        if(service!=NULL)
            ret =  service->transact(protocol, data, &reply);
        else
        {
            obj_state = ret = SF_DEAD;
        }
        pthread_mutex_unlock(&mMutex);
        return ret;
    }    
};

#endif
