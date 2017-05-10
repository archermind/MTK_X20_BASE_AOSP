#ifndef	_INTERFACE_TRANSFER_H_
#define	_INTERFACE_TRANSFER_H_
#include "common/types.h"
#include <string>

using std::size_t;

DECLEAR_INTERFACE class ITransmission
{
public:
   //return actual read bytes. timeout throw exception.
   virtual size_t receive(uint8* data, size_t length, uint32 timeout) = 0;
   //return actual send bytes. timeout throw exception.
   virtual size_t send(uint8* data, size_t length, uint32 timeout) = 0;
protected:
   virtual ~ITransmission(){}
};

DECLEAR_INTERFACE class ITransmissionCtrl
{
public:
   virtual void open(const std::string& name) = 0;
   virtual void cancel() = 0;
   virtual void close() = 0;
protected:
   virtual ~ITransmissionCtrl(){}
};

DECLEAR_INTERFACE class ITransmission_engine : public IMPLEMENTS ITransmission, public IMPLEMENTS ITransmissionCtrl
{
protected:
   virtual ~ITransmission_engine(){}
};


#endif