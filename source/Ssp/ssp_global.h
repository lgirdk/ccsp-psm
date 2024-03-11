#include "ansc_platform.h"

#include "ccsp_custom.h"

/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "ccsp_message_bus.h"
#include "ccsp_base_api.h"

//#include "psm_platform_plugin.h"

/*
 *  Define custom trace module ID
 */
#ifdef   ANSC_TRACE_MODULE_ID
    #undef  ANSC_TRACE_MODULE_ID
#endif

#define  ANSC_TRACE_MODULE_ID                       ANSC_TRACE_ID_SSP

#define  CCSP_COMPONENT_VERSION_PSM                 1
#define  CCSP_PSM_FORMAT_VERSION                    1
#define  CCSP_COMPONENT_AUTHOR_PSM                  "Yan Li"

#define PSM_SUPPORT_PRECONFIGURED_NEXTLEVEL          "dmsb.psm.feature.preconfiguredgetnextlevel"
#define PSM_SUPPORT_PRECONFIGURED_NEXTLEVEL_PREFIX   "dmsb.nextlevel."

#define PSM_SUPPORT_UPGRADE_FROM_NON_DATABASEVERSIONG "dmsb.psm.feature.upgradefromnondatabaseversioning"
#define PSM_SUPPORT_DOWNGRADE_TO_NON_DATABASEVERSIONG "dmsb.psm.feature.downgradetonondatabaseversioning"

extern char     bPsmSupportUpgradeFromNonDatabaseVersionging;
extern char     bPsmSupportDowngradeToNonDatabaseVersionging;

extern char    *pPsmDefFileBuffer;
extern char    *pPsmCurFileBuffer;
extern int      PsmDefFileLen;
extern int      PsmCurFileLen;

extern int      bPsmNeedSaving;
extern int      PsmContentChangedTime;

#define  PSM_TIMER_INTERVAL              3           /* 3 seconds */
#define  PSM_FLUSH_DELAY                 3           /* 3 seconds                  */


#ifdef PSM_DEF_XML_CONFIG_FILE_PATH

#undef PSM_DEF_XML_CONFIG_FILE_PATH
#define PSM_DEF_XML_CONFIG_FILE_PATH "/usr/ccsp/"

#endif

#ifndef PSM_NEW_CUR_XML_CONFIG_FILE_NAME
#define  PSM_NEW_CUR_XML_CONFIG_FILE_NAME          "/tmp/bbhm_lite_cur_cfg.xml"
#endif
#ifndef PSM_NEW_BAK_XML_CONFIG_FILE_NAME
#define  PSM_NEW_BAK_XML_CONFIG_FILE_NAME          "/nvram/bbhm_lite_bak_cfg.xml"
#endif
#ifndef PSM_NEW_TMP_XML_CONFIG_FILE_NAME
#define  PSM_NEW_TMP_XML_CONFIG_FILE_NAME          "/nvram/bbhm_lite_tmp_cfg.xml"
#endif

enum CUSTOM_PARAM_LOADING{
    CUSTOM_PARAM_DEFAULT = 1,
    CUSTOM_PARAM_OVERWRITE
};

int  cmd_dispatch(int  command);
int  gather_info ();

int  PsmDbusInit ();

#define  PSM_SYS_FILE_PATH_SIZE                    128
#define  PSM_SYS_FILE_NAME_SIZE                    128

typedef  struct
_PSM_FILE_INFO
{
    char                            SysFilePath[PSM_SYS_FILE_PATH_SIZE];
    char                            DefFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            CurFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            BakFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            TmpFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            NewCurFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            NewBakFileName[PSM_SYS_FILE_NAME_SIZE];
    char                            NewTmpFileName[PSM_SYS_FILE_NAME_SIZE];
}
PSM_FILE_INFO,  *PPSM_FILE_INFO;

extern PSM_FILE_INFO PsmFileinfo;

#define  MAX_NAME_SZ                                128
#define  MAX_VALUE_SZ                               128

typedef struct PsmHalParam_s
{
    char                            name[MAX_NAME_SZ];
    char                            value[MAX_VALUE_SZ];
} PsmHalParam_t;


int PsmHashLoadCustom(enum CUSTOM_PARAM_LOADING type);

