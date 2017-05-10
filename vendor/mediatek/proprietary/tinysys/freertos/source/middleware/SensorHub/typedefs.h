#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

// ---------------------------------------------------------------------------
//  Basic Type Definitions
// ---------------------------------------------------------------------------

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned long long  UINT64;

typedef signed char     INT8;
typedef signed short    INT16;
typedef signed int      INT32;
typedef signed long long     INT64;

typedef unsigned int bool;
// ---------------------------------------------------------------------------
//  Utilities
// ---------------------------------------------------------------------------
#define false (0)
#define true (1)

#define SM_ERROR (-1)
#define SM_SUCCESS (0)

#define MAXIMUM(A,B)       (((A)>(B))?(A):(B))
#define MINIMUM(A,B)       (((A)<(B))?(A):(B))

#define ARY_SIZE(x) (sizeof((x)) / sizeof((x[0])))

#endif  // _TYPEDEFS_H

