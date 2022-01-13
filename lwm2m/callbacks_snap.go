// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package lwm2m

import (
	"fmt"
	"log"
	"strconv"
	"time"
	"unsafe"

	"github.com/snapcore/snapd/client"

	"launchpad.net/ce-web/alpaca/objects"
)

/*
#cgo LDFLAGS: -L${SRCDIR} -llwm2mclient
#cgo CFLAGS: -I${SRCDIR}/wakaama/core
#define _GNU_SOURCE
#include <stdlib.h>
*/
import "C"

//export GetSnapCount
func GetSnapCount() C.int {
	l := objects.GetSnapsInstance()
	number := len(l.Snaps)

	return C.int(number)
}

//export SnapInstanceRead
func SnapInstanceRead(instanceID int, rid int) *C.char {

	var resp *C.char
	defer C.free(unsafe.Pointer(resp))

	o := objects.GetSnapsInstance()

	if instanceID > len(o.Snaps) {
		log.Println("Attempt to retrieve an unlisted snap")
		return resp
	}

	switch rid {
	case 0:
		resp = C.CString(o.Snaps[instanceID].Name)
	case 1:
		resp = C.CString(o.Snaps[instanceID].Summary)
	case 2:
		resp = C.CString(o.Snaps[instanceID].Confinement)
	case 3:
		resp = C.CString(o.Snaps[instanceID].Developer)
	case 4:
		resp = C.CString(o.Snaps[instanceID].InstallDate.UTC().Format(time.RFC3339))
	case 5:
		value := strconv.FormatInt(o.Snaps[instanceID].InstalledSize, 10)
		resp = C.CString(value)
	case 6:
		resp = C.CString(o.Snaps[instanceID].Status)
	case 7:
		resp = C.CString(o.Snaps[instanceID].Version)
	case 8:
		resp = C.CString(o.Snaps[instanceID].Revision.String())
	case 9:
		if o.Snaps[instanceID].DevMode {
			resp = C.CString("true")
		}
		resp = C.CString("false")
	default:
		resp = C.CString("")
	}

	return resp
}

//export SnapInstanceExecute
func SnapInstanceExecute(instanceID int, rid int) *C.char {

	var resp *C.char
	defer C.free(unsafe.Pointer(resp))

	o := objects.GetSnapsInstance()

	if instanceID > len(o.Snaps) {
		log.Println("Attempt to retrieve an unlisted snap")
		resp = C.CString("")
		return resp
	}

	snapName := o.Snaps[instanceID].Name

	switch rid {
	case 11:
		resp = C.CString(o.Refresh(snapName))
	case 12:
		resp = C.CString(o.Remove(snapName))
	case 13:
		resp = C.CString(o.Revert(snapName))
	case 14:
		resp = C.CString(o.Enable(snapName))
	case 15:
		resp = C.CString(o.Disable(snapName))
	default:
		resp = C.CString("Unknown action requested")
	}

	return resp
}

//export SnapDelimited
func SnapDelimited(instanceID int) *C.char {

	var resp *C.char
	defer C.free(unsafe.Pointer(resp))

	o := objects.GetSnapsInstance()

	if instanceID > len(o.Snaps) {
		log.Println("Attempt to retrieve an unlisted snap")
		return resp
	}

	// Return a pipe-delimited representation of the snap
	s := encodeSnap(o.Snaps[instanceID])
	resp = C.CString(s)

	return resp
}

//export SnapConfig
func SnapConfig(instanceID int) {
	o := objects.GetSnapsInstance()
	if instanceID > len(o.Snaps) {
		log.Println("Attempt to retrieve an unlisted snap")
		return
	}

	snap := o.Snaps[instanceID]
	c, err := o.Conf(snap.Name)
	if err != nil {
		log.Println("Error retrieving the snap config:", err)
	}

	log.Println("---", c)
}

// encodeSnap creates a pipe-delimited description of the snap. This is a hacked solution
// as LwM2M support for updating multi-instance objects is poor
func encodeSnap(s client.Snap) string {
	// Encode the list of details as a delimited string
	return s.Name + "|" + s.Description + "|" + s.Summary + "|" + s.Version + "|" + s.Status
}

//export SnapExecute
func SnapExecute(action int, name *C.char) *C.char {

	var resp *C.char
	defer C.free(unsafe.Pointer(resp))

	o := objects.GetSnapsInstance()
	snapName := C.GoString(name)

	switch action {
	case 10:
		resp = C.CString(o.Install(snapName))
	case 11:
		resp = C.CString(o.Uninstall(snapName))
	case 12:
		resp = C.CString(o.Refresh(snapName))
	case 13:
		resp = C.CString(o.Revert(snapName))
	case 14:
		resp = C.CString(o.Enable(snapName))
	case 15:
		resp = C.CString(o.Disable(snapName))
	}

	return resp
}

// SnapRefreshData refreshes the data for resources whose values change often
func SnapRefreshData() map[string]string {
	o := objects.GetSnapsInstance()

	data := map[string]string{
		"/30000/0/0": string(len(o.Snaps)),
		"/30000/0/1": "",
		"/30000/0/2": "",
	}

	for i := range o.Snaps {
		data[fmt.Sprintf("/30001/%d/0", i)] = o.Snaps[i].Name
		data[fmt.Sprintf("/30001/%d/1", i)] = o.Snaps[i].Summary
		data[fmt.Sprintf("/30001/%d/2", i)] = o.Snaps[i].Confinement
		data[fmt.Sprintf("/30001/%d/3", i)] = o.Snaps[i].Developer
		data[fmt.Sprintf("/30001/%d/4", i)] = o.Snaps[i].InstallDate.UTC().Format(time.RFC3339)
		data[fmt.Sprintf("/30001/%d/5", i)] = string(o.Snaps[i].InstalledSize)
		data[fmt.Sprintf("/30001/%d/6", i)] = o.Snaps[i].Status
		data[fmt.Sprintf("/30001/%d/7", i)] = o.Snaps[i].Version
		data[fmt.Sprintf("/30001/%d/8", i)] = o.Snaps[i].Revision.String()
		if o.Snaps[i].DevMode {
			data[fmt.Sprintf("/30001/%d/9", i)] = "true"
		} else {
			data[fmt.Sprintf("/30001/%d/9", i)] = "false"
		}
	}

	return data
}
