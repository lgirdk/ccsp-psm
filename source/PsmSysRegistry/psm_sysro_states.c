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

    module:	psm_sysro_states.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the advanced state-access functions
        of the Psm Sys Registry Object.

        *   PsmSysroGetPsmSseIf
        *   PsmSysroSetPsmSseIf
        *   PsmSysroGetPsmFileLoader
        *   PsmSysroGetSysInfoRepository
        *   PsmSysroGetProperty
        *   PsmSysroSetProperty
        *   PsmSysroResetProperty
        *   PsmSysroReset

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
#include "safec_lib_common.h"


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        PsmSysroGetPsmSseIf
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to retrieve object state.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     object state.

**********************************************************************/

ANSC_HANDLE
PsmSysroGetPsmSseIf
    (
        ANSC_HANDLE                 hThisObject
    )
{
//  CcspTraceInfo(("\n##PsmSysroGetPsmSseIf() BEGINs##\n"));
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
//    CcspTraceInfo(("\n##PsmSysroGetPsmSseIf() ends##\n"));
    return  pMyObject->hPsmSseIf;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroSetPsmSseIf
            (
                ANSC_HANDLE                 hThisObject,
                ANSC_HANDLE                 hInterface
            );

    description:

        This function is called to configure object state.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                ANSC_HANDLE                 hInterface
                Specifies the object state to be configured.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroSetPsmSseIf
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hInterface
    )
{
//    CcspTraceInfo(("\n##PsmSysroSetPsmSseIf() begins##\n"));
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;

    pMyObject->hPsmSseIf = hInterface;
//    CcspTraceInfo(("\n##PsmSysroSetPsmSseIf() ends##\n"));
    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        PsmSysroGetPsmFileLoader
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to retrieve object state.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     object state.

**********************************************************************/

ANSC_HANDLE
PsmSysroGetPsmFileLoader
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
//    CcspTraceInfo(("\n##PsmSysroGetPsmFileLoader() ends##\n"));
    return  pMyObject->hPsmFileLoader;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        PsmSysroGetSysInfoRepository
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to retrieve object state.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     object state.

**********************************************************************/

ANSC_HANDLE
PsmSysroGetSysInfoRepository
    (
        ANSC_HANDLE                 hThisObject
    )
{
//	    CcspTraceInfo(("\n##PsmSysroGetSysInfoRepository() begins##\n"));
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
//    CcspTraceInfo(("\n##PsmSysroGetSysInfoRepository() ends##\n"));
    return  pMyObject->hSysInfoRepository;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroGetProperty
            (
                ANSC_HANDLE                 hThisObject,
                ANSC_HANDLE                 hProperty
            );

    description:

        This function is called to retrieve object property.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                ANSC_HANDLE                 hProperty
                Specifies the property data structure to be filled.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroGetProperty
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    )
{
//    CcspTraceInfo(("\n##PsmSysroGetProperty() begins##\n"));
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty    = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;

    *(PPSM_SYS_REGISTRY_PROPERTY)hProperty = *pProperty;
 //   CcspTraceInfo(("\n##PsmSysroGetProperty() ends##\n"));
    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroSetProperty
            (
                ANSC_HANDLE                 hThisObject,
                ANSC_HANDLE                 hProperty
            );

    description:

        This function is called to configure object property.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                ANSC_HANDLE                 hProperty
                Specifies the property data structure to be copied.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroSetProperty
    (
        ANSC_HANDLE                 hThisObject,
        ANSC_HANDLE                 hProperty
    )
{
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty    = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
//    CcspTraceInfo(("\n##PsmSysroSetProperty() begins##\n"));
    *pProperty = *(PPSM_SYS_REGISTRY_PROPERTY)hProperty;
	errno_t rc = -1;

    if ( AnscSizeOfString(pProperty->BakFileName) == 0 )
    {
	rc = strcpy_s(pProperty->BakFileName, sizeof(pProperty->BakFileName), PSM_DEF_BAK_FILE_NAME);
	if(rc != EOK)
	{
	    ERR_CHK(rc);
	    return ANSC_STATUS_FAILURE;
	}
    }

    if ( AnscSizeOfString(pProperty->TmpFileName) == 0 )
    {
	rc = strcpy_s(pProperty->TmpFileName, sizeof(pProperty->TmpFileName), PSM_DEF_TMP_FILE_NAME);
	if(rc != EOK)
	{
	    ERR_CHK(rc);
	    return ANSC_STATUS_FAILURE;
	}
    }
//    CcspTraceInfo(("\n##PsmSysroSetProperty() ends##\n"));
    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroResetProperty
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to reset object property.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroResetProperty
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PPSM_SYS_REGISTRY_OBJECT       pMyObject    = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty    = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
	errno_t                        rc = -1;
//    CcspTraceInfo(("\n##PsmSysroResetProperty() begins##\n"));
    AnscZeroMemory(pProperty, sizeof(PSM_SYS_REGISTRY_PROPERTY));

	rc = strcpy_s(pProperty->SysFilePath, sizeof(pProperty->SysFilePath), PSM_DEF_SYS_FILE_PATH);
	if(rc != EOK)
	{
		ERR_CHK(rc);
		return ANSC_STATUS_FAILURE;
	}
	rc = strcpy_s(pProperty->DefFileName, sizeof(pProperty->DefFileName), PSM_DEF_DEF_FILE_NAME);
	if(rc != EOK)
	{
		ERR_CHK(rc);
		return ANSC_STATUS_FAILURE;
	}
	rc = strcpy_s(pProperty->CurFileName, sizeof(pProperty->CurFileName), PSM_DEF_CUR_FILE_NAME);
	if(rc != EOK)
	{
		ERR_CHK(rc);
		return ANSC_STATUS_FAILURE;
	}
	rc = strcpy_s(pProperty->BakFileName, sizeof(pProperty->BakFileName), PSM_DEF_BAK_FILE_NAME);
	if(rc != EOK)
	{
		ERR_CHK(rc);
		return ANSC_STATUS_FAILURE;
	}
	rc = strcpy_s(pProperty->TmpFileName, sizeof(pProperty->TmpFileName), PSM_DEF_TMP_FILE_NAME);
	if(rc != EOK)
	{
		ERR_CHK(rc);
		return ANSC_STATUS_FAILURE;
	}
//    CcspTraceInfo(("\n##PsmSysroResetProperty() ends##\n"));
    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroReset
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to reset object states.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroReset
    (
        ANSC_HANDLE                 hThisObject
    )
{
    UNREFERENCED_PARAMETER(hThisObject);
//   CcspTraceInfo(("\n##PsmSysroReset() begins##\n"));
//    CcspTraceInfo(("\n##PsmSysroReset() ends##\n"));
    return  ANSC_STATUS_SUCCESS;
}
