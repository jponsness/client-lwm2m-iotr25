// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package main

import (
	"fmt"
	"os"

	"github.com/jessevdk/go-flags"
	"launchpad.net/ce-web/alpaca/configure"
)

func main() {
	parser := flags.NewParser(&configure.Configure, flags.HelpFlag)
	_, err := parser.Parse()

	if err != nil {
		if e, ok := err.(*flags.Error); ok {
			if e.Type == flags.ErrHelp || e.Type == flags.ErrCommandRequired {
				parser.WriteHelp(os.Stdout)
				os.Exit(0)
			}
		}
		fmt.Println(err)
		os.Exit(1)
	}

	os.Exit(0)
}
