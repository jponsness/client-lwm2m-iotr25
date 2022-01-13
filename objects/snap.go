// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package objects

import (
	"log"
	"sort"
	"sync"
	"time"

	"github.com/snapcore/snapd/client"
	"launchpad.net/ce-web/alpaca/snapdapi"
)

// SnapList defines a snap objects
type SnapList struct {
	Snaps       []client.Snap
	lastRefresh int64
	client      *snapdapi.ClientAdapter
}

// Using a singleton to define the snap objects
var snapInstance *SnapList
var snapOnce sync.Once

// ByName implements sort.Interface for the snap list
type ByName []client.Snap

func (a ByName) Len() int           { return len(a) }
func (a ByName) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a ByName) Less(i, j int) bool { return a[i].Name < a[j].Name }

// GetSnapsInstance returns an instance of a device object
func GetSnapsInstance() *SnapList {
	snapOnce.Do(func() {
		snapInstance = &SnapList{client: snapdapi.NewClientAdapter()}
	})
	if dataIsStale(snapInstance.lastRefresh) {
		snapInstance.refresh()
		snapInstance.lastRefresh = time.Now().Unix()
	}

	return snapInstance
}

// refresh the snap information from the snapd API
func (s *SnapList) refresh() {
	snaps, err := s.client.List([]string{}, nil)
	if err != nil {
		log.Printf("Error refreshing the list of installed snaps: %v", err)
		return
	}

	// Copy the items from the snap list to our slice
	s.Snaps = []client.Snap{}
	for _, p := range snaps {
		snap := *p
		s.Snaps = append(s.Snaps, snap)
	}
	sort.Sort(ByName(s.Snaps))
}

// Install installs a snap from the store
func (s *SnapList) Install(name string) string {
	log.Printf("---Install snap: %s", name)
	resp, err := s.client.Install(name, nil)
	if err != nil {
		log.Println(err)
		return err.Error()
	}
	log.Println("Response:", resp)
	return resp
}

// Uninstall removes a snap
func (s *SnapList) Uninstall(name string) string {
	log.Printf("---Uninstall snap: %s", name)
	resp, err := s.client.Remove(name, nil)
	if err != nil {
		log.Println(err)
		return err.Error()
	}
	log.Println("Response:", resp)
	return resp
}

// Refresh updates a snap from the store
func (s *SnapList) Refresh(name string) string {
	log.Println("---Refresh snap", name)
	resp, err := s.client.Refresh(name, nil)
	if err != nil {
		return err.Error()
	}
	return resp
}

// Remove updates a snap from the store
func (s *SnapList) Remove(name string) string {
	log.Println("---Remove snap", name)
	resp, err := s.client.Remove(name, nil)
	if err != nil {
		return err.Error()
	}
	return resp
}

// Revert updates a snap from the store
func (s *SnapList) Revert(name string) string {
	log.Println("---Revert snap", name)
	resp, err := s.client.Revert(name, nil)
	if err != nil {
		return err.Error()
	}
	return resp
}

// Enable updates a snap from the store
func (s *SnapList) Enable(name string) string {
	log.Println("---Enable snap", name)
	resp, err := s.client.Enable(name, nil)
	if err != nil {
		return err.Error()
	}
	return resp
}

// Disable updates a snap from the store
func (s *SnapList) Disable(name string) string {
	log.Println("---Disable snap", name)
	resp, err := s.client.Disable(name, nil)
	if err != nil {
		return err.Error()
	}
	return resp
}

// Conf gets the snaps config
func (s *SnapList) Conf(name string) (map[string]interface{}, error) {
	log.Println("---Snap config", name)
	return s.client.Conf(name)
}
