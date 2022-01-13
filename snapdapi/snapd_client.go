// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package snapdapi

import (
	"fmt"
	"log"
	"net/url"
	"strconv"
	"syscall"
	"time"

	"github.com/snapcore/snapd/asserts"
	"github.com/snapcore/snapd/client"
)

// SnapdClient is a client of the snapd REST API
type SnapdClient interface {
	Snap(name string) (*client.Snap, *client.ResultInfo, error)
	List(names []string, opts *client.ListOptions) ([]*client.Snap, error)
	Install(name string, options *client.SnapOptions) (string, error)
	Refresh(name string, options *client.SnapOptions) (string, error)
	Revert(name string, options *client.SnapOptions) (string, error)
	Remove(name string, options *client.SnapOptions) (string, error)
	Enable(name string, options *client.SnapOptions) (string, error)
	Disable(name string, options *client.SnapOptions) (string, error)
	ServerVersion() (*client.ServerVersion, error)
	Ack(b []byte) error
	Known(assertTypeName string, headers map[string]string) ([]asserts.Assertion, error)
	Conf(name string) (map[string]interface{}, error)
	SetConf(name string, patch map[string]interface{}) (string, error)
	Find(opts *client.FindOptions) ([]*client.Snap, *client.ResultInfo, error)
	FindOne(name string) (*client.Snap, *client.ResultInfo, error)
	FindSnaps(query, section string, private bool) ([]*client.Snap, *client.ResultInfo, error)
}

// ClientAdapter adapts our expectations to the snapd client API.
type ClientAdapter struct {
	snapdClient *client.Client
}

// NewClientAdapter creates a new ClientAdapter for use in snapweb.
func NewClientAdapter() *ClientAdapter {
	return &ClientAdapter{
		snapdClient: client.New(nil),
	}
}

// Snap returns the most recently published revision of the snap with the
// provided name.
func (a *ClientAdapter) Snap(name string) (*client.Snap, *client.ResultInfo, error) {
	return a.snapdClient.Snap(name)
}

// List returns the list of all snaps installed on the system
// with names in the given list; if the list is empty, all snaps.
func (a *ClientAdapter) List(names []string, opts *client.ListOptions) ([]*client.Snap, error) {
	return a.snapdClient.List(names, opts)
}

// Install adds the snap with the given name from the given channel (or
// the system default channel if not).
func (a *ClientAdapter) Install(name string, options *client.SnapOptions) (string, error) {
	return a.snapdClient.Install(name, options)
}

// Refresh updates the snap with the given name from the given channel (or
// the system default channel if not).
func (a *ClientAdapter) Refresh(name string, options *client.SnapOptions) (string, error) {
	return a.snapdClient.Refresh(name, options)
}

// Revert rolls the snap back to the previous on-disk state
func (a *ClientAdapter) Revert(name string, options *client.SnapOptions) (string, error) {
	return a.snapdClient.Revert(name, options)
}

// Remove removes the snap with the given name.
func (a *ClientAdapter) Remove(name string, options *client.SnapOptions) (string, error) {
	return a.snapdClient.Remove(name, options)
}

// Enable activates the snap with the given name.
func (a *ClientAdapter) Enable(name string, options *client.SnapOptions) (string, error) {
	return a.snapdClient.Enable(name, options)
}

// Disable deactivates the snap with the given name.
func (a *ClientAdapter) Disable(name string, options *client.SnapOptions) (string, error) {
	return a.snapdClient.Disable(name, options)
}

// ServerVersion returns information about the snapd server.
func (a *ClientAdapter) ServerVersion() (*client.ServerVersion, error) {
	return a.snapdClient.ServerVersion()
}

// Ack adds a new assertion to the system
func (a *ClientAdapter) Ack(b []byte) error {
	return a.snapdClient.Ack(b)
}

// Known queries assertions with type assertTypeName and matching assertion headers.
func (a *ClientAdapter) Known(assertTypeName string, headers map[string]string) ([]asserts.Assertion, error) {
	return a.snapdClient.Known(assertTypeName, headers)
}

// Conf gets the snap's current configuration
func (a *ClientAdapter) Conf(name string) (map[string]interface{}, error) {
	return a.snapdClient.Conf(name, []string{})
}

// SetConf requests a snap to apply the provided patch to the configuration
func (a *ClientAdapter) SetConf(name string, patch map[string]interface{}) (string, error) {
	return a.snapdClient.SetConf(name, patch)
}

// GetModelInfo returns information about the device.
func GetModelInfo(c SnapdClient) (DeviceInfo, error) {

	device := DeviceInfo{
		DeviceName: "Device Name",
		Brand:      "Unknown",
		Model:      "Unknown",
		Serial:     "Unknown",
	}

	// Server version
	sysInfo, err := c.ServerVersion()
	if err != nil {
		return device, err
	}
	device.SoftwareVersion = sysInfo.OSID + " " + sysInfo.Series
	device.FirmwareVersion = sysInfo.KernelVersion

	// Get Model Info from the serial assertion
	serialInfo, err := c.Known("serial", map[string]string{})
	if err == nil {
		if len(serialInfo) == 0 {
			log.Println("GetModelInfo: No assertions returned for serial type")
		} else {
			device.Brand = serialInfo[0].Header("brand-id").(string)
			device.Model = serialInfo[0].Header("model").(string)
			device.Serial = serialInfo[0].Header("serial").(string)
		}
	} else {
		log.Println(fmt.Sprintf("GetModelInfo: No serial type info found: %s", err))
	}

	// Time fields
	device.CurrentTime = strconv.FormatInt(time.Now().Unix(), 10)
	zone, _ := time.Now().Zone()
	device.Timezone = zone
	device.UTCOffset = time.Now().Format(time.RFC1123Z)
	device.UTCOffset = device.UTCOffset[len(device.UTCOffset)-5:]

	// Uptime
	var msi syscall.Sysinfo_t
	err = syscall.Sysinfo(&msi)
	if err != nil {
		return device, err
	}
	device.Uptime = (time.Duration(msi.Uptime) * time.Second).String()

	return device, nil
}

// FindOne returns a list of snaps available for install from the
// store for this system and that match the query
func (a *ClientAdapter) FindOne(name string) (*client.Snap, *client.ResultInfo, error) {
	return a.snapdClient.FindOne(name)
}

// Find returns a list of snaps available for install from the
// store for this system and that match the query
func (a *ClientAdapter) Find(opts *client.FindOptions) ([]*client.Snap, *client.ResultInfo, error) {
	return a.snapdClient.Find(opts)
}

// FindSnaps is a wrapper around the Find function
func (a *ClientAdapter) FindSnaps(query, section string, private bool) ([]*client.Snap, *client.ResultInfo, error) {
	opts := &client.FindOptions{
		Query:   url.QueryEscape(query),
		Private: private,
		Section: section,
	}
	return a.snapdClient.Find(opts)
}
