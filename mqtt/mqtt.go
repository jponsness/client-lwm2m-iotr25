// -*- Mode: Go; indent-tabs-mode: t -*-
// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

package mqtt

import (
	"fmt"
	"log"

	"github.com/eclipse/paho.mqtt.golang"
)

// Constants for connecting to the MQTT broker
const (
	quiesce        = 250
	QOSAtMostOnce  = byte(0)
	QOSAtLeastOnce = byte(1)
	QOSExactlyOnce = byte(2)
)

// Connection for MQTT protocol
type Connection struct {
	client mqtt.Client
}

// NewConnection creates an MQTT connection
func NewConnection(serverHost, serverPort, name, username, password string) (*Connection, error) {
	url := fmt.Sprintf("tcp://%s:%s", serverHost, serverPort)

	log.Println("Connect to the MQTT broker", url)
	opts := mqtt.NewClientOptions().AddBroker(url)
	opts.SetClientID(name)
	opts.SetUsername(username)
	opts.SetPassword(password)

	client := mqtt.NewClient(opts)

	if token := client.Connect(); token.Wait() && token.Error() != nil {
		return nil, token.Error()
	}

	return &Connection{
		client: client,
	}, nil
}

// Publish sends data to the MQTT broker
func (c *Connection) Publish(topic, payload string) error {
	token := c.client.Publish(topic, QOSAtLeastOnce, false, payload)
	token.Wait()
	if token.Error() != nil {
		return token.Error()
	}
	return nil
}

// Subscribe starts a new subscription, providing a message handler for the topic
func (c *Connection) Subscribe(topic string, callback mqtt.MessageHandler) error {
	token := c.client.Subscribe(topic, QOSAtLeastOnce, callback)
	token.Wait()
	if token.Error() != nil {
		return token.Error()
	}
	return nil
}

// Close closes the connection to the MQTT broker
func (c *Connection) Close() {
	c.client.Disconnect(quiesce)
}
