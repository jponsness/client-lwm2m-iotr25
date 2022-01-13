// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package objects

import "C"
import (
	"strconv"
	"sync"
	"time"

	"launchpad.net/ce-web/alpaca/snapdapi"
)

// Device defines a device object
type Device struct {
	client      *snapdapi.ClientAdapter
	Info        snapdapi.DeviceInfo
	lastRefresh int64
}

// Using a singleton to define the device
var deviceInstance *Device
var deviceOnce sync.Once

// GetDeviceInstance returns an instance of a device object
func GetDeviceInstance() *Device {
	deviceOnce.Do(func() {
		deviceInstance = &Device{client: snapdapi.NewClientAdapter()}
	})
	if dataIsStale(deviceInstance.lastRefresh) {
		deviceInstance.refresh()
		deviceInstance.lastRefresh = time.Now().Unix()
	} else {
		// Update the time field only
		deviceInstance.Info.CurrentTime = strconv.FormatInt(time.Now().Unix(), 10)
	}
	return deviceInstance
}

// refresh the device information
func (d *Device) refresh() {
	details, err := snapdapi.GetModelInfo(d.client)
	if err != nil {
		return
	}

	d.Info = details
}
