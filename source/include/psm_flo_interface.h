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

    module:	psm_flo_interface.h

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This wrapper file defines the base class data structure and
        interface for the Psm File Loader Objects.

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


#ifndef  _PSM_FLO_INTERFACE_
#define  _PSM_FLO_INTERFACE_


/*
 * This object is derived a virtual base object defined by the underlying framework. We include the
 * interface header files of the base object here to shield other objects from knowing the derived
 * relationship between this object and its base class.
 */
#include "ansc_co_interface.h"
#include "ansc_co_external_api.h"
#include "psm_properties.h"


/***********************************************************
        PSM FILE LOADER COMPONENT OBJECT DEFINITION
***********************************************************/

/*
 * Define some const values that will be used in the os wrapper object definition.
 */
#define  PSM_FLO_ERROR_CODE_noError                0
#define  PSM_FLO_ERROR_CODE_invalidFormat          1
#define  PSM_FLO_ERROR_CODE_wrongChecksum          2
#define  PSM_FLO_ERROR_CODE_wrongDevice            3
#define  PSM_FLO_ERROR_CODE_other                  4

/*
 * Since we write all kernel modules in C (due to better performance and lack of compiler support), we
 * have to simulate the C++ object by encapsulating a set of functions inside a data structure.
 */
typedef  ANSC_HANDLE
(*PFN_PSMFLO_GET_CONTEXT)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_SET_CONTEXT)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hContext
    );

typedef  ANSC_HANDLE
(*PFN_PSMFLO_GET_IF)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_SET_IF)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hInterface
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_GET_PROPERTY)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_SET_PROPERTY)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_RESET)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_ENGAGE)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_CANCEL)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ULONG
(*PFN_PSMFLO_TEST)
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_LOAD)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMFLO_SAVE)
    (
        ANSC_HANDLE                 hThisObject
    );

/*
 * The Psm device should be able to backup all configuration settings, and then restore this
 * configuration at a later date. The restore operation should be done without forcing the user to
 * restart the device at that time. The administrator must be offered the option to restart the
 * device or to restart the device later. The backup/restore feature should be available via the
 * web interface and SNMP.
 */
#define  PSM_FILE_LOADER_CLASS_CONTENT                                                    \
    /* duplication of the base object class content */                                    \
    ANSCCO_CLASS_CONTENT                                                                  \
    /* start of object class content */                                                   \
    PSM_FILE_LOADER_PROPERTY       Property;                                              \
    ANSC_HANDLE                    hPsmCfmIf;                                             \
    ANSC_HANDLE                    hSysIraIf;                                             \
    BOOL                           bActive;                                               \
                                                                                          \
    PFN_PSMFLO_GET_IF              GetPsmCfmIf;                                           \
    PFN_PSMFLO_SET_IF              SetPsmCfmIf;                                           \
    PFN_PSMFLO_GET_IF              GetSysIraIf;                                           \
    PFN_PSMFLO_SET_IF              SetSysIraIf;                                           \
    PFN_PSMFLO_GET_PROPERTY        GetProperty;                                           \
    PFN_PSMFLO_SET_PROPERTY        SetProperty;                                           \
    PFN_PSMFLO_RESET               ResetProperty;                                         \
    PFN_PSMFLO_RESET               Reset;                                                 \
                                                                                          \
    PFN_PSMFLO_ENGAGE              Engage;                                                \
    PFN_PSMFLO_CANCEL              Cancel;                                                \
                                                                                          \
    PFN_PSMFLO_TEST                TestRegFile;                                           \
    PFN_PSMFLO_LOAD                LoadRegFile;                                           \
    PFN_PSMFLO_SAVE                SaveRegFile;                                           \
    /* end of object class content */                                                     \

typedef  struct
_PSM_FILE_LOADER_OBJECT
{
    PSM_FILE_LOADER_CLASS_CONTENT
}
PSM_FILE_LOADER_OBJECT,  *PPSM_FILE_LOADER_OBJECT;

#define  ACCESS_PSM_FILE_LOADER_OBJECT(p)          \
         ACCESS_CONTAINER(p, PSM_FILE_LOADER_OBJECT, Linkage)


#endif
