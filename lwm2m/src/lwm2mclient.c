/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Benjamin Cabé - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Christian Renz - Please refer to git log
 *    Ricky Liu - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>
 Bosch Software Innovations GmbH - Please refer to git log

*/

// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.


#include "liblwm2m.h"
#ifdef WITH_TINYDTLS
#include "dtlsconnection.h"
#else
#include "connection.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include "lwm2mclient.h"
#include "gocallbacks.h"

extern lwm2m_object_t * get_object_device(void);
extern void display_device_object(lwm2m_object_t * objectP);
extern void free_object_device(lwm2m_object_t * objectP);

extern lwm2m_object_t * get_security_object(int serverId, const char* serverUri,
     char * bsPskId, char * psk, uint16_t pskLen, bool isBootstrap);
extern void clean_security_object(lwm2m_object_t * objectP);
extern void display_security_object(lwm2m_object_t * objectP);
extern void copy_security_object(lwm2m_object_t * objectDest, lwm2m_object_t * objectSrc);

extern char * get_server_uri(lwm2m_object_t * objectP, uint16_t secObjInstID);
extern lwm2m_object_t * get_server_object(int serverId, const char* binding, int lifetime, bool storing);
extern void clean_server_object(lwm2m_object_t * object);
extern void display_server_object(lwm2m_object_t * objectP);
extern void copy_server_object(lwm2m_object_t * objectDest, lwm2m_object_t * objectSrc);

extern lwm2m_object_t * get_snap_control_object(void);
extern void display_snap_control_object(lwm2m_object_t * object);
extern void free_snap_control_object(lwm2m_object_t * object);

extern lwm2m_object_t * get_snap_object(void);
extern void display_snap_object(lwm2m_object_t * object);
extern void free_snap_object(lwm2m_object_t * object);

extern void init_value_change(lwm2m_context_t * lwm2m);
extern void sendFullObjectList();

#define MAX_PACKET_SIZE 1024

int g_reboot = 0;
int lifetime = 30;
char * clientName;
int totalSnaps = 0;

// only backup security and server objects
# define BACKUP_OBJECT_COUNT 2
lwm2m_object_t * backupObjectArray[BACKUP_OBJECT_COUNT];

typedef struct
{
    lwm2m_object_t * securityObjP;
    lwm2m_object_t * serverObject;
    int sock;
#ifdef WITH_TINYDTLS
    dtls_connection_t * connList;
    lwm2m_context_t * lwm2mH;
#else
    connection_t * connList;
#endif
    int addressFamily;
} client_data_t;


#ifdef WITH_TINYDTLS
void * lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
  client_data_t * dataP;
  lwm2m_list_t * instance;
  dtls_connection_t * newConnP = NULL;
  dataP = (client_data_t *)userData;
  lwm2m_object_t  * securityObj = dataP->securityObjP;

  instance = LWM2M_LIST_FIND(dataP->securityObjP->instanceList, secObjInstID);
  if (instance == NULL) return NULL;


  newConnP = connection_create(dataP->connList, dataP->sock, securityObj, instance->id, dataP->lwm2mH, dataP->addressFamily);
  if (newConnP == NULL)
  {
      fprintf(stderr, "Connection creation failed.\n");
      return NULL;
  }

  dataP->connList = newConnP;
  return (void *)newConnP;
}
#else
void * lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
    client_data_t * dataP;
    char * uri;
    char * host;
    char * port;
    connection_t * newConnP = NULL;

    dataP = (client_data_t *)userData;

    uri = get_server_uri(dataP->securityObjP, secObjInstID);

    if (uri == NULL) return NULL;

    fprintf(stdout, "Connecting to %s\r\n", uri);

    // parse uri in the form "coaps://[host]:[port]"
    if (0 == strncmp(uri, "coaps://", strlen("coaps://"))) {
        host = uri+strlen("coaps://");
    }
    else if (0 == strncmp(uri, "coap://", strlen("coap://"))) {
        host = uri+strlen("coap://");
    }
    else {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL) goto exit;
    // remove brackets
    if (host[0] == '[')
    {
        host++;
        if (*(port - 1) == ']')
        {
            *(port - 1) = 0;
        }
        else goto exit;
    }
    // split strings
    *port = 0;
    port++;

    fprintf(stderr, "Opening connection to server at %s:%s\r\n", host, port);
    newConnP = connection_create(dataP->connList, dataP->sock, host, port, dataP->addressFamily);
    if (newConnP == NULL) {
        fprintf(stderr, "Connection creation failed.\r\n");
    }
    else {
        dataP->connList = newConnP;
    }

exit:
    lwm2m_free(uri);
    return (void *)newConnP;
}
#endif

void lwm2m_close_connection(void * sessionH,
                            void * userData)
{
    client_data_t * app_data;
#ifdef WITH_TINYDTLS
    dtls_connection_t * targetP;
#else
    connection_t * targetP;
#endif

    app_data = (client_data_t *)userData;
#ifdef WITH_TINYDTLS
    targetP = (dtls_connection_t *)sessionH;
#else
    targetP = (connection_t *)sessionH;
#endif

    if (targetP == app_data->connList)
    {
        app_data->connList = targetP->next;
        lwm2m_free(targetP);
    }
    else
    {
#ifdef WITH_TINYDTLS
        dtls_connection_t * parentP;
#else
        connection_t * parentP;
#endif

        parentP = app_data->connList;
        while (parentP != NULL && parentP->next != targetP)
        {
            parentP = parentP->next;
        }
        if (parentP != NULL)
        {
            parentP->next = targetP->next;
            lwm2m_free(targetP);
        }
    }
}

void print_state(lwm2m_context_t * lwm2mH)
{
    lwm2m_server_t * targetP;

    fprintf(stderr, "State: ");
    switch(lwm2mH->state)
    {
    case STATE_INITIAL:
        fprintf(stderr, "STATE_INITIAL");
        break;
    case STATE_BOOTSTRAP_REQUIRED:
        fprintf(stderr, "STATE_BOOTSTRAP_REQUIRED");
        break;
    case STATE_BOOTSTRAPPING:
        fprintf(stderr, "STATE_BOOTSTRAPPING");
        break;
    case STATE_REGISTER_REQUIRED:
        fprintf(stderr, "STATE_REGISTER_REQUIRED");
        break;
    case STATE_REGISTERING:
        fprintf(stderr, "STATE_REGISTERING");
        break;
    case STATE_READY:
        fprintf(stderr, "STATE_READY");
        break;
    default:
        fprintf(stderr, "Unknown !");
        break;
    }
    fprintf(stderr, "\r\n");

    targetP = lwm2mH->bootstrapServerList;

    if (lwm2mH->bootstrapServerList == NULL)
    {
        fprintf(stderr, "No Bootstrap Server.\r\n");
    }
    else
    {
        fprintf(stderr, "Bootstrap Servers:\r\n");
        for (targetP = lwm2mH->bootstrapServerList ; targetP != NULL ; targetP = targetP->next)
        {
            fprintf(stderr, " - Security Object ID %d", targetP->secObjInstID);
            fprintf(stderr, "\tHold Off Time: %lu s", (unsigned long)targetP->lifetime);
            fprintf(stderr, "\tstatus: ");
            switch(targetP->status)
            {
            case STATE_DEREGISTERED:
                fprintf(stderr, "DEREGISTERED\r\n");
                break;
            case STATE_BS_HOLD_OFF:
                fprintf(stderr, "CLIENT HOLD OFF\r\n");
                break;
            case STATE_BS_INITIATED:
                fprintf(stderr, "BOOTSTRAP INITIATED\r\n");
                break;
            case STATE_BS_PENDING:
                fprintf(stderr, "BOOTSTRAP PENDING\r\n");
                break;
            case STATE_BS_FINISHED:
                fprintf(stderr, "BOOTSTRAP FINISHED\r\n");
                break;
            case STATE_BS_FAILED:
                fprintf(stderr, "BOOTSTRAP FAILED\r\n");
                break;
            default:
                fprintf(stderr, "INVALID (%d)\r\n", (int)targetP->status);
            }
            fprintf(stderr, "\r\n");
        }
    }

    if (lwm2mH->serverList == NULL)
    {
        fprintf(stderr, "No LWM2M Server.\r\n");
    }
    else
    {
        fprintf(stderr, "LWM2M Servers:\r\n");
        for (targetP = lwm2mH->serverList ; targetP != NULL ; targetP = targetP->next)
        {
            fprintf(stderr, " - Server ID %d", targetP->shortID);
            fprintf(stderr, "\tstatus: ");
            switch(targetP->status)
            {
            case STATE_DEREGISTERED:
                fprintf(stderr, "DEREGISTERED\r\n");
                break;
            case STATE_REG_PENDING:
                fprintf(stderr, "REGISTRATION PENDING\r\n");
                break;
            case STATE_REGISTERED:
                fprintf(stderr, "REGISTERED\tlocation: \"%s\"\tLifetime: %lus\r\n", targetP->location, (unsigned long)targetP->lifetime);
                break;
            case STATE_REG_UPDATE_PENDING:
                fprintf(stderr, "REGISTRATION UPDATE PENDING\r\n");
                break;
            case STATE_REG_FULL_UPDATE_NEEDED:
                fprintf(stderr, "REGISTRATION FULL UPDATE REQUIRED\r\n");
                break;
            case STATE_REG_UPDATE_NEEDED:
                fprintf(stderr, "REGISTRATION UPDATE REQUIRED\r\n");
                break;
            case STATE_DEREG_PENDING:
                fprintf(stderr, "DEREGISTRATION PENDING\r\n");
                break;
            case STATE_REG_FAILED:
                fprintf(stderr, "REGISTRATION FAILED\r\n");
                break;
            default:
                fprintf(stderr, "INVALID (%d)\r\n", (int)targetP->status);
            }
            fprintf(stderr, "\r\n");
        }
    }
}

#define OBJ_COUNT 5

client_data_t data;
lwm2m_context_t * lwm2mH = NULL;
lwm2m_object_t * objArray[OBJ_COUNT];

#ifdef LWM2M_BOOTSTRAP
lwm2m_client_state_t previousState = STATE_INITIAL;
#endif

char * pskId = NULL;
char * psk = NULL;
uint16_t pskLen = -1;
char * pskBuffer = NULL;

// Timing for the send/receive connections to the server
struct timeval tv;
fd_set readfds;


#ifdef LWM2M_BOOTSTRAP

static void prv_initiate_bootstrap(char * buffer,
                                   void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *)user_data;
    lwm2m_server_t * targetP;

    // HACK !!!
    lwm2mH->state = STATE_BOOTSTRAP_REQUIRED;
    targetP = lwm2mH->bootstrapServerList;
    while (targetP != NULL)
    {
        targetP->lifetime = 0;
        targetP = targetP->next;
    }
}

static void prv_display_objects(char * buffer,
                                void * user_data)
{
    lwm2m_context_t * lwm2mH = (lwm2m_context_t *)user_data;
    lwm2m_object_t * object;

    for (object = lwm2mH->objectList; object != NULL; object = object->next){
        if (NULL != object) {
            switch (object->objID)
            {
            case LWM2M_SECURITY_OBJECT_ID:
                display_security_object(object);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                display_server_object(object);
                break;
            case LWM2M_ACL_OBJECT_ID:
                break;
            case LWM2M_DEVICE_OBJECT_ID:
                display_device_object(object);
                break;
            case LWM2M_SNAP_CONTROL_OBJECT_ID:
                display_snap_control_object(object);
                break;
            case LWM2M_SNAP_OBJECT_ID:
                display_snap_object(object);
                break;
            }
        }
    }
}

static void prv_display_backup(char * buffer,
        void * user_data)
{
   int i;
   for (i = 0 ; i < BACKUP_OBJECT_COUNT ; i++) {
       lwm2m_object_t * object = backupObjectArray[i];
       if (NULL != object) {
           switch (object->objID)
           {
           case LWM2M_SECURITY_OBJECT_ID:
               display_security_object(object);
               break;
           case LWM2M_SERVER_OBJECT_ID:
               display_server_object(object);
               break;
           default:
               break;
           }
       }
   }
}

static void prv_backup_objects(lwm2m_context_t * context)
{
    uint16_t i;

    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        if (NULL != backupObjectArray[i]) {
            switch (backupObjectArray[i]->objID)
            {
            case LWM2M_SECURITY_OBJECT_ID:
                clean_security_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                clean_server_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            default:
                break;
            }
        }
        backupObjectArray[i] = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
        memset(backupObjectArray[i], 0, sizeof(lwm2m_object_t));
    }

    /*
     * Backup content of objects 0 (security) and 1 (server)
     */
    copy_security_object(backupObjectArray[0], (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID));
    copy_server_object(backupObjectArray[1], (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID));
}

static void prv_restore_objects(lwm2m_context_t * context)
{
    lwm2m_object_t * targetP;

    /*
     * Restore content  of objects 0 (security) and 1 (server)
     */
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID);
    // first delete internal content
    clean_security_object(targetP);
    // then restore previous object
    copy_security_object(targetP, backupObjectArray[0]);

    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID);
    // first delete internal content
    clean_server_object(targetP);
    // then restore previous object
    copy_server_object(targetP, backupObjectArray[1]);

    // restart the old servers
    fprintf(stdout, "[BOOTSTRAP] ObjectList restored\r\n");
}

static void close_backup_object()
{
    int i;
    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        if (NULL != backupObjectArray[i]) {
            switch (backupObjectArray[i]->objID)
            {
            case LWM2M_SECURITY_OBJECT_ID:
                clean_security_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                clean_server_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            default:
                break;
            }
        }
    }
}

static void update_bootstrap_info(lwm2m_client_state_t * previousBootstrapState,
        lwm2m_context_t * context)
{
    // Send the full object list when we've just registered
    if ((*previousBootstrapState == STATE_REGISTERING) && (context->state == STATE_READY)) {
        sendFullObjectList();
    }

    if (*previousBootstrapState != context->state)
    {
        *previousBootstrapState = context->state;
        switch(context->state)
        {
            case STATE_BOOTSTRAPPING:
#ifdef WITH_LOGS
                fprintf(stdout, "[BOOTSTRAP] backup security and server objects\r\n");
#endif
                prv_backup_objects(context);
                break;
            default:
                break;
        }
    }
}

#endif



int createServer(
        char server[50], char port[5], char localPort[5], char name[20],
        int bootstrapRequested)
{
    int result;
    int opt;

    clientName = name;

    memset(&data, 0, sizeof(client_data_t));

    data.addressFamily = AF_INET;   // Default to IPv4

    /*
     *This call an internal function that create an IPv6 socket on the port 5683.
     */
    fprintf(stderr, "Trying to bind LWM2M Client to port %s\r\n", localPort);
    data.sock = create_socket(localPort, data.addressFamily);
    if (data.sock < 0)
    {
        fprintf(stderr, "Failed to open socket: %d %s\r\n", errno, strerror(errno));
        return -1;
    }

    /*
     * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
     * Those functions are located in their respective object file.
     */
#ifdef WITH_TINYDTLS
    if (psk != NULL)
    {
        pskLen = strlen(psk) / 2;
        pskBuffer = malloc(pskLen);

        if (NULL == pskBuffer)
        {
            fprintf(stderr, "Failed to create PSK binary buffer\r\n");
            return -1;
        }
        // Hex string to binary
        char *h = psk;
        char *b = pskBuffer;
        char xlate[] = "0123456789ABCDEF";

        for ( ; *h; h += 2, ++b)
        {
            char *l = strchr(xlate, toupper(*h));
            char *r = strchr(xlate, toupper(*(h+1)));

            if (!r || !l)
            {
                fprintf(stderr, "Failed to parse Pre-Shared-Key HEXSTRING\r\n");
                return -1;
            }

            *b = ((l - xlate) << 4) + (r - xlate);
        }
    }
#endif

     char serverUri[50];
     int serverId = 123;
     sprintf (serverUri, "coap://%s:%s", server, port);

#ifdef LWM2M_BOOTSTRAP
    bool bootstrap = false;
    if (bootstrapRequested == 1) {
        bootstrap = true;
    }
    objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, bootstrap);
#else
    objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, false);
#endif
    if (NULL == objArray[0])
    {
        fprintf(stderr, "Failed to create security object\r\n");
        return -1;
    }
    data.securityObjP = objArray[0];

    objArray[1] = get_server_object(serverId, "U", lifetime, false);
    if (NULL == objArray[1])
    {
        fprintf(stderr, "Failed to create server object\r\n");
        return -1;
    }

    objArray[2] = get_object_device();
    if (NULL == objArray[2])
    {
        fprintf(stderr, "Failed to create Device object\r\n");
        return -1;
    }

    objArray[3] = get_snap_control_object();
    if (NULL == objArray[3])
    {
        fprintf(stderr, "Failed to create Snap control object\r\n");
        return -1;
    }

    objArray[4] = get_snap_object();
    if (NULL == objArray[4])
    {
        fprintf(stderr, "Failed to create Snap list object\r\n");
        return -1;
    }

    /*
     * The liblwm2m library is now initialized with the functions that will be in
     * charge of communication
     */
    lwm2mH = lwm2m_init(&data);
    if (NULL == lwm2mH)
    {
        fprintf(stderr, "lwm2m_init() failed\r\n");
        return -1;
    }

#ifdef WITH_TINYDTLS
    data.lwm2mH = lwm2mH;
#endif

    /*
     * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
     * the number of objects we will be passing through and the objects array
     */
    result = lwm2m_configure(lwm2mH, name, NULL, NULL, OBJ_COUNT, objArray);
    if (result != 0)
    {
        fprintf(stderr, "lwm2m_configure() failed: 0x%X\r\n", result);
        return -1;
    }

    // Initialize the value changed callback
    init_value_change(lwm2mH);

    // Initialize the count of snaps
    totalSnaps = GetSnapCount();

    return 0;

}

void handle_value_changed(lwm2m_context_t * lwm2mH,
                          lwm2m_uri_t * uri,
                          const char * value,
                          size_t valueLength)
{
    lwm2m_object_t * object = (lwm2m_object_t *)LWM2M_LIST_FIND(lwm2mH->objectList, uri->objectId);

    if (NULL != object)
    {
        if (object->writeFunc != NULL)
        {
            lwm2m_data_t * dataP;
            int result;

            dataP = lwm2m_data_new(1);
            if (dataP == NULL)
            {
                fprintf(stderr, "Internal allocation failure !\n");
                return;
            }
            dataP->id = uri->resourceId;
            lwm2m_data_encode_nstring(value, valueLength, dataP);

            // JBP change
            // result = object->writeFunc(uri->instanceId, 1, dataP, object);
            result = object->writeFunc(lwm2mH, uri->instanceId, 1, dataP, object, LWM2M_WRITE_PARTIAL_UPDATE);

            if (COAP_204_CHANGED != result)
            {
                //fprintf(stderr, "Failed to change value!\n");
            }
            else
            {
                // Indicate that the value has changed
                lwm2m_resource_value_changed(lwm2mH, uri);
            }
            lwm2m_data_free(1, dataP);
            return;
        }
        else
        {
            fprintf(stderr, "write not supported for specified resource!\n");
        }
        return;
    }
    else
    {
        fprintf(stderr, "Object not found !\n");
    }
}

int closeServer() {

    /*
     * Finally when the loop is left, we unregister our client from it
     */

#ifdef WITH_TINYDTLS
    free(pskBuffer);
#endif

#ifdef LWM2M_BOOTSTRAP
    close_backup_object();
#endif
    lwm2m_close(lwm2mH);
    close(data.sock);
    connection_free(data.connList);

    clean_security_object(objArray[0]);
    clean_server_object(objArray[1]);
    free_object_device(objArray[2]);
    free_snap_control_object(objArray[3]);
    free_snap_object(objArray[4]);

    fprintf(stdout, "\r\n\n");

    return 0;

}

// Waits for the timeout set by the previous function's call
int waitForTimeout() {

    int result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

    if (result < 0)
    {
        if (errno != EINTR)
        {
            fprintf(stderr, "Error in select(): %d %s\r\n", errno, strerror(errno));
        }
        return -1;
    }
    return 0;
}

int sendData() {

    int result;

    // Set the timeout value
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(data.sock, &readfds);

    print_state(lwm2mH);

    /*
    * This function does two things:
    *  - first it does the work needed by liblwm2m (eg. (re)sending some packets).
    *  - Secondly it adjusts the timeout value (default 60s) depending on the state of the transaction
    *    (eg. retransmission) and the time before the next operation
    */
    result = lwm2m_step(lwm2mH, &(tv.tv_sec));
    fprintf(stdout, " -> State: ");
    switch (lwm2mH->state)
    {
    case STATE_INITIAL:
        fprintf(stdout, "STATE_INITIAL\r\n");
        break;
    case STATE_BOOTSTRAP_REQUIRED:
        fprintf(stdout, "STATE_BOOTSTRAP_REQUIRED\r\n");
        break;
    case STATE_BOOTSTRAPPING:
        fprintf(stdout, "STATE_BOOTSTRAPPING\r\n");
        break;
    case STATE_REGISTER_REQUIRED:
        fprintf(stdout, "STATE_REGISTER_REQUIRED\r\n");
        break;
    case STATE_REGISTERING:
        fprintf(stdout, "STATE_REGISTERING\r\n");
        break;
    case STATE_READY:
        fprintf(stdout, "STATE_READY\r\n");
        break;
    default:
        fprintf(stdout, "Unknown...\r\n");
        break;
    }
    if (result != 0)
    {
        fprintf(stderr, "lwm2m_step() failed: 0x%X\r\n", result);
        if(previousState == STATE_BOOTSTRAPPING)
        {
#ifdef WITH_LOGS
            fprintf(stdout, "[BOOTSTRAP] restore security and server objects\r\n");
#endif
            printf("[BOOTSTRAP] restore security and server objects\r\n");
            prv_restore_objects(lwm2mH);
            lwm2mH->state = STATE_INITIAL;
        }
        else return -1;
    }

#ifdef LWM2M_BOOTSTRAP
    update_bootstrap_info(&previousState, lwm2mH);
#endif

    return 0;
}

int readData() {

    uint8_t buffer[MAX_PACKET_SIZE];
    int numBytes;

    /*
     * If an event happens on the socket
     */
    if (FD_ISSET(data.sock, &readfds))
    {
        struct sockaddr_storage addr;
        socklen_t addrLen;

        addrLen = sizeof(addr);

        /*
         * We retrieve the data received
         */
        numBytes = recvfrom(data.sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

        if (0 > numBytes)
        {
            fprintf(stderr, "Error in recvfrom(): %d %s\r\n", errno, strerror(errno));
        }
        else if (0 < numBytes)
        {
#ifdef WITH_TINYDTLS
            dtls_connection_t * connP;
#else
            connection_t * connP;
#endif

            connP = connection_find(data.connList, &addr, addrLen);
            if (connP != NULL)
            {
                /*
                 * Let liblwm2m respond to the query depending on the context
                 */
#ifdef WITH_TINYDTLS
                int result = connection_handle_packet(connP, buffer, numBytes);
                if (0 != result)
                {
                     printf("error handling message %d\n",result);
                }
#else
                lwm2m_handle_packet(lwm2mH, buffer, numBytes, connP);
#endif
            }
            else
            {
                /*
                 * This packet comes from an unknown peer
                 */
                fprintf(stderr, "received bytes ignored!\r\n");
            }
        }
    }
}

// Update a resource that has had its value refreshed
void handleValueRefresh(
        char * resourceUri,
        int resourceUriLength,
        char * value,
        int valueLength) {
    lwm2m_uri_t uri;

    if (lwm2m_stringToUri(resourceUri, resourceUriLength, &uri)) {
        handle_value_changed(lwm2mH, &uri, value, valueLength);
        tv.tv_sec = 10;
    }

    refreshObjects();
    tv.tv_sec = 10;
}

void sendFullObjectList() {
    lwm2m_remove_object(lwm2mH, LWM2M_SNAP_OBJECT_ID);
    free_snap_object(objArray[4]);
    objArray[4] = get_snap_object();
    lwm2m_add_object(lwm2mH, objArray[4]);
    tv.tv_sec = 10;
}

// Send a full object registry update to the server
void refreshObjects() {

    // Go callback to get the number of snaps installed
    int count = GetSnapCount();

    // If the number of snaps changes, then send the full object list
    if (count != totalSnaps) {
        sendFullObjectList();
        totalSnaps = count;
    }
}
