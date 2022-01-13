// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package configure

import (
	"fmt"
	"log"

	"launchpad.net/ce-web/alpaca/lwm2m"
)

// ConfigCommand defines the options for the configure command-line utility
type ConfigCommand struct {
	ServerHost     string `short:"s" long:"server" description:"Hostname of the LWM2M Server" default:"localhost"`
	ServerPort     string `short:"p" long:"port" description:"Port of the LWM2M Server" default:"5683"`
	LocalPort      string `short:"l" long:"localport" description:"Local port for the LWM2M client" default:"56830"`
	Name           string `short:"n" long:"name" description:"Name of the LWM2M client" default:"lwm2mclient"`
	Bootstrap      string `short:"b" long:"bootstrap" description:"Whether bootstrap is required" default:"false" choice:"false" choice:"true"`
	SerialVaultURL string `short:"u" long:"url" description:"URL to the serial-vault" default:"https://serial-vault-partners.canonical.com/v1/"`
	SerialVaultAPI string `short:"a" long:"apikey" description:"API key for the serial-vault"`
}

// Execute the adding a new user
func (cmd ConfigCommand) Execute(args []string) error {

	c := lwm2m.ConfigParameters{
		ServerHost:        cmd.ServerHost,
		ServerPort:        cmd.ServerPort,
		LocalPort:         cmd.LocalPort,
		Name:              cmd.Name,
		BootstrapRequired: false,
		SerialVaultURL:    cmd.SerialVaultURL,
		SerialVaultAPI:    cmd.SerialVaultAPI,
	}

	log.Println("---", cmd.Bootstrap)
	if cmd.Bootstrap == "true" {
		c.BootstrapRequired = true
	}

	err := lwm2m.StoreParameters(c)
	if err != nil {
		fmt.Printf("Error storing the credentials: %v", err)
		return err
	}

	return nil
}
