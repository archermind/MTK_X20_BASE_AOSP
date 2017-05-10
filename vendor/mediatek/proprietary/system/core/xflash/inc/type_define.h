#ifndef	_TYPE_DEFINE_H_
#define	_TYPE_DEFINE_H_


typedef unsigned char         uint8;
typedef signed char           int8;
typedef unsigned short        uint16;
typedef signed short          int16;
typedef unsigned int          uint32;
typedef signed int            int32;


typedef char                  char8;
typedef short                 char16;
typedef int                   char32;

typedef unsigned long         ulong32;
typedef long                  long32;
typedef unsigned long long    uint64;
typedef signed long long      int64;
typedef void*                 pvoid;

typedef int                   BOOL;
typedef int                   status_t;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef long HSESSION;
#define INVALID_HSESSION_VALUE 0

#endif
