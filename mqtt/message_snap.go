// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package mqtt

import (
	"encoding/json"
	"log"

	"launchpad.net/ce-web/alpaca/objects"
)

// SnapList publishes the list of snaps to the broker
func SnapList(connection *Connection) error {
	o := objects.GetSnapsInstance()

	b, err := json.Marshal(o.Snaps)
	if err != nil {
		log.Printf("Error marshalling the snap list: %v\n", err)
		return err
	}

	log.Println("Publish to", Config.Name+"/snaps")
	err = connection.Publish(Config.Name+"/snaps", string(b))
	return err
}
