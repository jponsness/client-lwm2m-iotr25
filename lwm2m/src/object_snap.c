// Ubuntu Management Agent
// Copyright 2017 Canonical Ltd.  All rights reserved.

#include "liblwm2m.h"
#include "lwm2mclient.h"
#include "gocallbacks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define SNAP_RESOURCES                      10


static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_data_t ** dataArrayP,
                        lwm2m_object_t * objectP)
{
    lwm2m_list_t * targetP;
    int i;

    // Check that we have the instance in the list
    targetP = lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0)
    {
        // Full object requested
        *dataArrayP = lwm2m_data_new(SNAP_RESOURCES);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = SNAP_RESOURCES;
        for (i = 0 ; i < *numDataP ; i++) {
            (*dataArrayP)[i].id = i;
        }

        for (i = 0 ; i < *numDataP ; i++)
        {
    
            char * value = SnapInstanceRead(instanceId, i);
            lwm2m_data_encode_string(value, *dataArrayP + i);
        }
    } else {
        // Read a single instance
        char * value = SnapInstanceRead(instanceId, (**dataArrayP).id);
        lwm2m_data_encode_string(value, *dataArrayP);
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    lwm2m_list_t * targetP;
    int i;

    targetP = lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    for (i = 0 ; i < numData ; i++)
    {
        char * value = SnapInstanceRead(instanceId, dataArray[i].id);
        lwm2m_data_encode_string(value, dataArray + i);
    }

    return COAP_204_CHANGED;
}


static uint8_t prv_discover(uint16_t instanceId,
                            int * numDataP,
                            lwm2m_data_t ** dataArrayP,
                            lwm2m_object_t * objectP)
{
    int i;

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(SNAP_RESOURCES);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = SNAP_RESOURCES;
    }
    return COAP_205_CONTENT;
}


uint8_t prv_delete(uint16_t id,
                    lwm2m_object_t * objectP)
{
    lwm2m_list_t * targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, &targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}


static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        uint8_t * buffer,
                        int length,
                        lwm2m_object_t * objectP)
{
    char action[200];
    char * result;

    if (NULL == lwm2m_list_find(objectP->instanceList, instanceId)) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        fprintf(stdout, "\r\n-----------------\r\n"
                        "Execute on %hu/%d/%d\r\n"
                        " Parameter (%d bytes):\r\n",
                        objectP->objID, instanceId, resourceId, length);

        result = SnapInstanceExecute(instanceId, resourceId);
        fprintf(stdout, "%s\n", buffer);
        fprintf(stdout, "%s\n", result);
        fprintf(stdout, "-----------------\r\n\r\n");
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

void display_snap_object(lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    fprintf(stdout, "  /%u: Snap object, instances:\r\n", object->objID);
    prv_instance_t * instance = (prv_instance_t *)object->instanceList;
    while (instance != NULL)
    {
        fprintf(stdout, "    /%u/%u: Id: %u\r\n",
                object->objID, instance->id);
        instance = (prv_instance_t *)instance->next;
    }
#endif
}

lwm2m_object_t * get_snap_object(void)
{
    lwm2m_object_t * snapObj;

    snapObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != snapObj)
    {
        int i;
        lwm2m_list_t * targetP;

        memset(snapObj, 0, sizeof(lwm2m_object_t));

        snapObj->objID = LWM2M_SNAP_OBJECT_ID;

        // Go callback to get the number of snaps installed
        int count = GetSnapCount();

        // Initialize the instance list for each snap
        for (i=0 ; i < count ; i++)
        {
            targetP = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
            if (NULL == targetP) return NULL;
            memset(targetP, 0, sizeof(lwm2m_list_t));
            targetP->id = i;
            snapObj->instanceList = LWM2M_LIST_ADD(snapObj->instanceList, targetP);
        }

        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        snapObj->readFunc = prv_read;
        snapObj->writeFunc = prv_write;
        snapObj->executeFunc = prv_exec;
        //snapObj->createFunc = prv_create;
        snapObj->deleteFunc = prv_delete;
        snapObj->discoverFunc = prv_discover;
    }

    return snapObj;
}

void free_snap_object(lwm2m_object_t * object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}