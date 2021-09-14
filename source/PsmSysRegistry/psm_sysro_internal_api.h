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

    module:	psm_sysro_internal_api.h

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This header file contains the prototype definition for all
        the internal functions provided by the Psm Sys Registry
        Object.

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


#ifndef  _PSM_SYSRO_INTERNAL_API_
#define  _PSM_SYSRO_INTERNAL_API_


/***********************************************************
         FUNCTIONS IMPLEMENTED IN PSM_SYSRO_STATES.C
***********************************************************/

ANSC_HANDLE
PsmSysroGetPsmSseIf
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroSetPsmSseIf
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hInterface
    );

ANSC_HANDLE
PsmSysroGetPsmFileLoader
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_HANDLE
PsmSysroGetSysInfoRepository
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroGetProperty
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

ANSC_STATUS
PsmSysroSetProperty
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

ANSC_STATUS
PsmSysroResetProperty
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
       FUNCTIONS IMPLEMENTED IN PSM_SYSRO_OPERATION.C
***********************************************************/

ANSC_STATUS
PsmSysroEngage
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroCancel
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroRegTimerInvoke
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
        FUNCTIONS IMPLEMENTED IN PSM_SYSRO_CONTROL.C
***********************************************************/

ANSC_STATUS
PsmSysroSetupEnv
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroCloseEnv
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
        FUNCTIONS IMPLEMENTED IN PSM_SYSRO_STORAGE.C
***********************************************************/

ANSC_STATUS
PsmSysroResetToFactoryDefault
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroImportConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize,
        void*                       pDecryptKey,
        ULONG                       ulKeySize
    );

ANSC_STATUS
PsmSysroExportConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        PULONG                      pulCfgSize,
        void*                       pEncryptKey,
        ULONG                       ulKeySize
    );

ULONG
PsmSysroGetConfigSize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmSysroSaveConfigToFlash
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
         FUNCTIONS IMPLEMENTED IN PSM_SYSRO_CFMIF.C
***********************************************************/

ANSC_STATUS
PsmSysroCfmReadCurConfig
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    );

ANSC_STATUS
PsmSysroCfmReadDefConfig
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    );

ANSC_STATUS
PsmSysroCfmSaveCurConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    );


/***********************************************************
       FUNCTIONS IMPLEMENTED IN PSM_SYSRO_SYSRAMIF.C
***********************************************************/

ANSC_STATUS
PsmSysroSysRamEnableFileSync
    (
        ANSC_HANDLE                 hThisObject,
        BOOL                        bEnabled
    );

ANSC_STATUS
PsmSysroSysRamNotify
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hSysRepFolder,
        ULONG                       ulEvent
    );


#endif
