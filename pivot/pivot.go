// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package pivot

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strconv"

	"github.com/snapcore/snapd/asserts"
	"launchpad.net/ce-web/alpaca/snapdapi"
)

const (
	paramsEnvVar    = "SNAP_DATA"
	assertsFilename = "assertion"
	pivotModelFile  = "pivot-model"
	assertsPath     = "/var/lib/snapd/assertions/asserts-v0/model"
	seedPath        = "/var/lib/snapd/seed/assertions/asserts-v0/model"
)

// ModelAssertionRequest is the JSON version of a model assertion request
type ModelAssertionRequest struct {
	BrandID string `json:"brand-id"`
	Name    string `json:"model"`
}

// BooleanResponse is the JSON response from an API method, indicating success or failure.
type BooleanResponse struct {
	Success      bool   `json:"success"`
	ErrorCode    string `json:"error_code"`
	ErrorSubcode string `json:"error_subcode"`
	ErrorMessage string `json:"message"`
}

// VerifyPivot checks if the device needs to be pivoted to a reseller model
func VerifyPivot(serialVaultURL, serialVaultAPIKey string) error {
	// Check if the device has been pivoted
	pivotFile := GetPath(pivotModelFile, "")
	if _, err := os.Stat(pivotModelFile); err == nil {
		// The file exists, so the pivoting is done
		return nil
	}

	// Get current model details and the serial assertion
	snappy := snapdapi.NewClientAdapter()
	deviceInfo, err := snapdapi.GetModelInfo(snappy)
	if err != nil {
		log.Println("Error retrieving the model info:", err)
		return err
	}

	// Get the current model assertion
	modelAssertions, err := snappy.Known(asserts.ModelType.Name, map[string]string{})
	if err != nil || len(modelAssertions) == 0 {
		log.Println("Error retrieving the serial assertion:", err)
		return err
	}

	// Log the old model to a file, so it can be the pivoted on reboot
	// - Remove old model assertion
	if err := logAssertionRemoval(pivotFile, modelAssertions[0]); err != nil {
		log.Println("Error creating the pivot file:", err)
		return err
	}

	serialAssertions, err := snappy.Known(asserts.SerialType.Name, map[string]string{})
	if err != nil || len(serialAssertions) == 0 {
		log.Println("Error retrieving the serial assertion:", err)
		return err
	}
	// Call home to the server to fetch the model assertion and store it
	if err := fetchAndStoreAssertions(pivotFile, snappy, serialAssertions[0], deviceInfo, serialVaultURL, serialVaultAPIKey); err != nil {
		log.Println("Error:", err)
		return err
	}

	// Reboot the device
	log.Println("-----------TODO: reboot device")
	return nil
}

func logAssertionRemoval(pivotFile string, oldModel asserts.Assertion) error {
	f, err := os.Create(pivotFile)
	if err != nil {
		return err
	}
	defer f.Close()

	// Log the old assertion that needs to be removed
	removeModelAssert := fmt.Sprintf(
		"-%s/%s/%s/%s/active\n",
		assertsPath, oldModel.HeaderString("series"), oldModel.HeaderString("brand-id"), oldModel.HeaderString("model"),
	)
	_, err = f.WriteString(removeModelAssert)
	return err
}

func logAssertionCopy(pivotFile string, newModel asserts.Assertion) error {
	f, err := os.OpenFile(pivotFile, os.O_APPEND|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	// Log the new assertion that needs to be copied
	copyModelAssert := fmt.Sprintf(
		"+%s/%s/%s/%s/active %s/%s/%s/%s/\n",
		assertsPath, newModel.HeaderString("series"), newModel.HeaderString("brand-id"), newModel.HeaderString("model"),
		seedPath, newModel.HeaderString("series"), newModel.HeaderString("brand-id"), newModel.HeaderString("model"),
	)
	_, err = f.WriteString(copyModelAssert)
	return err
}

func fetchAndStoreAssertions(pivotFile string, snappy *snapdapi.ClientAdapter, serialAssert asserts.Assertion, deviceInfo snapdapi.DeviceInfo, serialVaultURL, serialVaultAPIKey string) error {
	assertions := []asserts.Assertion{}

	// Send the serial assertion to receive the pivoted model assertion
	err := getPivotAssertion(&assertions, "pivotmodel", serialAssert, serialVaultURL, serialVaultAPIKey)
	if err != nil {
		return err
	}

	// Also get the pivoted serial assertion
	err = getPivotAssertion(&assertions, "pivotserial", serialAssert, serialVaultURL, serialVaultAPIKey)
	if err != nil {
		return err
	}

	// Add the new assertions to the system
	for i, a := range assertions {
		err = snappy.Ack(asserts.Encode(a))
		if err != nil {
			log.Println("Error adding the assertion to the system:", err)
			return err
		}
		log.Println(string(asserts.Encode(a)))

		if a.Type().Name == asserts.ModelType.Name {
			err = logAssertionCopy(pivotFile, a)
			if err != nil {
				log.Println("Error adding the assertion to the system:", err)
				return err
			}
		}

		p := GetPath(assertsFilename, strconv.Itoa(i))
		log.Println("Save assertion to", p)
		err = ioutil.WriteFile(p, asserts.Encode(a), 0644)
		if err != nil {
			log.Println("Error storing the assertion on the filesystem:", err)
			return err
		}
	}

	return nil
}

// getPivotAssertion sends the original serial assertion to the Serial Vault and
// returns the related assertion(s) of the pivoted model
func getPivotAssertion(assertions *[]asserts.Assertion, endPoint string, serialAssert asserts.Assertion, serialVaultURL, serialVaultAPIKey string) error {

	url := serialVaultURL + endPoint
	data := asserts.Encode(serialAssert)
	headers := map[string]string{
		"api-key":      serialVaultAPIKey,
		"Content-Type": asserts.MediaType,
	}

	// Fetch the signed pivoted model assertion from the serial vault
	resp, err := submitPOSTRequest(url, headers, data)
	if err != nil {
		log.Println("Error fetching the pivot model assertion:", err)
		return err
	}

	if resp.StatusCode != 200 {
		result := BooleanResponse{}
		err = json.NewDecoder(resp.Body).Decode(&result)
		return errors.New("Error retrieving the model assertion")
	}

	defer resp.Body.Close()

	dec := asserts.NewDecoder(resp.Body)

	for {
		a, err := dec.Decode()
		if err != nil {
			break
		}
		*assertions = append(*assertions, a)
	}

	return err
}

func submitPOSTRequest(url string, headers map[string]string, data []byte) (*http.Response, error) {
	r, _ := http.NewRequest("POST", url, bytes.NewReader(data))

	for k, v := range headers {
		r.Header.Set(k, v)
	}
	client := http.Client{}
	resp, err := client.Do(r)
	if err != nil {
		return resp, err
	}

	return resp, nil
}

// GetPath formats the full path for a file in $SNAP_DATA
func GetPath(f, suffix string) string {
	return fmt.Sprintf("%s/%s%s", os.Getenv(paramsEnvVar), f, suffix)
}
