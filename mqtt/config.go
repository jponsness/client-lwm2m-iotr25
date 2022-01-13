// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package mqtt

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"os"
)

const (
	defaultServerHost = "localhost"
	defaultServerPort = "1883"
	defaultClientName = "alpaca-mqtt"
	paramsEnvVar      = "SNAP_DATA"
	paramsFilename    = "params-mqtt"
)

// ConfigParameters holds the parameters to configure the client service
type ConfigParameters struct {
	ServerHost string `json:"serverhost"`
	ServerPort string `json:"serverport"`
	Name       string `json:"name"`
	Username   string `json:"username"`
	Password   string `json:"password"`
}

// Config holds the config parameters for the application
var Config ConfigParameters

// ReadParameters fetches the store config parameters
func ReadParameters() ConfigParameters {
	Config = ConfigParameters{
		ServerHost: defaultServerHost,
		ServerPort: defaultServerPort,
		Name:       defaultClientName,
	}

	path := getPath()

	dat, err := ioutil.ReadFile(path)
	if err != nil {
		log.Printf("Error reading config parameters: %v", err)
		return Config
	}

	err = json.Unmarshal(dat, &Config)
	if err != nil {
		log.Printf("Error parsing config parameters: %v", err)
		return Config
	}

	return Config
}

func getPath() string {
	return fmt.Sprintf("%s/%s", os.Getenv(paramsEnvVar), paramsFilename)
}
