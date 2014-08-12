/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**********************************************************************

    module: psm_hal_apis.c

        For CCSP Persistent Storage Manager component

    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved. 2012~2014

    ---------------------------------------------------------------

    description:

        This file defines Persistent Storage Manager component 
        specific HAL apis.

    ---------------------------------------------------------------

    environment:

        platform dependent

    ---------------------------------------------------------------

    author:

        Ding Hua

    ---------------------------------------------------------------

    revision:

        04/16/2014    initial revision.

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "psm_hal_apis.h"

/**********************************************************************
                            HAL API IMPLEMENTATION
**********************************************************************/

/*
 *
    Description:
        This HAL API returns the platform specific persistent configuration
        parameters.

        Memory block of returned params is allocated inside of the routine
        using malloc(). Caller is responsible to call free() to free the
        memory after use.

    Arguments:
        PsmHalParam_t **            params,
        Returns the retrieved configuration parameters. Memory block
        is allocated in the routine and caller is responsible to free it.

        int *                       cnt
        Returns the number of parameters.

    Returns:
        0       - success
        -1      - failure or no custom parameters
 *
 */
int PsmHal_GetCustomParams( PsmHalParam_t **params, int *cnt)
{
    return  -1;
}


/*
 *
    Description:
        After PSM reset to factory defaults, this HAL API gives the 
        underlying layer an opportunity to restore extra factory defaults 

    Arguments:

    Returns:
        0       - success
        -1      - failure
 *
 */
int
PsmHal_RestoreFactoryDefaults
    (
        void
    )
{
    return  0;
}
