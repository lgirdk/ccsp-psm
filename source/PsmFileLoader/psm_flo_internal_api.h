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

    module: psm_flo_internal_api.h

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This header file contains the prototype definition for all
        the internal functions provided by the Psm File Loader 
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


#ifndef  _PSM_FLO_INTERNAL_API_
#define  _PSM_FLO_INTERNAL_API_


/***********************************************************
         FUNCTIONS IMPLEMENTED IN PSM_FLO_STATES.C
***********************************************************/

ANSC_HANDLE
PsmFloGetPsmCfmIf
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmFloSetPsmCfmIf
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hInterface
    );

ANSC_HANDLE
PsmFloGetSysIraIf
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmFloSetSysIraIf
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hInterface
    );


ANSC_STATUS
PsmFloGetProperty
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

ANSC_STATUS
PsmFloSetProperty
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

ANSC_STATUS
PsmFloResetProperty
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmFloReset
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
       FUNCTIONS IMPLEMENTED IN PSM_FLO_OPERATION.C
***********************************************************/

ANSC_STATUS
PsmFloEngage
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmFloCancel
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
        FUNCTIONS IMPLEMENTED IN PSM_FLO_CONTROL.C
***********************************************************/

ULONG
PsmFloTestRegFile
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    );

ANSC_STATUS
PsmFloLoadRegFile
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
PsmFloSaveRegFile
    (
        ANSC_HANDLE                 hThisObject
    );


/***********************************************************
        FUNCTIONS IMPLEMENTED IN PSM_FLO_PARSER.C
***********************************************************/

ANSC_STATUS
PsmSysFolderToXMLHandle
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ANSC_HANDLE                 hXMLHandle
    );

ANSC_STATUS
PsmSysFolderFromXMLHandle
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ANSC_HANDLE                 hXMLHandle
    );


#endif
