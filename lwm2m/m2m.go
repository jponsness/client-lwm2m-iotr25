// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package lwm2m

// #cgo LDFLAGS: -L${SRCDIR} -llwm2mclient
// #define _GNU_SOURCE
// #include "src/lwm2mclient.h"
// #include <stdlib.h>
import "C"
import "unsafe"

// CreateServer wraps C library createServer
func CreateServer(serverHost, serverPort, localPort, name string, bootstrapRequired bool) int {

	chost := C.CString(serverHost)
	cport := C.CString(serverPort)
	clocal := C.CString(localPort)
	cname := C.CString(name)
	cbootstrap := C.int(0)
	if bootstrapRequired {
		cbootstrap = C.int(1)
	}

	out := C.createServer(chost, cport, clocal, cname, cbootstrap)
	C.free(unsafe.Pointer(chost))
	C.free(unsafe.Pointer(cport))
	C.free(unsafe.Pointer(clocal))
	C.free(unsafe.Pointer(cname))
	return int(out)
}

// CloseServer closes the connection to the server
func CloseServer() int {
	out := C.closeServer()
	return int(out)
}

// WaitForTimeout waits for the timeout set by the previous function's call.
func WaitForTimeout() int {
	out := C.waitForTimeout()
	return int(out)
}

// SendData sends data to the server from the object registry
func SendData() int {
	out := C.sendData()
	return int(out)
}

// ReadData reads data sent from the server and updates the object registry
func ReadData() int {
	out := C.readData()
	return int(out)
}

// RefreshData refreshes data for resources that change frequently e.g. time, battery levels
func RefreshData() {
	changedItems := DeviceRefreshData()
	for k, v := range changedItems {
		handleValueChanged(k, v)
	}

	changedSnap := SnapRefreshData()
	for k, v := range changedSnap {
		handleValueChanged(k, v)
	}

}

// RefreshObjects refreshes the full object list. Used after snap install/uninstall
func RefreshObjects() {
	C.refreshObjects()
}

// handleValueChanged calls the lwm2m function to update the object registry
func handleValueChanged(uri, value string) {
	curi := C.CString(uri)
	cvalue := C.CString(value)

	C.handleValueRefresh(curi, C.int(len(uri)), cvalue, C.int(len(value)))

	C.free(unsafe.Pointer(curi))
	C.free(unsafe.Pointer(cvalue))
}
