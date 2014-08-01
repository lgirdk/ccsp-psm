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

    module:	psm_sysro_sysramif.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

                    Copyright (c) 2011, Cisco Systems, Inc.

                            CISCO CONFIDENTIAL
              Unauthorized distribution or copying is prohibited
                            All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Cisco Systems, Inc.

    ---------------------------------------------------------------

    description:

        This module implements the advanced interface functions
        of the Psm Sys Registry Object.

        *   PsmSysroSysRamEnableFileSync
        *   PsmSysroSysRamNotify

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
        PsmSysroSysRamEnableFileSync
            (
                ANSC_HANDLE                 hThisObject,
                BOOL                        bEnabled
            );

    description:

        This function is called to enable/disable file synchronization.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                BOOL                        bEnabled
                Specifies whether the file sync should be enabled.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroSysRamEnableFileSync
    (
        ANSC_HANDLE                 hThisObject,
        BOOL                        bEnabled
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository = (PSYS_INFO_REPOSITORY_OBJECT)pMyObject->hSysInfoRepository;
    PSYS_IRA_INTERFACE              pIraIf             = (PSYS_IRA_INTERFACE         )pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);

    AnscAcquireLock(&pMyObject->AccessLock);

    if ( bEnabled )
    {
        pMyObject->FileSyncRefCount--;
    }
    else
    {
        pMyObject->FileSyncRefCount++;
    }

    AnscReleaseLock(&pMyObject->AccessLock);

    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroSysRamNotify
            (
                ANSC_HANDLE                 hThisObject,
                ANSC_HANDLE                 hSysRepFolder,
                ULONG                       ulEvent
            );

    description:

        This function is called whenever a repository folder is updated.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                ANSC_HANDLE                 hSysRepFolder
                Specifies the current repository is reporting the event.

                ULONG                       ulEvent
                Specifies the update event to be processed.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroSysRamNotify
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hSysRepFolder,
        ULONG                       ulEvent
    )
{
    ANSC_STATUS                     returnStatus    = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject       = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty       = (PPSM_SYS_REGISTRY_PROPERTY  )&pMyObject->Property;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader = (PPSM_FILE_LOADER_OBJECT     )pMyObject->hPsmFileLoader;
    PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj    = (PANSC_TIMER_DESCRIPTOR_OBJECT)pMyObject->hRegTimerObj;

    if ( pMyObject->bNoSave || pMyObject->bSaveInProgress )
    {
        return  ANSC_STATUS_SUCCESS;
    }

    switch ( ulEvent )
    {
        case    SYS_RAM_EVENT_folderAdded   :
        case    SYS_RAM_EVENT_folderUpdated :
        case    SYS_RAM_EVENT_folderDeleted :
        case    SYS_RAM_EVENT_folderCleared :
        case    SYS_RAM_EVENT_recordAdded   :
        case    SYS_RAM_EVENT_recordUpdated :
        case    SYS_RAM_EVENT_recordDeleted :

                /*
                pRegTimerObj->Stop ((ANSC_HANDLE)pRegTimerObj);
                pRegTimerObj->Start((ANSC_HANDLE)pRegTimerObj);
                */
                pMyObject->LastRegWriteAt = AnscGetTickInSeconds();
                pMyObject->bNeedFlush     = TRUE;

                break;

        default :

                break;
    }

    return  returnStatus;
}
