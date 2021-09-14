/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

    module:	psm_sysro_control.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the advanced env-control functions
        of the Psm Sys Registry Object.

        *   PsmSysroSetupEnv
        *   PsmSysroCloseEnv

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Jian Wu

    ---------------------------------------------------------------

    revision:

        06/17/11    initial revision.

**********************************************************************/


#include "psm_sysro_global.h"


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroSetupEnv
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to set up the runtime environment.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroSetupEnv
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository = (PSYS_INFO_REPOSITORY_OBJECT)pMyObject->hSysInfoRepository;
    PSYS_IRA_INTERFACE              pIraIf             = (PSYS_IRA_INTERFACE         )pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);
    ANSC_HANDLE                     hRfoKeyCfg         = (ANSC_HANDLE                )NULL;
    ANSC_HANDLE                     hRfoKeyCfgPrv      = (ANSC_HANDLE                )NULL;
//CcspTraceInfo(("\n##PsmSysroSetupEnv() beginss##\n"));
    if ( !pMyObject->bActive )
    {
        returnStatus = ANSC_STATUS_NOT_READY;

        goto  EXIT1;
    }

    /*
     * First thing first, we create all the system-level folders in System Information Repository
     * before engaging in any other activities since all other operations utilize the system-level
     * folders either directly or implicitly.
     */
    hRfoKeyCfg = pIraIf->AddSysFolder(pIraIf->hOwnerContext, SYS_FOLDER_L1_CONFIGURATION      );

    if ( !hRfoKeyCfg )
    {
        returnStatus = ANSC_STATUS_INTERNAL_ERROR;

        goto  EXIT2;
    }

    hRfoKeyCfgPrv    = pIraIf->AddFolder(pIraIf->hOwnerContext, hRfoKeyCfg, SYS_FOLDER_L2_PROVISION       );

    if ( !hRfoKeyCfgPrv )
    {
        returnStatus = ANSC_STATUS_INTERNAL_ERROR;
    }
    else
    {
        returnStatus = ANSC_STATUS_SUCCESS;
    }

    goto  EXIT3;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT3:

    pIraIf->CloseFolder(pIraIf->hOwnerContext, hRfoKeyCfgPrv   );

EXIT2:

    pIraIf->CloseFolder(pIraIf->hOwnerContext, hRfoKeyCfg);

EXIT1:
//CcspTraceInfo(("\n##PsmSysroSetupEnv() ends##\n"));
    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroCloseEnv
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to close the runtime environment.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroCloseEnv
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
//CcspTraceInfo(("\n##PsmSysroCloseEnv() begins##\n"));
    if ( !pMyObject->bActive )
    {
        returnStatus = ANSC_STATUS_NOT_READY;

        goto  EXIT1;
    }


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT1:
//CcspTraceInfo(("\n##PsmSysroCloseEnv() ends##\n"));
    return  returnStatus;
}
