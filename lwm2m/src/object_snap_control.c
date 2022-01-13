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

#define SNAP_RESOURCES                      3


extern lwm2m_object_t * get_snap_object(void);
extern void free_snap_object(lwm2m_object_t * object);


static uint8_t prv_set_value(lwm2m_data_t * dataP)
{
    char * value;
    int i, val, count;

    // a simple switch structure is used to respond at the specified resource asked
    switch (dataP->id)
    {
    case 0:
        val = GetSnapCount();
        lwm2m_data_encode_int(val, dataP);
        return COAP_205_CONTENT;
    case 1:
        // Get the snaps into an array
        count = GetSnapCount();
        lwm2m_data_t* subTlvP = lwm2m_data_new(count);
        for (i=0; i < count; i++) {
            value = SnapDelimited(i);
            subTlvP[i].id = i;
            lwm2m_data_encode_string(value, subTlvP + i);
        }
        lwm2m_data_encode_instances(subTlvP, count, dataP);
        return COAP_205_CONTENT;
    case 2:
        // count = GetSnapCount();
        // for (i=0; i < count; i++) {
        //     SnapConfig(i);
        // }
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_data_t ** dataArrayP,
                        lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {0,1};
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_set_value((*dataArrayP) + i);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
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
        uint16_t resList[] = {
            0,
            1
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }
    return COAP_205_CONTENT;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    uint8_t result;
    int i = 0;

    do
    {
        result = prv_set_value(dataArray + i);
        i++;
    } while (i < numData && result == COAP_205_CONTENT);

    if (result == COAP_205_CONTENT) {
        return COAP_204_CHANGED;
    }

    return result;

}

static uint8_t prv_delete(uint16_t id,
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
    char name[200];
    char * result;

    if (NULL == lwm2m_list_find(objectP->instanceList, instanceId)) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        if (length > 200) {
            length = 199;
        }
        snprintf(name, length+1, "%s", buffer);
        result = SnapExecute(resourceId, name);
        fprintf(stdout, "%s\n", result);
        fprintf(stdout, "-----------------\r\n\r\n");
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

void display_snap_control_object(lwm2m_object_t * object)
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

lwm2m_object_t * get_snap_control_object(void)
{
    lwm2m_object_t * snapObj;

    snapObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != snapObj)
    {
        memset(snapObj, 0, sizeof(lwm2m_object_t));

        snapObj->objID = LWM2M_SNAP_CONTROL_OBJECT_ID;

        snapObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != snapObj->instanceList)
        {
            memset(snapObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(snapObj);
            return NULL;
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
        //snapObj->deleteFunc = prv_delete;
        snapObj->discoverFunc = prv_discover;
    }

    return snapObj;
}

void free_snap_control_object(lwm2m_object_t * object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        lwm2m_free(object->userData);
        object->userData = NULL;
    }
    lwm2m_free(object);
}
