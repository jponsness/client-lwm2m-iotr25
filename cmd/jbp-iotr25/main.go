
// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package main

import (
	"log"

	"launchpad.net/ce-web/alpaca/lwm2m"
	"launchpad.net/ce-web/alpaca/pivot"
)

func main() {

	// Get the server details from the stored parameters
	c := lwm2m.ReadParameters()

	// TODO: make model pivoting configurable through the parameters
	const pivotModel = false

	if pivotModel {
		log.Println("Verify the device pivot")
		err := pivot.VerifyPivot(c.SerialVaultURL, c.SerialVaultAPI)
		if err != nil {
			log.Println(err)
		}
	}

	log.Printf("Starting LWM2M client '%s'\n", c.Name)
	log.Printf("Connect to the LwM2M server %s:%s\n", c.ServerHost, c.ServerPort)

	// Initialize the connection to the LWM2M server
	out := lwm2m.CreateServer(c.ServerHost, c.ServerPort, c.LocalPort, c.Name, c.BootstrapRequired)

	defer lwm2m.CloseServer()

	// Create the event loop for the LWM2M connection
	for {

		// Update changed data e.g. time, battery levels
		lwm2m.RefreshData()

		// Send the queued updates to the lwm2m server
		lwm2m.SendData()

		out = lwm2m.WaitForTimeout()
		if out != 0 {
			continue
		}

		// Read the responses from the lwm2m server
		lwm2m.ReadData()

		out = lwm2m.WaitForTimeout()
		if out != 0 {
			continue
		}
	}

}
