/* Created by "go tool cgo" - DO NOT EDIT. */

/* package lwm2m */

/* Start of preamble from import "C" comments.  */

// JBP removed
//#line 22 "/home/jjesudason/Documents/PES/go/src/launchpad.net/ce-web/alpaca/lwm2m/callbacks_device.go"



#define _GNU_SOURCE
#include <stdlib.h>

//JBP Removed
//#line 34 "/home/jjesudason/Documents/PES/go/src/launchpad.net/ce-web/alpaca/lwm2m/callbacks_snap.go"

#define _GNU_SOURCE
#include <stdlib.h>



/* End of preamble from import "C" comments.  */


/* Start of boilerplate cgo prologue.  */

#ifndef GO_CGO_PROLOGUE_H
#define GO_CGO_PROLOGUE_H

typedef signed char GoInt8;
typedef unsigned char GoUint8;
typedef short GoInt16;
typedef unsigned short GoUint16;
typedef int GoInt32;
typedef unsigned int GoUint32;
typedef long long GoInt64;
typedef unsigned long long GoUint64;
typedef GoInt64 GoInt;
typedef GoUint64 GoUint;
typedef __SIZE_TYPE__ GoUintptr;
typedef float GoFloat32;
typedef double GoFloat64;
typedef float _Complex GoComplex64;
typedef double _Complex GoComplex128;

/*
  static assertion to make sure the file is being used on architecture
  at least with matching size of GoInt.
*/
//JBP removed (need to validate if this is OK; this was needed to )
//typedef char _check_for_64_bit_pointer_matching_GoInt[sizeof(void*)==64/8 ? 1:-1];

typedef struct { const char *p; GoInt n; } GoString;
typedef void *GoMap;
typedef void *GoChan;
typedef struct { void *t; void *v; } GoInterface;
typedef struct { void *data; GoInt len; GoInt cap; } GoSlice;

#endif

/* End of boilerplate cgo prologue.  */

#ifdef __cplusplus
extern "C" {
#endif


extern void GetDeviceInformation();

extern char* DeviceObjectRead(GoInt p0);

extern int GetSnapCount();

extern char* SnapInstanceRead(GoInt p0, GoInt p1);

extern char* SnapInstanceExecute(GoInt p0, GoInt p1);

extern char* SnapDelimited(GoInt p0);

extern void SnapConfig(GoInt p0);

extern char* SnapExecute(GoInt p0, char* p1);

#ifdef __cplusplus
}
#endif
