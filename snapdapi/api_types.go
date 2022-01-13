// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package snapdapi

// DeviceInfo holds the details of the device
type DeviceInfo struct {
	DeviceName      string
	Brand           string
	Model           string
	Serial          string
	FirmwareVersion string
	SoftwareVersion string
	Uptime          string
	CurrentTime     string
	UTCOffset       string
	Timezone        string
}
