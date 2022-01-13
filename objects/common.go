// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package objects

import (
	"time"
)

// Only refresh the data if that last retrieval was older than this time
const apiRefresh = 10

func dataIsStale(lastRefresh int64) bool {
	if time.Now().Unix()-lastRefresh > apiRefresh {
		return true
	}
	return false
}
