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

    module:	psm_properties.h

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This file defines the configuration parameters that can be
        applied to different Psm objects.

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


#ifndef  _PSM_PROPERTIES_
#define  _PSM_PROPERTIES_


/***********************************************************
         PSM SYS REGISTRY OBJECT PROPERTY DEFINITION
***********************************************************/

/*
 * The Hmsd device should be able to backup all configuration settings, and then restore this
 * configuration at a later date. The restore operation should be done without forcing the user to
 * restart the device at that time. The administrator must be offered the option to restart the
 * device or to restart the device later.
 */
#define  PSM_SYS_FILE_PATH_SIZE                    128
#define  PSM_SYS_FILE_NAME_SIZE                    128


    #define  PSM_DEF_SYS_FILE_PATH                     "/psm/config/"
    #define  PSM_DEF_DEF_FILE_NAME                     "psm_def_cfg.xml.gz"
    #define  PSM_DEF_CUR_FILE_NAME                     "psm_cur_cfg.xml.gz"
    #define  PSM_DEF_BAK_FILE_NAME                     "psm_bak_cfg.xml.gz"
    #define  PSM_DEF_TMP_FILE_NAME                     "psm_tmp_cfg.xml.gz"

    #define  PSM_DEF_LOG_FILE_PATH                     "/var/log/psm-local.log"


typedef  struct
_PSM_SYS_REGISTRY_PROPERTY
{
    char                            SysFilePath[PSM_SYS_FILE_PATH_SIZE];
    char                            DefFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            CurFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            BakFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            TmpFileName[PSM_SYS_FILE_NAME_SIZE];
}
PSM_SYS_REGISTRY_PROPERTY,  *PPSM_SYS_REGISTRY_PROPERTY;

typedef  struct  _PSM_SYS_REGISTRY_PROPERTY  PSM_SYSRO_SIMPLE_PROPERTY,  *PPSM_SYSRO_SIMPLE_PROPERTY;


/***********************************************************
         PSM FILE LOADER OBJECT PROPERTY DEFINITION
***********************************************************/

/*
 * All Hmsd devices will be solid state. They will use a flash file system to store configuration
 * settings persistently. The boot loader will be the UBOOT boot loader for Power-PC. For the Pike
 * and Shark devices the switch subsystem will boot from its own flash. However, the switch agent
 * will not offer switching service on its external ports until the firewall stack is configured
 * and running and all policies configured appropriately. The firewall agent will then enable the
 * switch ports through SNMP requests to the switch agent.
 */
typedef  struct
_PSM_FILE_LOADER_PROPERTY
{
    ULONG                           Dummy;
}
PSM_FILE_LOADER_PROPERTY,  *PPSM_FILE_LOADER_PROPERTY;


#endif
