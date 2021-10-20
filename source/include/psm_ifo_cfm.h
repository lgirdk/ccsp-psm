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

    module:	psm_ifo_cfm.h

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This wrapper file defines the base class data structure and
        interface for the Configuration File Manager Objects.

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
#include "ansc_ifo_interface.h"

#ifndef  _PSM_IFO_CFM_
#define  _PSM_IFO_CFM_


/***********************************************************
    PSM CONFIGURATION FILE MANAGER INTERFACE DEFINITION
***********************************************************/

/*
 * Define some const values that will be used in the os wrapper object definition.
 */
#define  PSM_CFM_INTERFACE_NAME                    "psmConfigurationFileManagerIf"
#define  PSM_CFM_INTERFACE_ID                      0

/*
 * Since we write all kernel modules in C (due to better performance and lack of compiler support), we
 * have to simulate the C++ object by encapsulating a set of functions inside a data structure.
 */
typedef  ANSC_STATUS
(*PFN_PSMCFMIF_READ_CFG)
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    );

typedef  ANSC_STATUS
(*PFN_PSMCFMIF_SAVE_CFG)
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    );

typedef  ANSC_STATUS
(*PFN_PSMCFMIF_UPDATE_CFG)
    (
        ANSC_HANDLE                 hThisObject,
        const char                  *newCfgPath
    );


/*
 * As a general requirement, each module SHOULD provide an interface for external components to
 * configure its parameters and policies. Although the benefit of unifying the definition and usage
 * of such an interface is obvious, we DON'T want to impose any set of rules on the implementation.
 * Instead, we expect every module will implement its configuration interfaces independently.
 */
#define  PSM_CFM_INTERFACE_CLASS_CONTENT                                                   \
    /* duplication of the base object class content */                                      \
    ANSCIFO_CLASS_CONTENT                                                                   \
    /* start of object class content */                                                     \
    PFN_PSMCFMIF_READ_CFG          ReadCurConfig;                                          \
    PFN_PSMCFMIF_READ_CFG          ReadDefConfig;                                          \
    PFN_PSMCFMIF_SAVE_CFG          SaveCurConfig;                                          \
    PFN_PSMCFMIF_UPDATE_CFG        UpdateConfigs;                                          \
    /* end of object class content */                                                       \

typedef  struct
_PSM_CFM_INTERFACE
{
    PSM_CFM_INTERFACE_CLASS_CONTENT
}
PSM_CFM_INTERFACE,  *PPSM_CFM_INTERFACE;

#define  ACCESS_PSM_CFM_INTERFACE(p)               \
         ACCESS_CONTAINER(p, PSM_CFM_INTERFACE, Linkage)


int backup_file (const char *bkupFile, const char *localFile);
#endif
