#ifndef	_TYPES_H_
#define	_TYPES_H_

#include "type_define.h"

//define some code identifier

#define DECLEAR_INTERFACE
#define IMPLEMENTS
#define EXTENDS
#define ABSTRACT




#ifdef _WIN32 /*windows -------------------------------------------------------*/
typedef void*        COM_HANDLE;

#else /*linux ----------------------------------------------------------------*/

typedef int        COM_HANDLE;



#endif


#endif
