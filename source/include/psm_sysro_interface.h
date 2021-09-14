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

    module:	psm_sysro_interface.h

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This wrapper file defines the base class data structure and
        interface for the Psm Sys Registry Objects.

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


#ifndef  _PSM_SYSRO_INTERFACE_
#define  _PSM_SYSRO_INTERFACE_


/*
 * This object is derived a virtual base object defined by the underlying framework. We include the
 * interface header files of the base object here to shield other objects from knowing the derived
 * relationship between this object and its base class.
 */
#include "ansc_co_interface.h"
#include "ansc_co_external_api.h"
#include "psm_properties.h"

#include "psm_ifo_cfm.h"
#include "sys_ifo_ram.h"


/***********************************************************
       PSM SYS REGISTRY COMPONENT OBJECT DEFINITION
***********************************************************/

/*
 * Define some const values that will be used in the os wrapper object definition.
 */
//#define  PSM_SYSRO_REG_TIMER_INTERVAL              3 * 1000    /* 3 seconds, in milliseconds */
#define  PSM_SYSRO_REG_TIMER_INTERVAL              10 * 1000    /* 3 seconds, in milliseconds */
#define  PSM_SYSRO_REG_FLUSH_DELAY                 3           /* 3 seconds                  */

/*
 * Since we write all kernel modules in C (due to better performance and lack of compiler support), we
 * have to simulate the C++ object by encapsulating a set of functions inside a data structure.
 */
typedef  ANSC_HANDLE
(*PFN_PSMSYSRO_GET_CONTEXT)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_SET_CONTEXT)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hContext
    );

typedef  ANSC_HANDLE
(*PFN_PSMSYSRO_GET_IF)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_SET_IF)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hInterface
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_GET_PROPERTY)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_SET_PROPERTY)
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_RESET)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_ENGAGE)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_CANCEL)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_INVOKE)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_SETUP)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_CLOSE)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_RESET_TO)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_IMPORT)
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize,
        void*                       pDecryptKey,
        ULONG                       ulKeySize
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_EXPORT)
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        PULONG                      pulCfgSize,
        void*                       pEncryptKey,
        ULONG                       ulKeySize
    );

typedef  ULONG
(*PFN_PSMSYSRO_GET_SIZE)
    (
        ANSC_HANDLE                 hThisObject
    );

typedef  ANSC_STATUS
(*PFN_PSMSYSRO_SAVE)
    (
        ANSC_HANDLE                 hThisObject
    );

/*
 * The Psm device should be able to backup all configuration settings, and then restore this
 * configuration at a later date. The restore operation should be done without forcing the user to
 * restart the device at that time. The administrator must be offered the option to restart the
 * device or to restart the device later.
 */
#define  PSM_SYS_REGISTRY_CLASS_CONTENT                                                    \
    /* duplication of the base object class content */                                     \
    ANSCCO_CLASS_CONTENT                                                                   \
    /* start of object class content */                                                    \
    PSM_SYS_REGISTRY_PROPERTY      Property;                                               \
    ANSC_HANDLE                    hPsmSseIf;                                              \
    ANSC_HANDLE                    hPsmCfmIf;                                              \
    ANSC_HANDLE                    hPsmFileLoader;                                         \
    ANSC_HANDLE                    hSysRamIf;                                              \
    ANSC_HANDLE                    hSysInfoRepository;                                     \
                                                                                           \
    ANSC_HANDLE                    hRegTimerObj;                                           \
    ANSC_HANDLE                    hRegTimerIf;                                            \
    ULONG                          LastRegWriteAt;                                         \
    ULONG                          LastRegFlushAt;                                         \
    ULONG                          FileSyncRefCount;                                       \
    BOOL                           bNoSave;                                                \
    BOOL                           bNeedFlush;                                             \
    BOOL                           bSaveInProgress;                                        \
    BOOL                           bProcSeparation;                                        \
    BOOL                           bActive;                                                \
    ANSC_LOCK                      AccessLock;                                             \
                                                                                           \
    PFN_PSMSYSRO_GET_IF            GetPsmSseIf;                                            \
    PFN_PSMSYSRO_SET_IF            SetPsmSseIf;                                            \
    PFN_PSMSYSRO_GET_CONTEXT       GetPsmFileLoader;                                       \
    PFN_PSMSYSRO_GET_CONTEXT       GetSysInfoRepository;                                   \
                                                                                           \
    PFN_PSMSYSRO_GET_PROPERTY      GetProperty;                                            \
    PFN_PSMSYSRO_SET_PROPERTY      SetProperty;                                            \
    PFN_PSMSYSRO_RESET             ResetProperty;                                          \
    PFN_PSMSYSRO_RESET             Reset;                                                  \
                                                                                           \
    PFN_PSMSYSRO_ENGAGE            Engage;                                                 \
    PFN_PSMSYSRO_CANCEL            Cancel;                                                 \
    PFN_PSMSYSRO_INVOKE            RegTimerInvoke;                                         \
                                                                                           \
    PFN_PSMSYSRO_SETUP             SetupEnv;                                               \
    PFN_PSMSYSRO_CLOSE             CloseEnv;                                               \
                                                                                           \
    PFN_PSMSYSRO_RESET_TO          ResetToFactoryDefault;                                  \
    PFN_PSMSYSRO_IMPORT            ImportConfig;                                           \
    PFN_PSMSYSRO_EXPORT            ExportConfig;                                           \
    PFN_PSMSYSRO_GET_SIZE          GetConfigSize;                                          \
    PFN_PSMSYSRO_SAVE              SaveConfigToFlash;                                      \
                                                                                           \
    PFN_PSMCFMIF_READ_CFG          CfmReadCurConfig;                                       \
    PFN_PSMCFMIF_READ_CFG          CfmReadDefConfig;                                       \
    PFN_PSMCFMIF_SAVE_CFG          CfmSaveCurConfig;                                       \
                                                                                           \
    PFN_SYSRAMIF_ENABLE_SYNC       SysRamEnableFileSync;                                   \
    PFN_SYSRAMIF_NOTIFY            SysRamNotify;                                           \
    /* end of object class content */                                                      \

typedef  struct
_PSM_SYS_REGISTRY_OBJECT
{
    PSM_SYS_REGISTRY_CLASS_CONTENT
}
PSM_SYS_REGISTRY_OBJECT,  *PPSM_SYS_REGISTRY_OBJECT;

#define  ACCESS_PSM_SYS_REGISTRY_OBJECT(p)         \
         ACCESS_CONTAINER(p, PSM_SYS_REGISTRY_OBJECT, Linkage)


#endif
