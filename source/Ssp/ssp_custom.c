/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include <ansc_platform.h>
#include <ccsp_base_api.h>
#include <ccsp_memory.h>
#include <ccsp_custom.h>

#include "ssp_global.h"
#include "ssp_hash.h"
#include "ssp_utils.h"


/**
 * Get all default value which need to be unique per box or any extra default value
 * For example: wifi ssid, password.
 * Params : The parameter structure to return
 * cnt    : How many parameters
 * return if return 0 (success) 
 */
int PsmHal_GetCustomParams_default( PsmHalParam_t **params, int *cnt)
{
    PsmHalParam_t * tmp = (PsmHalParam_t *)AnscAllocateMemory(sizeof(PsmHalParam_t));
    if(tmp != NULL ){
        strcpy(tmp->name, "test.customdefault.name1");
        strcpy(tmp->value, "test.customdefault.value1");

        CcspTraceWarning(("%s: default loading \n", __FUNCTION__));

        *params = tmp;
        *cnt = 1;
    }else{
        *params = NULL;
        *cnt = 0;
    }

    return 0;
}




/**
 * Get all values which need to be used for sure even home user changed it.
 * This just align with existing PSM. But I don't know what's the real use case.
 * Params : The parameter structure to return
 * cnt    : How many parameters
 * return if return 0 (success) 
 */
int PsmHal_GetCustomParams_overwrite( PsmHalParam_t **params, int *cnt)
{
    PsmHalParam_t * tmp = (PsmHalParam_t *)AnscAllocateMemory(sizeof(PsmHalParam_t));
    if(tmp != NULL ){
        strcpy(tmp->name, "test.customoverwrite.name2");
        strcpy(tmp->value, "test.customoverwrite.value2");

        CcspTraceWarning(("%s: overwriteading \n", __FUNCTION__));

        *params = tmp;
        *cnt = 1;
    }else{
        *params = NULL;
        *cnt = 0;
    }

    return 0;
}


