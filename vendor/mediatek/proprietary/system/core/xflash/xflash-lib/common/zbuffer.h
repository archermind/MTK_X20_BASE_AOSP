#pragma once
#include "common/types.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

/*
* Add this class because of:
* boost 1.52 do not support share_ptr<uint8[]>, only have shared_array<uint8>
* but 1.53 and later have no shared_array<uint8> any more.
* gcc4.4.3 only support boost1.52
*/
class zbuffer
{
public:
   zbuffer(uint32 len)
   :length(len)
   {
      data = new uint8[length];
      memset(data, 0, length);
   }
   ~zbuffer()
   {
      reset();
   }
   void reset()
   {
      if(data != 0)delete[] data;
      data = 0;
      length = 0;
   }
   uint8* get(){return data;}
   uint32 size(){return length;}
private:
   zbuffer();
   zbuffer(zbuffer&);
   uint8* data;
   uint32 length;
};
