#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

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
