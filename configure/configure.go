// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package configure

// Command defines the options for the configure command-line utility
type Command struct {
	Config ConfigCommand `command:"config" alias:"c" description:"Configure the client connections"`
}

// Configure is the implementation of the command configuration for the configure command-line
var Configure Command
