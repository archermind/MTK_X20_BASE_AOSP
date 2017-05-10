#ifndef COMM_ENGINE_H
#define COMM_ENGINE_H

#include "transfer/ITransmission.h"
#include "common/types.h"
#include <string>
#include "common/common_include.h"
using std::string;

class comm_engine : IMPLEMENTS public ITransmission_engine
{
public:
   comm_engine();
   comm_engine(STD_::string str_baud_rate);
   ~comm_engine(void);
   void open(const STD_::string& name);
   //return actual read bytes. timeout or cancel throw exception.
   size_t receive(uint8* data, size_t length, uint32 timeout);
   //return actual send bytes. timeout or cancel throw exception.
   size_t send(uint8* data, size_t length, uint32 timeout);
   void close();
   void cancel();
   void set_baudrate(uint32 baud);
private:
   COM_HANDLE hcom;
   void* ext; //to put some data special with OS.
   uint32 baud_rate;
public:
   static void test();
};


#endif