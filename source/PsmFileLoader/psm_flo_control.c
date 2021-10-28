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

    module: psm_flo_control.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the advanced env-control functions
        of the Psm File Loader Object.

        *   PsmFloTestRegFile
        *   PsmFloLoadRegFile
        *   PsmFloSaveRegFile

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


#include "psm_flo_global.h"


#define  SYSTEM_CONFIG_ROOT_NAME                    "Provision"


/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        PsmFloLoadRegFile
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pCfgBuffer,
                ULONG                       ulCfgSize
            );

    description:

        This function is called to set up the runtime environment.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pCfgBuffer,
                The input buffer;

                ULONG                       ulCfgSize
                The size of the buffer;

    return:     status of operation.

**********************************************************************/
ULONG
PsmFloTestRegFile
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    )
{
    //CcspTraceInfo(("PsmFloTestRegFile begins '\n"));
    UNREFERENCED_PARAMETER(hThisObject);
    PANSC_XML_DOM_NODE_OBJECT       pRootNode = NULL;
    PUCHAR                          pBack     = (PUCHAR)pCfgBuffer;
    ULONG                           uLength   = ulCfgSize;

    pRootNode = (PANSC_XML_DOM_NODE_OBJECT)
        AnscXmlDomParseString((ANSC_HANDLE)NULL, (PCHAR*)&pBack, uLength);
        CcspTraceInfo(("pRootNode Initialized\n"));

    if( pRootNode == NULL)
    {
    	CcspTraceInfo(("PsmFloTestRegFile, Invalid format of pRootNode\n"));
        return PSM_FLO_ERROR_CODE_invalidFormat;
    }

    AnscXmlDomNodeRemove(pRootNode);
    //CcspTraceInfo(("PsmFloTestRegFile, ends '\n"));
    return PSM_FLO_ERROR_CODE_noError;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloLoadRegFile
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to set up the runtime environment.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloLoadRegFile
    (
        ANSC_HANDLE                 hThisObject
    )
{
    //CcspTraceInfo(("PsmFloLoadRegFile begins\n"));
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT)hThisObject;
    PPSM_CFM_INTERFACE              pPsmCfmIf    = (PPSM_CFM_INTERFACE     )pMyObject->hPsmCfmIf;
    PSYS_IRA_INTERFACE              pSysIraIf    = (PSYS_IRA_INTERFACE     )pMyObject->hSysIraIf;
    PANSC_XML_DOM_NODE_OBJECT       pRootNode    = NULL;
    PUCHAR                          pFileData    = NULL;
    PUCHAR                          pBackData    = NULL;
    ULONG                           dataLength   = 0;
    ANSC_HANDLE                     hSysRoot     = NULL;

    if ( !pMyObject->bActive )
    {
    	CcspTraceInfo(("PsmFloLoadRegFile, pMyObject is not Active\n"));
        return ANSC_STATUS_NOT_READY;
    }

    /* get the system root folder */
    hSysRoot =
        pSysIraIf->OpenFolder
            (
                pSysIraIf->hOwnerContext,
                (ANSC_HANDLE)NULL,
                "/Configuration/Provision"
            );

    if ( !hSysRoot)
    {
    	CcspTraceInfo(("PsmFloLoadRegFile, Access Denied for hSysRoot\n"));
        returnStatus =  ANSC_STATUS_ACCESS_DENIED;

        goto EXIT;
    }

    returnStatus =
        pPsmCfmIf->ReadCurConfig
            (
                pPsmCfmIf->hOwnerContext,
                (void**)&pFileData,
                &dataLength
            );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
    	CcspTraceInfo(("PsmFloLoadRegFile, Return Status is not Success \n"));
        goto EXIT;
    }

    /* decoding the XML data */
    pBackData = pFileData;
    pRootNode =
        (PANSC_XML_DOM_NODE_OBJECT)AnscXmlDomParseString
            (
                (ANSC_HANDLE)NULL,
                (PCHAR*)&pBackData,
                dataLength
            );

    if ( pRootNode == NULL )
    {
    	CcspTraceInfo(("PsmFloLoadRegFile, pRootNode is NULL set Return status and EXIT\n"));
        returnStatus = ANSC_STATUS_RESOURCES;

        goto EXIT;
    }

    /* add the content */
    returnStatus =
        PsmSysFolderFromXMLHandle
            (
                pSysIraIf,
                hSysRoot,
                (ANSC_HANDLE)pRootNode
            );

EXIT:

    if ( pFileData != NULL )
    {
    	AnscFreeMemory(pFileData);
    	CcspTraceInfo(("PsmFloLoadRegFile, pFileData is free\n"));
    }

    if ( pRootNode != NULL )
    {
    	pRootNode->Remove(pRootNode);
    	CcspTraceInfo(("PsmFloLoadRegFile, pRootNode is free\n"));
    }

    if ( hSysRoot != NULL )
    {
        pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);
    	CcspTraceInfo(("PsmFloLoadRegFile, hSysRoot folder closed\n"));
    }

   // CcspTraceInfo(("PsmFloLoadRegFile, ends '\n"));
    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloSaveRegFile
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to close the runtime environment.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloSaveRegFile
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT)hThisObject;
    PPSM_CFM_INTERFACE              pPsmCmfIf    = (PPSM_CFM_INTERFACE     )pMyObject->hPsmCfmIf;
    PSYS_IRA_INTERFACE              pSysIraIf    = (PSYS_IRA_INTERFACE     )pMyObject->hSysIraIf;
    PANSC_XML_DOM_NODE_OBJECT       pRootNode    = NULL;
    PUCHAR                          pFileData    = NULL;
    ULONG                           dataLength   = 0;
    ULONG                           totalLength  = 0;
    ANSC_HANDLE                     hSysRoot     = NULL;
    //CcspTraceInfo(("PsmFloSaveRegFile begins \n"));
    if ( !pMyObject->bActive )
    {
        return ANSC_STATUS_NOT_READY;
    }

    CcspTraceInfo(("AcqWriteAccess in 'PsmFloSaveRegFile'\n"));

    returnStatus =
        pSysIraIf->AcqWriteAccess(pSysIraIf->hOwnerContext);

    if( returnStatus != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("Failed to AcqWriteAccess in 'PsmFloSaveRegFile'\n"));

        return returnStatus;
    }

    /* get the system root folder */
    hSysRoot =
        pSysIraIf->OpenFolder
            (
                pSysIraIf->hOwnerContext,
                (ANSC_HANDLE)NULL,
                "/Configuration/Provision"
            );

    if ( !hSysRoot)
    {
        returnStatus = ANSC_STATUS_ACCESS_DENIED;

        goto EXIT;
    }

    /* write the system folder to XML handle */
    pRootNode = (PANSC_XML_DOM_NODE_OBJECT)AnscCreateXmlDomNode(NULL);

    if( pRootNode == NULL)
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto EXIT;
    }

    AnscXmlDomNodeSetName(pRootNode, SYSTEM_CONFIG_ROOT_NAME);

    returnStatus =
        PsmSysFolderToXMLHandle
            (
                (ANSC_HANDLE)pSysIraIf,
                hSysRoot,
                (ANSC_HANDLE)pRootNode
            );

    if( ANSC_STATUS_SUCCESS != returnStatus)
    {
        goto EXIT;
    }

    /* write to data */
    dataLength = AnscXmlDomNodeGetEncodedSize(pRootNode);

    if( dataLength == 0)
    {
        returnStatus = ANSC_STATUS_FAILURE;

        goto EXIT;
    }

    pFileData = (PUCHAR)AnscAllocateMemory(dataLength + 64);

    if( pFileData == NULL)
    {
        returnStatus = ANSC_STATUS_RESOURCES;

        goto EXIT;
    }

    totalLength = dataLength;

    /* be careful, if this function succeeded, the findal dataLength == 0 */
    returnStatus = AnscXmlDomNodeEncode(pRootNode, (PVOID)pFileData, &dataLength);

    if( returnStatus != ANSC_STATUS_SUCCESS)
    {
        goto EXIT;
    }

    if( (LONG)dataLength != 0)
    {
        CcspTraceWarning(("****Warning!******\n Write XML node size not matched: %lu\n", (LONG)dataLength));
    }

    /* write to config */
    pSysIraIf->RelWriteAccess(pSysIraIf->hOwnerContext);
    returnStatus =
        pPsmCmfIf->SaveCurConfig
            (
                pPsmCmfIf->hOwnerContext,
                (void*)pFileData,
                totalLength
            );
    pSysIraIf->AcqWriteAccess(pSysIraIf->hOwnerContext);


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT:

    if( pFileData != NULL)
    {
        AnscFreeMemory(pFileData);
    }

    if( pRootNode != NULL)
    {
    	AnscXmlDomNodeRemove(pRootNode);
    }

    if( hSysRoot != NULL)
    {
        pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);
    }

    /* release the write access */
    pSysIraIf->RelWriteAccess(pSysIraIf->hOwnerContext);

    CcspTraceInfo(("RelWriteAccess in 'PsmFloSaveRegFile'\n"));
    //CcspTraceInfo(("PsmFloSaveRegFile ends \n"));
    return  returnStatus;
}
