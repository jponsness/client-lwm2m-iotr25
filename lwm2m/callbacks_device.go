// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package lwm2m

/*
#cgo LDFLAGS: -L${SRCDIR} -llwm2mclient
#cgo CFLAGS: -I${SRCDIR}/wakaama/core
#define _GNU_SOURCE
#include <stdlib.h>
*/
import "C"
import (
	"unsafe"

	"launchpad.net/ce-web/alpaca/objects"
)

//export GetDeviceInformation
func GetDeviceInformation() {
	objects.GetDeviceInstance()
}

//export DeviceObjectRead
func DeviceObjectRead(rid int) *C.char {

	var resp *C.char
	defer C.free(unsafe.Pointer(resp))

	o := objects.GetDeviceInstance()

	switch rid {
	case 0:
		resp = C.CString(o.Info.Brand)
	case 1:
		resp = C.CString(o.Info.Model)
	case 2:
		resp = C.CString(o.Info.Serial)
	case 3:
		resp = C.CString(o.Info.FirmwareVersion)
	case 13:
		resp = C.CString(o.Info.CurrentTime)
	case 14:
		resp = C.CString(o.Info.UTCOffset)
	case 15:
		resp = C.CString(o.Info.Timezone)
	case 19:
		resp = C.CString(o.Info.SoftwareVersion)
	default:
		resp = C.CString("")
	}

	return resp
}

// DeviceRefreshData refreshes the data for resources whose values change often
func DeviceRefreshData() map[string]string {
	o := objects.GetDeviceInstance()

	data := map[string]string{
		"/3/0/3":  o.Info.FirmwareVersion,
		"/3/0/13": o.Info.CurrentTime,
		"/3/0/14": o.Info.UTCOffset,
		"/3/0/19": o.Info.SoftwareVersion,
	}

	return data
}
