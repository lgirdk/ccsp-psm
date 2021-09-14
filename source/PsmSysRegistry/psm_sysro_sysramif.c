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

    module:	psm_sysro_sysramif.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

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
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    AnscAcquireLock(&pMyObject->AccessLock);
//CcspTraceInfo(("\n##PsmSysroSysRamEnableFileSync() begins##\n"));
    if ( bEnabled )
    {
        pMyObject->FileSyncRefCount--;
    }
    else
    {
        pMyObject->FileSyncRefCount++;
    }

    AnscReleaseLock(&pMyObject->AccessLock);
//CcspTraceInfo(("\n##PsmSysroSysRamEnableFileSync() ENDs##\n"));
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
    UNREFERENCED_PARAMETER(hSysRepFolder);
    ANSC_STATUS                     returnStatus    = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject       = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
   // PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj    = (PANSC_TIMER_DESCRIPTOR_OBJECT)pMyObject->hRegTimerObj;
//CcspTraceInfo(("\n##PsmSysroSysRamNotify() begins##\n"));
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
//CcspTraceInfo(("\n##PsmSysroSysRamNotify() ends##\n"));
    return  returnStatus;
}
