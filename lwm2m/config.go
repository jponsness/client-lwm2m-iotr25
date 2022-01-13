// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package lwm2m

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

const (
	defaultServerHost     = "localhost"
	defaultServerPort     = "5683"
	defaultLocalPort      = "56830"
	defaultClientName     = "alpaca"
	defaultBootstrap      = false
	defaultSerialVaultURL = "https://serial-vault-partners.canonical.com/v1/"
	defaultSerialVaultAPI = ""
	paramsEnvVar          = "SNAP_DATA"
	paramsFilename        = "params"
)

// ConfigParameters holds the parameters to configure the client service
type ConfigParameters struct {
	ServerHost        string `json:"serverhost"`
	ServerPort        string `json:"serverport"`
	LocalPort         string `json:"localport"`
	Name              string `json:"name"`
	BootstrapRequired bool   `json:"bootstrap"`
	SerialVaultURL    string `json:"url"`
	SerialVaultAPI    string `json:"api-key"`
}

// StoreParameters stores the configuration parameters on the filesystem
func StoreParameters(c ConfigParameters) error {
	path := getPath()

	// Default empty parameters
	if len(c.ServerHost) == 0 {
		c.ServerHost = defaultServerHost
	}
	if len(c.ServerPort) == 0 {
		c.ServerPort = defaultServerPort
	}
	if len(c.LocalPort) == 0 {
		c.LocalPort = defaultLocalPort
	}
	if len(c.Name) == 0 {
		c.Name = defaultClientName
	}
	if len(c.SerialVaultURL) == 0 {
		c.SerialVaultURL = defaultSerialVaultURL
	}
	if len(c.SerialVaultAPI) == 0 {
		c.SerialVaultAPI = defaultSerialVaultAPI
	}

	// Create the output file
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()

	// Convert the parameters to JSON
	b, err := json.Marshal(c)
	if err != nil {
		log.Printf("Error marshalling config parameters: %v", err)
		return err
	}

	// Output the JSON to the file
	_, err = f.Write(b)
	if err != nil {
		log.Printf("Error storing config parameters: %v", err)
		return err
	}
	f.Sync()

	// Restrict access to the file
	err = os.Chmod(path, 0600)
	return nil
}

// ReadParameters fetches the store config parameters
func ReadParameters() ConfigParameters {
	c := ConfigParameters{
		ServerHost:        defaultServerHost,
		ServerPort:        defaultServerPort,
		LocalPort:         defaultLocalPort,
		Name:              defaultClientName,
		BootstrapRequired: defaultBootstrap,
		SerialVaultURL:    defaultSerialVaultURL,
		SerialVaultAPI:    defaultSerialVaultAPI,
	}

	path := getPath()

	dat, err := ioutil.ReadFile(path)
	if err != nil {
		log.Printf("Error reading config parameters: %v", err)
		return c
	}

	err = json.Unmarshal(dat, &c)
	if err != nil {
		log.Printf("Error parsing config parameters: %v", err)
		return c
	}

	return c
}

func getPath() string {
	return fmt.Sprintf("%s/%s", os.Getenv(paramsEnvVar), paramsFilename)
}
