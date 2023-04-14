/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <rbus/rbus.h>
#include <libgen.h>
#include "ssp_global.h"
#include "safec_lib_common.h"

extern  PPSM_SYS_REGISTRY_OBJECT  pPsmSysRegistry;

typedef enum _rbus_legacy_support
{
    RBUS_LEGACY_STRING = 0,    /**< Null terminated string                                           */
    RBUS_LEGACY_INT,           /**< Integer (2147483647 or -2147483648) as String                    */
    RBUS_LEGACY_UNSIGNEDINT,   /**< Unsigned Integer (ex: 4,294,967,295) as String                   */
    RBUS_LEGACY_BOOLEAN,       /**< Boolean as String (ex:"true", "false"                            */
    RBUS_LEGACY_DATETIME,      /**< ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ) as String                 */
    RBUS_LEGACY_BASE64,        /**< Base64 representation of data as String                          */
    RBUS_LEGACY_LONG,          /**< Long (ex: 9223372036854775807 or -9223372036854775808) as String */
    RBUS_LEGACY_UNSIGNEDLONG,  /**< Unsigned Long (ex: 18446744073709551615) as String               */
    RBUS_LEGACY_FLOAT,         /**< Float (ex: 1.2E-38 or 3.4E+38) as String                         */
    RBUS_LEGACY_DOUBLE,        /**< Double (ex: 2.3E-308 or 1.7E+308) as String                      */
    RBUS_LEGACY_BYTE,
    RBUS_LEGACY_NONE
} rbusLegacyDataType_t;

static bool rbusValue_SetParamVal(rbusValue_t value, ULONG contentType, char const* pBuff)
{

    rbusValueType_t type;
    bool rc = false;

    if(RBUS_LEGACY_BYTE == contentType)
    {
        rbusValue_SetBytes(value, (uint8_t*)pBuff, strlen(pBuff));
        rc = true;
    }
    else if( RBUS_LEGACY_BASE64 == contentType)
    {
        CcspTraceWarning(("RBUS_LEGACY_BASE64_TYPE: Base64 type was never used in CCSP so far.\
                    So, Rbus did not support it till now. Since this is the first Base64 query,\
                    please report to get it fixed."));
        rbusValue_SetString(value, pBuff);
        rc = true;
    }
    else
    {
        switch(contentType)
        {
            case RBUS_LEGACY_STRING:        type = RBUS_STRING; break;
            case RBUS_LEGACY_INT:           type = RBUS_INT32; break;
            case RBUS_LEGACY_UNSIGNEDINT:   type = RBUS_UINT32; break;
            case RBUS_LEGACY_BOOLEAN:       type = RBUS_BOOLEAN; break;
            case RBUS_LEGACY_LONG:          type = RBUS_INT64; break;
            case RBUS_LEGACY_UNSIGNEDLONG:  type = RBUS_UINT64; break;
            case RBUS_LEGACY_FLOAT:         type = RBUS_SINGLE; break;
            case RBUS_LEGACY_DOUBLE:        type = RBUS_DOUBLE; break;
            case RBUS_LEGACY_DATETIME:      type = RBUS_DATETIME; break;
            default:                        return rc;
        }
        rc = rbusValue_SetFromString(value, type, pBuff);
    }
    return rc;
}

static rbusLegacyDataType_t getLegacyType(rbusValueType_t type)
{
    switch(type)
    {
        case RBUS_STRING:   return RBUS_LEGACY_STRING;
        case RBUS_INT32:    return RBUS_LEGACY_INT;
        case RBUS_UINT32:   return RBUS_LEGACY_UNSIGNEDINT;
        case RBUS_BOOLEAN:  return RBUS_LEGACY_BOOLEAN;
        case RBUS_INT64:    return RBUS_LEGACY_LONG;
        case RBUS_UINT64:   return RBUS_LEGACY_UNSIGNEDLONG;
        case RBUS_SINGLE:   return RBUS_LEGACY_FLOAT;
        case RBUS_DOUBLE:   return RBUS_LEGACY_DOUBLE;
        case RBUS_DATETIME: return RBUS_LEGACY_DATETIME;
        default:            break;
    }
    return RBUS_LEGACY_NONE;
}

static void setOutparams(rbusObject_t outParams,char *parameterName, bool val)
{
    rbusValue_t value;

    rbusValue_Init(&value);
    rbusValue_SetBoolean(value,val);
    rbusObject_SetValue(outParams, parameterName, value);
    rbusValue_Release(value);
}

static int setParameterValues_rbus(rbusObject_t inParams, rbusObject_t outParams)
{
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    PSYS_RRO_RENDER_ATTR            pRroRenderAttr      = (PSYS_RRO_RENDER_ATTR)NULL;
    SYS_RRO_RENDER_ATTR             rroRenderAttr;
    char    oldValBuf[512] = {0};
    ULONG   ulRecordType, ulRecordSize;
    errno_t rc = -1;
    int ind = -1, ret = RBUS_ERROR_BUS_ERROR;
    rbusValue_t value;
    rbusProperty_t prop;
    rbusValueType_t type;
    rbusLegacyDataType_t legacyType;
    char *parameterName, *parameterValue;

    if ( pPsmSysRegistry == NULL )
    {
        CcspTraceError(("%s - pPsmSysRegistry is NULL\n",__func__));
        return ret;
    }
    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;
    if ( pSysInfoRepository == NULL )
    {
        CcspTraceError(("%s - pSysInfoRepository is NULL\n",__func__));
        return ret;
    }
    pSysIraIf = (PSYS_IRA_INTERFACE)pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);
    pSysIraIf->AcqThreadLock(pSysIraIf->hOwnerContext);
    hSysRoot =
        pSysIraIf->OpenFolder
        (
         pSysIraIf->hOwnerContext,
         (ANSC_HANDLE)NULL,
         "/Configuration/Provision"
        );
    if ( hSysRoot == NULL )
    {
        CcspTraceError(("%s - hSysRoot is NULL\n",__func__));
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        return ret;
    }

    SysInitRroRenderAttr((&rroRenderAttr));
    prop = rbusObject_GetProperties(inParams);
    for( ; (NULL != prop) ; prop = rbusProperty_GetNext(prop))
    {
        parameterName = (char *)rbusProperty_GetName(prop);
        setOutparams(outParams,parameterName,false);
        value = rbusProperty_GetValue(prop);
        type = rbusValue_GetType(value);
        legacyType = getLegacyType(type);

        if(RBUS_LEGACY_NONE == legacyType)
        {
            CcspTraceWarning(("%s Invalid datatype \n",__func__));
            continue;
        }

        if(RBUS_LEGACY_BOOLEAN == legacyType)
        {
            if(true == rbusValue_GetBoolean(value))
                parameterValue = strdup("true");
            else
                parameterValue = strdup("false");
        }
        else
        {
            parameterValue = rbusValue_ToString(value,NULL,0);
        }

        /*RDKB-24884 : PSM set operation with same value should not update bbhm xml*/
        rc =  memset_s(oldValBuf, sizeof(oldValBuf), 0, sizeof(oldValBuf));
        ERR_CHK(rc);
        returnStatus =
            pSysIraIf->GetRecord
            (
             pSysIraIf->hOwnerContext,
             hSysRoot,
             parameterName,
             &ulRecordType,
             (PANSC_HANDLE)&pRroRenderAttr,
             oldValBuf,
             &ulRecordSize
            );

        if (ANSC_STATUS_SUCCESS == returnStatus)
        {
            if( pRroRenderAttr->ContentType != legacyType)
            {
                CcspTraceWarning(("%s %s is already set with type %lu. Invalid type %u is set.\n",
                            __func__, parameterName, pRroRenderAttr->ContentType, legacyType ));
                free(parameterValue);
                continue;
            }

            rc = strcmp_s(oldValBuf, sizeof(oldValBuf), parameterValue, &ind);
            ERR_CHK(rc);
            if((rc == EOK) && (ind == 0))
            {
                CcspTraceWarning(("%s %s is already present with value %s. oldValBuf %s\n",
                            __func__, parameterName, parameterValue,oldValBuf));
                free(parameterValue);
                continue;
            }
        }
        rroRenderAttr.ContentType = legacyType;

        returnStatus =
            pSysIraIf->AddRecord2
            (
             pSysIraIf->hOwnerContext,
             hSysRoot,
             parameterName,
             SYS_RRO_PERMISSION_ALL,
             SYS_REP_RECORD_TYPE_ASTR,
             (ANSC_HANDLE)&rroRenderAttr,
             (PVOID)parameterValue,
             strlen(parameterValue)
            );
        CcspTraceInfo(("%s Add entry:  param : %s , val : %s %s , return %lu +++\n",
                    __func__,((returnStatus != ANSC_STATUS_SUCCESS)? "failed" : "success"),
                    parameterName, parameterValue, returnStatus));
        if(ANSC_STATUS_SUCCESS == returnStatus)
            setOutparams(outParams,parameterName,true);
        free(parameterValue);
    }
    ret = RBUS_ERROR_SUCCESS;
    pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);
    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
    return ret;
}

static int getParameterValues_rbus(rbusObject_t inParams, rbusObject_t outParams)
{
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    PSYS_RRO_RENDER_ATTR            pRroRenderAttr      = (PSYS_RRO_RENDER_ATTR)NULL;
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ULONG ulRecordType, ulRecordSize;
    int rc = RBUS_ERROR_SUCCESS;
    char *parameterName, *parameterValue, *str_value;
    rbusValue_t value;
    rbusProperty_t prop,out_prop;
    rbusValueType_t type;

    if ( pPsmSysRegistry == NULL )
    {
        CcspTraceError(("%s - pPsmSysRegistry is NULL\n",__func__));
        return RBUS_ERROR_BUS_ERROR;
    }

    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;

    if ( pSysInfoRepository == NULL )
    {
        CcspTraceError(("%s - pSysInfoRepository is NULL\n",__func__));
        return RBUS_ERROR_BUS_ERROR;
    }

    pSysIraIf = (PSYS_IRA_INTERFACE)pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);

    pSysIraIf->AcqThreadLock(pSysIraIf->hOwnerContext);

    hSysRoot =
        pSysIraIf->OpenFolder
        (
         pSysIraIf->hOwnerContext,
         (ANSC_HANDLE)NULL,
         "/Configuration/Provision"
        );

    if ( hSysRoot == NULL )
    {
        CcspTraceError(("%s - hSysRoot is NULL\n",__func__));
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        return RBUS_ERROR_BUS_ERROR;
    }

    prop = rbusObject_GetProperties(inParams);
    for( ; (NULL != prop) ; prop = rbusProperty_GetNext(prop))
    {
        parameterName = (char *)rbusProperty_GetName(prop);
        returnStatus =
            pSysIraIf->GetRecord
            (
             pSysIraIf->hOwnerContext,
             hSysRoot,
             parameterName,
             &ulRecordType,
             NULL,
             NULL,
             &ulRecordSize
            );

        if ((returnStatus != ANSC_STATUS_BAD_SIZE)  &&
                (returnStatus != ANSC_STATUS_SUCCESS) &&
                (returnStatus != ANSC_STATUS_CANT_FIND))
        {
            CcspTraceError(("%s failed to get record for the parameter %s . \
                        ReturnStatus %lu\n",__func__,parameterName, returnStatus));
            /*Coverity Fix CID:57874 RESOURCE_LEAK */
            rc = RBUS_ERROR_BUS_ERROR;
            goto EXIT;
        }

        if(ANSC_STATUS_CANT_FIND == returnStatus)
        {
            CcspTraceWarning(("%s - Cannot find %s\n",__func__,parameterName));
            continue;
        }
        parameterValue = (char *)calloc(1,(ulRecordSize+1));
        if(!parameterValue)
        {
            CcspTraceError(("%s - calloc failed\n",__func__));
            rc = RBUS_ERROR_BUS_ERROR;
            goto EXIT;
        }
        returnStatus =
            pSysIraIf->GetRecord
            (
             pSysIraIf->hOwnerContext,
             hSysRoot,
             parameterName,
             &ulRecordType,
             (PANSC_HANDLE)&pRroRenderAttr,
             parameterValue,
             &ulRecordSize
            );
        if(returnStatus != ANSC_STATUS_SUCCESS)
        {
            CcspTraceError(("%s failed to get record for %s . ReturnStatus %lu\n",__func__, parameterName, returnStatus));
            free(parameterValue);
            rc = RBUS_ERROR_BUS_ERROR;
            goto EXIT;
        }
        rbusValue_Init(&value);
        if(true == rbusValue_SetParamVal(value, pRroRenderAttr->ContentType, parameterValue))
        {
            type = rbusValue_GetType(value);
            str_value = rbusValue_ToString(value,NULL,0);
            if(str_value)
            {
                rbusValue_SetFromString(value, type, str_value);
                rbusProperty_Init(&out_prop,rbusProperty_GetName(prop),value);
                rbusObject_SetProperty(outParams,out_prop);
                free(str_value);
            }
        }
        else
        {
            CcspTraceError(("%s Unable to set the value %s of type %lu",__func__,parameterValue,pRroRenderAttr->ContentType));
        }
        rbusValue_Release(value);
    }

EXIT:

    if ( hSysRoot )
    {
        pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);
    }

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
    return rc;
}

static int delParameterValues_rbus(rbusObject_t inParams, rbusObject_t outParams)
{
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    int rc = RBUS_ERROR_SUCCESS;
    char *parameterName;
    rbusProperty_t prop;

    if ( pPsmSysRegistry == NULL )
    {
        CcspTraceError(("%s - pPsmSysRegistry is NULL\n",__func__));
        return RBUS_ERROR_BUS_ERROR;
    }

    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;

    if ( pSysInfoRepository == NULL )
    {
        CcspTraceError(("%s - pSysInfoRepository is NULL\n",__func__));
        return RBUS_ERROR_BUS_ERROR;
    }

    pSysIraIf = (PSYS_IRA_INTERFACE)pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);

    pSysIraIf->AcqThreadLock(pSysIraIf->hOwnerContext);

    hSysRoot =
        pSysIraIf->OpenFolder
            (
                pSysIraIf->hOwnerContext,
                (ANSC_HANDLE)NULL,
                "/Configuration/Provision"
            );

    if ( hSysRoot == NULL )
    {
        CcspTraceError(("%s - hSysRoot is NULL\n",__func__));
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        return RBUS_ERROR_BUS_ERROR;
    }

    prop = rbusObject_GetProperties(inParams);
    for( ; (NULL != prop) ; prop = rbusProperty_GetNext(prop))
    {
        parameterName = (char *)rbusProperty_GetName(prop);
        returnStatus =
            pSysIraIf->DelRecord
            (
             pSysIraIf->hOwnerContext,
             hSysRoot,
             parameterName
            );

        if (ANSC_STATUS_SUCCESS == returnStatus)
        {
            setOutparams(outParams,parameterName,true);
        }
        else
        {
            CcspTraceError(("%s Failed to delete entry: %s ret %lu\n", __func__,parameterName,returnStatus));
            setOutparams(outParams,parameterName,false);
        }
    }

    pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
    return rc;
}

static rbusError_t psmDel(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;

    CcspTraceInfo(("psmGet called: %s %s\n",__func__,methodName));
    return delParameterValues_rbus(inParams,outParams);
}

static rbusError_t psmGet(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;

    CcspTraceInfo(("psmGet called: %s %s\n",__func__,methodName));
    return getParameterValues_rbus(inParams, outParams);
}

static rbusError_t psmSet(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;

    CcspTraceInfo(("psmGet called: %s %s\n",__func__,methodName));
    return setParameterValues_rbus(inParams, outParams);
}

int PsmRbusInit()
{
    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    char *component_name = "rbusPsmSsp";

    rc = rbus_open(&handle, component_name);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceWarning(("%s: rbus_open failed: %d\n", component_name, rc));
        return rc;
    }
#define dataElementsCount sizeof(dataElements)/sizeof(rbusDataElement_t)
    rbusDataElement_t dataElements[] = {
        {"SetPSMRecordValue()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, psmSet}},
        {"DeletePSMRecord()",   RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, psmDel}},
        {"GetPSMRecordValue()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, psmGet}}
    };

    rc = rbus_regDataElements(handle, dataElementsCount, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceWarning(("%s: rbus_regDataElements failed: %d\n", component_name,rc));
        rbus_unregDataElements(handle, dataElementsCount, dataElements);
        rbus_close(handle);
        CcspTraceWarning(("%s: exit\n",component_name));
    }
    return rc;
}
