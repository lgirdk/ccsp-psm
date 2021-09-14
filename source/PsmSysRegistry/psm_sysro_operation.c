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

    module:	psm_sysro_operation.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the advanced operation functions
        of the Psm Sys Registry Object.

        *   PsmSysroEngage
        *   PsmSysroCancel
        *   PsmSysroRegTimerInvoke

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
        PsmSysroEngage
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to engage the object activity.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroEngage
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus           = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject              = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader        = (PPSM_FILE_LOADER_OBJECT     )pMyObject->hPsmFileLoader;
    PSYS_RAM_INTERFACE              pSysRamIf              = (PSYS_RAM_INTERFACE           )pMyObject->hSysRamIf;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository     = (PSYS_INFO_REPOSITORY_OBJECT  )pMyObject->hSysInfoRepository;
    PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj           = (PANSC_TIMER_DESCRIPTOR_OBJECT)pMyObject->hRegTimerObj;
    PSYS_IRA_INTERFACE              pSysIraIf              = (PSYS_IRA_INTERFACE           )pSysInfoRepository->GetIraIf    ((ANSC_HANDLE)pSysInfoRepository);

//    CcspTraceInfo(("\n##PsmSysRegistry.Engage() begins##\n"));

    if ( pMyObject->bActive )
    {
        return  ANSC_STATUS_SUCCESS;
    }
    else
    {
        pMyObject->bActive = TRUE;
    }

    if ( TRUE )
    {
        pRegTimerObj->Start((ANSC_HANDLE)pRegTimerObj);
    }

    if ( TRUE )
    {
        pSysInfoRepository->IraSetSysRamIf((ANSC_HANDLE)pSysInfoRepository, (ANSC_HANDLE)pSysRamIf);
        pSysInfoRepository->Engage((ANSC_HANDLE)pSysInfoRepository);
    }

    returnStatus = pMyObject->SetupEnv((ANSC_HANDLE)pMyObject);

    /*
     * The Psm device should be able to backup all configuration settings, and then restore this
     * configuration at a later date. The restore operation should be done without forcing the user
     * to restart the device at that time. The administrator must be offered the option to restart
     * the device or to restart the device later. The backup/restore feature should be available
     * via the Web Interface, SNMP, and CLI.
     */
    if ( TRUE )
    {
        pPsmFileLoader->SetPsmCfmIf((ANSC_HANDLE)pPsmFileLoader, pMyObject->hPsmCfmIf  ); 
        pPsmFileLoader->SetSysIraIf((ANSC_HANDLE)pPsmFileLoader, (ANSC_HANDLE)pSysIraIf);
        pPsmFileLoader->Engage     ((ANSC_HANDLE)pPsmFileLoader);
        pPsmFileLoader->LoadRegFile((ANSC_HANDLE)pPsmFileLoader);
        pMyObject->bNoSave = FALSE;
    }

    returnStatus = ANSC_STATUS_SUCCESS;

//    CcspTraceInfo(("\n##PsmSysRegistry.Engage() ends##\n"));

    goto  EXIT1;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT1:

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroCancel
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to cancel the object activity.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroCancel
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader    = (PPSM_FILE_LOADER_OBJECT     )pMyObject->hPsmFileLoader;
    PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj       = (PANSC_TIMER_DESCRIPTOR_OBJECT)pMyObject->hRegTimerObj;

    if ( !pMyObject->bActive )
    {
        return  ANSC_STATUS_SUCCESS;
    }
    else
    {
        pMyObject->bActive = FALSE;
    }

    /*
     * The Psm device should be able to backup all configuration settings, and then restore this
     * configuration at a later date. The restore operation should be done without forcing the user
     * to restart the device at that time. The administrator must be offered the option to restart
     * the device or to restart the device later. The backup/restore feature should be available
     * via the Web Interface, SNMP, and CLI.
     */
    if ( TRUE )
    {
        pPsmFileLoader->Cancel((ANSC_HANDLE)pPsmFileLoader);
    }

    if ( TRUE )
    {
        pRegTimerObj->Stop((ANSC_HANDLE)pRegTimerObj);
    }

    returnStatus = pMyObject->CloseEnv((ANSC_HANDLE)pMyObject);

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroRegTimerInvoke
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called when the registry timer is fired.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroRegTimerInvoke
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader    = (PPSM_FILE_LOADER_OBJECT   )pMyObject->hPsmFileLoader;
    PSYS_IRA_INTERFACE              pSysIraIf          = (PSYS_IRA_INTERFACE         )pPsmFileLoader->hSysIraIf;

    if ( !pMyObject->bNeedFlush )
    {
        return  ANSC_STATUS_SUCCESS;
    }
    else if ( (AnscGetTickInSeconds() - pMyObject->LastRegWriteAt) < PSM_SYSRO_REG_FLUSH_DELAY )
    {
        return  ANSC_STATUS_SUCCESS;
    }

//    CcspTraceInfo(("\n##PsmSysRegistry.RegTimerInvoke() begins##\n"));

    AnscAcquireLock(&pMyObject->AccessLock);
    pSysIraIf->AcqThreadLock(pSysIraIf->hOwnerContext);

    if ( pMyObject->FileSyncRefCount > 0 )
    {
        returnStatus = ANSC_STATUS_SUCCESS;

        goto  EXIT1;
    }

    pMyObject->bSaveInProgress = TRUE;

    returnStatus = pPsmFileLoader->SaveRegFile((ANSC_HANDLE)pPsmFileLoader);

    pMyObject->bNeedFlush      = FALSE;
    pMyObject->bSaveInProgress = FALSE;
    pMyObject->LastRegFlushAt  = AnscGetTickInSeconds();


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT1:

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
    AnscReleaseLock(&pMyObject->AccessLock);

//    CcspTraceInfo(("\n##PsmSysRegistry.RegTimerInvoke() ends##\n"));

    return  returnStatus;
}
