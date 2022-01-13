// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package main

import (
	"log"
	"time"

	"launchpad.net/ce-web/alpaca/mqtt"
)

const tickInterval = 60

func main() {
	conf := mqtt.ReadParameters()

	log.Println("Starting MQTT client")

	c, err := mqtt.NewConnection(conf.ServerHost, conf.ServerPort, conf.Name, conf.Username, conf.Password)
	if err != nil {
		log.Printf("Error connecting to the MQTT broker: %v", err)
		return
	}

	ticker := time.NewTicker(time.Second * tickInterval)

	for range ticker.C {
		// Publish the list of installed snaps to the MQTT broker
		err = mqtt.SnapList(c)
		if err != nil {
			log.Printf("Error sending the list of snaps: %v", err)
			return
		}
	}

	ticker.Stop()
	c.Close()
}
