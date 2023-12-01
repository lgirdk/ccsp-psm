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

    module: psm_flo_parse.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements SysFolder <==> XML Handle functions
        of the Psm File Loader Object.

        *   PsmSysFolderToXMLHandle
        *   PsmSysFolderFromXMLHandle

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
#include "safec_lib_common.h"

#define  STR_FOLDER                                 "Folder"
#define  STR_RECORD                                 "Record"
#define  STR_NAME                                   "name"
#define  STR_ACCESS                                 "permission"
#define  STR_TYPE                                   "type"
#define  STR_CONTENT_TYPE                           "contentType"

#define  DEFAULT_RFO_PERMISSION                     SYS_RFO_ACCESS_MODE_ALL
#define  DEFAULT_RRO_PERMISSION                     SYS_RRO_ACCESS_MODE_ALL

static ULONG uTotalRecordType  = 7;
static char  ppRecordType[][8] =
{
    "sint","uint","bool","astr","bstr","hcxt","enum"
};

static ULONG uTotalRecordContentType  = 15;
static char ppRecordContentType[][16] =
{
    "int", "uint", "bool", "datetime", "base64", "long", "ulong", "float",
    "double", "byte", "none", "unknown1", "unknown2", "unknown3", "unknown4", "unknown5"
};

static char ppRecordContentType_old[][16] =
{
    "mapped","ip4Addr","macAddr","ip4AddrList","port","seconds","minutes",
    "hours","calendar","password","masked","button","switch","sintList","uintList"
};

static ULONG uTotalFolderType  = 4;
static char ppFolderType[][8]  =
{
    "storage","guarded","dynamic","session"
};

static ULONG uTotalFolderContentType  = 6;
static char ppFolderContentType[][16] =
{
    "category","table2D","table3D","chartPie","chartBar","graph"
};


/**********************************************************************

    static functions

**********************************************************************/
static
void writeIP4Address
    (
        PANSC_XML_DOM_NODE_OBJECT       pRecordNode,
        PUCHAR                          pData
    )
{
    CHAR                                pTemp[32]   = { 0 };

    _ansc_sprintf
        (
            pTemp,
            "%d.%d.%d.%d",
            pData[0],
            pData[1],
            pData[2],
            pData[3]
        );

    AnscXmlDomNodeSetDataString(pRecordNode, NULL, pTemp, AnscSizeOfString(pTemp));
}

static
ANSC_STATUS
decodeIpAddr
    (
        char                        *pEncodedRecord,
        ULONG                       ulEncodedSize,
        PUCHAR                      pDecode
    )
{
    char                            *pPos, *pNext, *pCur;
    ANSC_IPV4_ADDRESS               Ip;
    ULONG                           ulCount = 0;
    ULONG                           uVal;
    ANSC_STATUS                     status  = ANSC_STATUS_SUCCESS;

    if (ulEncodedSize == 0)
    {
        AnscZeroMemory(pDecode, sizeof(ULONG));

        return ANSC_STATUS_SUCCESS;
    }

    pCur    = pEncodedRecord;

    while (pCur)
    {
        pPos    = _ansc_strchr(pCur, '.');

        if (pPos)
        {
            pNext   = pPos + 1;
            *pPos   = 0;
        }
        else
        {
            pNext   = NULL;
        }

        uVal    = _ansc_atoi(pCur);

        if (uVal > 0xFF)
        {
            status  = ANSC_STATUS_FAILURE;

            break;
        }

        ulCount ++;

        if (ulCount > IPV4_ADDRESS_SIZE)
        {
            status  = ANSC_STATUS_FAILURE;

            break;
        }

        Ip.Dot[ulCount - 1]    = (UCHAR)uVal;

        pCur    = pNext;
    }

    if (ulCount !=  IPV4_ADDRESS_SIZE)
    {
        status  = ANSC_STATUS_FAILURE;
    }

    if (status == ANSC_STATUS_SUCCESS)
    {
        AnscCopyMemory(pDecode, Ip.Dot, sizeof(IPV4_ADDRESS_SIZE));
    }

    return status;
}

static
void writeCalendar
    (
        PANSC_XML_DOM_NODE_OBJECT       pRecordNode,
        PUCHAR                          pData,
        ULONG                           dataSize
    )
{
    UNREFERENCED_PARAMETER(dataSize);
    PANSC_UNIVERSAL_TIME                pTime       = (PANSC_UNIVERSAL_TIME)pData;
    CHAR                                pTemp[64]   = { 0 };

    _ansc_sprintf
        (
            pTemp,
            "%.4d/%.2d/%.2d %.2d:%.2d:%.2d",
            pTime->Year,
            pTime->Month,
            pTime->DayOfMonth,
            pTime->Hour,
            pTime->Minute,
            pTime->Second
        );

    AnscXmlDomNodeSetDataString(pRecordNode, NULL, pTemp, AnscSizeOfString(pTemp));
}

static
ANSC_STATUS
decodeCalendar
    (
        PUCHAR                          pDate,
        PANSC_UNIVERSAL_TIME            pTime
    )
{
    ULONG                               ulIndex = 0;
    /* Coverity Issue Fix - CID:68447 : UnInitialised Variable*/
    ULONG                               ulDate[6] = {0};
    PUCHAR                              pPos, pNext;
    char                                value[8] = {0};
    char                                ch;

    while (pDate)
    {
        if (ulIndex >= 6)
        {
            break;
        }

        while (TRUE)
        {
            ch  = *pDate;
            if ((ch >= '0' && ch <= '9') || ch == '\0')
            {
                break;
            }

            pDate ++;
        }

        pPos    = pDate + 1;

        while (TRUE)
        {
            ch  = *pPos;

            if (ch < '0' || ch > '9' || ch == '\0')
            {
                break;
            }

            pPos ++;
        }

        AnscCopyMemory(value, pDate, (ULONG)pPos - (ULONG)pDate);
        value[(ULONG)pPos - (ULONG)pDate] = 0;
        pNext   = pPos + 1;

        ulDate[ulIndex ++]  = (ULONG)_ansc_atol(value);

        pDate   = pNext;
    }

    pTime->bDayLightSaving  = FALSE;
    pTime->Year             = (USHORT)ulDate[0];
    pTime->Month            = (USHORT)ulDate[1];
    pTime->DayOfMonth       = (USHORT)ulDate[2];
    pTime->Hour             = (USHORT)ulDate[3];
    pTime->Minute           = (USHORT)ulDate[4];
    pTime->Second           = (USHORT)ulDate[5];
    pTime->MilliSecond      = 0;

    return ANSC_STATUS_SUCCESS;
}

static
void writeMacAddress
    (
        PANSC_XML_DOM_NODE_OBJECT       pRecordNode,
        PUCHAR                          pData,
        ULONG                           dataSize
    )
{
    CHAR                                pTemp[64]   = { 0 };
    ULONG                               i;

    for( i = 0; i < dataSize; i ++)
    {
        if( i == 0)
        {
            _ansc_sprintf
                (
                    pTemp,
                    "%.2X",
                    pData[i]
                );
        }
        else
        {
            _ansc_sprintf
                (
                    pTemp + strlen(pTemp),
                    ":%.2X",
                    pData[i]
                );
        }
    }

    AnscXmlDomNodeSetDataString(pRecordNode, NULL, pTemp, AnscSizeOfString(pTemp));
}

//unused function
#if 0
static
BOOL
isValidBinaryChar
    (
        char c
    )
{
    return (c >= '0' && c <= '9') || ( c >= 'A' && c <= 'F' );
}
#endif

static
ANSC_STATUS
decodeMacAddress
    (
        PVOID                       pCharData,
        PULONG                      pulSize
    )
{
    /***********************************************************
                    DEFINITION OF LOCAL VARIABLES
    ***********************************************************/

    /*
     * status of operation
     */
    /*
     * temporary counters
     */
    ULONG                           i = 0;

    /*
     * pointer to the converted string
     */
    PUCHAR                          pBinaryData = NULL;
    ULONG                           ulDataSize  = *pulSize / 3 + 1;
    UCHAR                           tempChar    = 0;


    /***********************************************************
            MAKE THE CONVERSION FROM ASCII TO BINARY DATA
    ***********************************************************/

    /*
     * each binary octet needs two ASCII chars
     */
    if ( (*pulSize % 3) != 2 )
    {
        CcspTraceWarning(("Invalid Mac Address String '%p'\n", pCharData));

        return  ANSC_STATUS_FAILURE;
    }

    pBinaryData = (PUCHAR)AnscAllocateMemory(ulDataSize);

    for ( i = 0; i < ulDataSize * 3 - 1; i++ )
    {
        tempChar = ((PCHAR)pCharData)[i];

        /* check the value ':' */
        if( i % 3 == 2)
        {
            if( tempChar != ':')
            {
                CcspTraceWarning(("Invalid Mac Address String '%p'\n", pCharData));
                AnscFreeMemory(pBinaryData); /*10-May-2016 RDKB-5568 CID-32890, free the pBinaryData in case of invalid Address String*/
                return  ANSC_STATUS_FAILURE;
            }
        }
        else
        {
            if ( tempChar <= '9' && tempChar >= '0' )
            {
                tempChar -= '0';
            }
            else if ( tempChar <= 'F' && tempChar >= 'A' )
            {
                tempChar -= 'A';
                tempChar += (UCHAR)10;
            }
            else if ( tempChar <= 'f' && tempChar >= 'a' )
            {
                tempChar -= 'a';
                tempChar += (UCHAR)10;
            }
            else
            {
                AnscFreeMemory(pBinaryData);

                return  ANSC_STATUS_XML_INVALID_ATTRIBUTE_VALUE;
            }

            pBinaryData[i / 3] += (i % 3 != 0)? tempChar : tempChar << 4;
        }
    }

    AnscCopyMemory(pCharData, pBinaryData, ulDataSize);

    *pulSize = ulDataSize;

    AnscFreeMemory(pBinaryData);

    return  ANSC_STATUS_SUCCESS;
}

static
PULONG
decodeIntList
    (
        PCHAR                           pData,
        PULONG                          pLength
    )
{
    PULONG                              pUlongArray         = NULL;
    ULONG                               uTotal              = 0;
    PCHAR                               pCur, pNext, pPos;

    if( pData == NULL)
    {
        *pLength = 0;

        return NULL;
    }

    pCur    = pData;

    while (pCur)
    {
        pPos    = _ansc_strchr(pCur, ',');

        if (!pPos)
        {
            pNext   = NULL;

            if( AnscSizeOfString(pCur) > 0)
            {
                uTotal ++;
            }
        }
        else
        {
            pNext   = pPos + 1;

            if( pPos > pCur)
            {
                uTotal ++;
            }
        }

        pCur    = pNext;
    }

    if( uTotal == 0)
    {
        return NULL;
    }

    /* allocate the memory */
    pUlongArray = (PULONG)AnscAllocateMemory( uTotal * sizeof(ULONG));

    if( pUlongArray == NULL)
    {
        return NULL;
    }

    pCur    = pData;
    uTotal  = 0;

    while (pCur)
    {
        pPos    = _ansc_strchr(pCur, ',');

        if (!pPos)
        {
            pNext   = NULL;
        }
        else
        {
            pNext   = pPos + 1;
            *pPos   = 0;
        }

        if( AnscSizeOfString(pCur) > 0)
        {
            pUlongArray[uTotal] = _ansc_atoi(pCur);

            uTotal ++;
        }

        pCur    = pNext;
    }

    *pLength = uTotal;

    return pUlongArray;
}

static
ANSC_STATUS
writeIntList
    (
        PANSC_XML_DOM_NODE_OBJECT       pRecordNode,
        PUCHAR                          pData,
        ULONG                           length
    )
{
    PULONG                              pUlongArray  = (PULONG)pData;
    char                                pString[64]  = { 0 };
    ULONG                               i;
    ULONG                               uTemp;

    if( length == 0)
    {
        return ANSC_STATUS_SUCCESS;
    }

    if( length % sizeof(ULONG) != 0 || length > 64)
    {
        return ANSC_STATUS_FAILURE;
    }

    for( i = 0; i < length / 4; i ++)
    {
        uTemp = pUlongArray[i];

        if( i == 0)
        {
            _ansc_sprintf
                (
                    pString,
                    "%lu",
                    uTemp
                );
        }
        else
        {
            _ansc_sprintf
                (
                    pString + strlen(pString),
                    ",%lu",
                    uTemp
                );
        }
    }

    AnscXmlDomNodeSetDataString(pRecordNode, NULL, pString, AnscSizeOfString(pString));

    return ANSC_STATUS_SUCCESS;
}

static
PUCHAR
decodeIP4AddrList
    (
        PCHAR                           pData,
        PULONG                          pLength
    )
{
    ULONG                               uTotal              = 0;
    PCHAR                               pCur, pNext, pPos;
    PUCHAR                              pCharArray;

    if( pData == NULL)
    {
        *pLength = 0;

        return NULL;
    }

    pCur    = pData;

    while (pCur)
    {
        pPos    = _ansc_strchr(pCur, ',');

        if (!pPos)
        {
            pNext   = NULL;

            if( AnscSizeOfString(pCur) > 0)
            {
                uTotal ++;
            }
        }
        else
        {
            pNext   = pPos + 1;

            if( pPos > pCur)
            {
                uTotal ++;
            }
        }

        pCur    = pNext;
    }

    if( uTotal == 0)
    {
        return NULL;
    }

    /* allocate the memory */
    pCharArray = (PUCHAR)AnscAllocateMemory( uTotal * sizeof(ULONG));

    if( pCharArray == NULL)
    {
        return NULL;
    }

    pCur    = pData;
    uTotal  = 0;

    while (pCur)
    {
        pPos    = _ansc_strchr(pCur, ',');

        if (!pPos)
        {
            pNext   = NULL;
        }
        else
        {
            pNext   = pPos + 1;
            *pPos   = 0;
        }

        if( AnscSizeOfString(pCur) > 0)
        {
            if( ANSC_STATUS_SUCCESS !=
                  decodeIpAddr
                    (
                        pCur,
                        AnscSizeOfString(pCur),
                        (PUCHAR)((ULONG)pCharArray + uTotal * 4)
                    ))
            {
                AnscFreeMemory(pCharArray);

                return NULL;
            }

            uTotal ++;
        }

        pCur    = pNext;
    }

    *pLength = uTotal * sizeof(ULONG);

    return pCharArray;
}

static
ANSC_STATUS
writeIP4AddrList
    (
        PANSC_XML_DOM_NODE_OBJECT       pRecordNode,
        PUCHAR                          pData,
        ULONG                           length
    )
{
    PCHAR                               pString      = NULL;
    CHAR                                pTemp[32]    = { 0 };
    ULONG                               i            = 0;

    if( length == 0)
    {
        return ANSC_STATUS_SUCCESS;
    }

    if( length % sizeof(ULONG) != 0)
    {
        return ANSC_STATUS_FAILURE;
    }

    pString = (PCHAR)AnscAllocateMemory(length * 16 / sizeof(ULONG));

    if( pString == NULL)
    {
        return ANSC_STATUS_RESOURCES;
    }

    for( i = 0; i < length / 4; i ++)
    {
        AnscZeroMemory(pTemp, 32);

        _ansc_sprintf
            (
                pTemp,
                "%d.%d.%d.%d",
                pData[ i * 4],
                pData[ i * 4 + 1],
                pData[ i * 4 + 2],
                pData[ i * 4 + 3]
            );

        if( i == 0)
        {
            _ansc_sprintf
                (
                    pString,
                    "%s",
                    pTemp
                );
        }
        else
        {
            _ansc_sprintf
                (
                    pString + strlen(pString),
                    ",%s",
                    pTemp
                );
        }
    }

    AnscXmlDomNodeSetDataString(pRecordNode, NULL, pString, AnscSizeOfString(pString));

    AnscFreeMemory(pString);

    return ANSC_STATUS_SUCCESS;
}

static
ULONG getRecordTypeFromString
    (
        PCHAR                       pString
    )
{
    ULONG                           i;
    errno_t                         rc  = -1;
    int                             ind = -1;

    for( i = 0; i < uTotalRecordType; i ++)
    {
	rc = strcasecmp_s(ppRecordType[i], sizeof(ppRecordType[i]), pString, &ind);
	ERR_CHK(rc);
        if((rc == EOK) && (!ind))
        {
            return i + 1;
        }
    }

    CcspTraceInfo(("Unknown Record Type '%s'\n", pString));

    return 0;
}

static
ULONG getRecordContentTypeFromString
    (
        PCHAR                       pString
    )
{
    ULONG                           i;
    errno_t                         rc  = -1;
    int                             ind = -1;

    for( i = 0; i < uTotalRecordContentType; i ++)
    {
	rc =strcasecmp_s(ppRecordContentType[i], sizeof(ppRecordContentType[i]), pString, &ind);
        ERR_CHK(rc);
	if((rc == EOK) && (!ind))
        {
            return i + 1;
        }
    }

    for( i = 0; i < uTotalRecordContentType; i ++)
    {
	rc =strcasecmp_s(ppRecordContentType_old[i], sizeof(ppRecordContentType_old[i]), pString, &ind);
	ERR_CHK(rc);
	if((rc == EOK) && (!ind))
        {
            CcspTraceWarning(("Legacy Record ContentType '%s' !!!\n", pString));
            return i + 1;
        }
    }

    CcspTraceInfo(("Unknown Record ContentType '%s', assume it's DEFAULT.\n", pString));

    return 0;
}

static
ULONG getFolderTypeFromString
    (
        PCHAR                       pString
    )
{
    ULONG                           i;
    errno_t                         rc = -1;
    int                             ind = -1;

    for( i = 0; i < uTotalFolderType; i ++)
    {
	rc = strcasecmp_s(ppFolderType[i], sizeof(ppFolderType[i]), pString, &ind);
        ERR_CHK(rc);
        if((rc == EOK) && (!ind))
        {
            return i + 1;
        }
    }

    CcspTraceInfo(("Unknown Folder Type '%s'\n", pString));

    return 0;
}

static
ULONG getFolderContentTypeFromString
    (
        PCHAR                       pString
    )
{
    ULONG                           i;
    errno_t                         rc = -1;
    int                             ind = -1;

    for( i = 0; i < uTotalFolderContentType; i ++)
    {
	rc = strcasecmp_s(ppFolderContentType[i], sizeof(ppFolderContentType[i]), pString, &ind);
        ERR_CHK(rc);
        if((rc == EOK) && (!ind))
        {
            return i + 1;
        }
    }

    CcspTraceInfo(("Unknown Folder ContentType '%s', assume it's DEFAULT.\n", pString));

    return 0;
}

//unused function
#if 0
static 
ANSC_STATUS
addRecordToXMLHandle
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ULONG                       nRecordIndex,
        ANSC_HANDLE                 hXMLHandle
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PSYS_IRA_INTERFACE              pSysIraIf          = (PSYS_IRA_INTERFACE       )hSysIraIf;
    PANSC_XML_DOM_NODE_OBJECT       pXMLRoot           = (PANSC_XML_DOM_NODE_OBJECT)hXMLHandle;
    PANSC_XML_DOM_NODE_OBJECT       pRecordNode        = NULL;
    ULONG                           nameSize           = SYS_MAX_RECORD_NAME_SIZE;
    ULONG                           recType, dataSize;
    ULONG                           access,contentType;
    CHAR                            pName[SYS_MAX_RECORD_NAME_SIZE];
    PUCHAR                          pData;

    AnscZeroMemory(pName, SYS_MAX_RECORD_NAME_SIZE);

    returnStatus =
        pSysIraIf->EnumRecord
            (
                pSysIraIf->hOwnerContext,
                hSysFolder,
                nRecordIndex,
                pName,
                &nameSize,
                &recType,
                &dataSize
            );

    if( returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    pRecordNode = (PANSC_XML_DOM_NODE_OBJECT)AnscCreateXmlDomNode(NULL);

    if( pRecordNode == NULL)
    {
        return ANSC_STATUS_RESOURCES;
    }

    AnscXmlDomNodeSetName(pRecordNode, STR_RECORD);
    AnscXmlDomNodeAddChild(pXMLRoot, pRecordNode);

    AnscXmlDomNodeSetAttrString
        (
            pRecordNode,
            STR_NAME,
            pName,
            AnscSizeOfString(pName)
        );

    AnscXmlDomNodeSetAttrString
        (
            pRecordNode,
            STR_TYPE,
            ppRecordType[recType - 1],
            AnscSizeOfString(ppRecordType[recType - 1])
        );

    /* get other infor */
    pSysIraIf->QueryRecord
        (
            pSysIraIf->hOwnerContext,
            hSysFolder,
            pName,
            NULL,           /* we don't care about timestamps right now */
            &access,
            &recType,
            &dataSize,
            &contentType,
            NULL
         );

    if( access != DEFAULT_RRO_PERMISSION)
    {
        AnscXmlDomNodeSetAttrUlong
            (
                pRecordNode,
                STR_ACCESS,
                access
            );
    }

    if( contentType != SYS_RECORD_CONTENT_DEFAULT)
    {
        AnscXmlDomNodeSetAttrString
            (
                pRecordNode,
                STR_CONTENT_TYPE,
                ppRecordContentType[contentType - 1],
                AnscSizeOfString(ppRecordContentType[contentType - 1])
            );
    }

    /* get the data */

    dataSize ++;
    pData = (PUCHAR)AnscAllocateMemory(dataSize);

    if( pData == NULL)
    {

        return ANSC_STATUS_RESOURCES;
    }

    AnscZeroMemory(pData, dataSize);

    /* get the value */
    pSysIraIf->GetRecord
        (
            pSysIraIf->hOwnerContext,
            hSysFolder,
            pName,
            &recType,
            NULL,
            (PVOID)pData,
            &dataSize
         );

    if( recType == SYS_REP_RECORD_TYPE_SINT || recType == SYS_REP_RECORD_TYPE_UINT ||
        recType == SYS_REP_RECORD_TYPE_HCXT )
    {
        if( contentType == SYS_RECORD_CONTENT_IP4_ADDR)
        {
            writeIP4Address(pRecordNode, (PUCHAR)pData);
        }
        else
        {
            AnscXmlDomNodeSetDataUlong(pRecordNode, NULL, *(PULONG)pData);
        }
    }
    else if( recType == SYS_REP_RECORD_TYPE_BOOL)
    {
        if( *(PBOOL)pData != 0)
        {
            AnscXmlDomNodeSetDataBoolean(pRecordNode, NULL, TRUE);
        }
        else
        {
            AnscXmlDomNodeSetDataBoolean(pRecordNode, NULL, FALSE);
        }
    }
    else if( recType == SYS_REP_RECORD_TYPE_ASTR)
    {
        AnscXmlDomNodeSetDataString(pRecordNode, NULL,(PCHAR) pData, dataSize);
    }
    else  if( recType == SYS_REP_RECORD_TYPE_BSTR)
    {
        if( contentType == SYS_RECORD_CONTENT_IP4_ADDR)
        {
            writeIP4Address(pRecordNode, (PUCHAR)pData);
        }
        else if( contentType == SYS_RECORD_CONTENT_MAC_ADDR)
        {
            writeMacAddress(pRecordNode, (PUCHAR)pData, dataSize);
        }
        else if( contentType == SYS_RECORD_CONTENT_CALENDAR_TIME)
        {
            writeCalendar(pRecordNode, (PUCHAR)pData, dataSize);
        }
        else if( contentType == SYS_RECORD_CONTENT_SINT_LIST || contentType == SYS_RECORD_CONTENT_UINT_LIST)
        {
            if( writeIntList(pRecordNode, pData, dataSize) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("Invalid int list data.\n"));
              /*Coverity Fix CID: 55479 RESOURCE_LEAK */  
                if(pData != NULL)
                   AnscFreeMemory(pData);
                return ANSC_STATUS_FAILURE;
            }
        }
        else if( contentType == SYS_RECORD_CONTENT_IP4_ADDR_LIST)
        {
            if( writeIP4AddrList(pRecordNode, pData, dataSize) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("Invalid ip4 address list.\n"));
              /*Coverity Fix CID: 55479 RESOURCE_LEAK */  
                if(pData != NULL)
                   AnscFreeMemory(pData);
               return ANSC_STATUS_FAILURE;
            }
        }
        else
        {
            AnscXmlDomNodeSetDataBinary(pRecordNode, NULL,(PCHAR) pData, dataSize);
        }
    }
    else
    {
        CcspTraceInfo(("Unsurpported record type: %d\n", recType));
    }

    /* free the allocated memory */
    if( pData != NULL)
    {
        AnscFreeMemory(pData);
    }

    return ANSC_STATUS_SUCCESS;
}
#endif

static
ANSC_STATUS
addRecordToXMLHandle2
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ULONG                       nRecordIndex,
        ANSC_HANDLE                 hXMLHandle
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PSYS_IRA_INTERFACE              pSysIraIf          = (PSYS_IRA_INTERFACE       )hSysIraIf;
    PANSC_XML_DOM_NODE_OBJECT       pXMLRoot           = (PANSC_XML_DOM_NODE_OBJECT)hXMLHandle;
    PANSC_XML_DOM_NODE_OBJECT       pRecordNode        = NULL;
    ULONG                           nameSize           = SYS_MAX_RECORD_NAME_SIZE;
    ULONG                           recType, dataSize;
    ULONG                           access,contentType;
    CHAR                            pName[SYS_MAX_RECORD_NAME_SIZE];
    PUCHAR                          pData = NULL;

    returnStatus = 
        pSysIraIf->GetRecordByIndex
            (
                 pSysIraIf->hOwnerContext,
                 hSysFolder,
                 nRecordIndex,
                 pName, 
                 &nameSize,
                 &recType,
                 &contentType,
                 &access,
                 &pData,
                 &dataSize
            );

    if (returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    pRecordNode = (PANSC_XML_DOM_NODE_OBJECT)AnscCreateXmlDomNode(NULL);
    if( pRecordNode == NULL)
    {
        return ANSC_STATUS_RESOURCES;
    }

    AnscXmlDomNodeSetName(pRecordNode, STR_RECORD);
    AnscXmlDomNodeAddChild(pXMLRoot, pRecordNode);

    AnscXmlDomNodeSetAttrString
        (
            pRecordNode,
            STR_NAME,
            pName,
            AnscSizeOfString(pName)
        );

    AnscXmlDomNodeSetAttrString
        (
            pRecordNode,
            STR_TYPE,
            ppRecordType[recType - 1],
            AnscSizeOfString(ppRecordType[recType - 1])
        );

    if( contentType != SYS_RECORD_CONTENT_DEFAULT)
    {
        AnscXmlDomNodeSetAttrString
            (
                pRecordNode,
                STR_CONTENT_TYPE,
                ppRecordContentType[contentType - 1],
                AnscSizeOfString(ppRecordContentType[contentType - 1])
            );
    }

    if( recType == SYS_REP_RECORD_TYPE_SINT || recType == SYS_REP_RECORD_TYPE_UINT ||
        recType == SYS_REP_RECORD_TYPE_HCXT )
    {
        if( contentType == SYS_RECORD_CONTENT_IP4_ADDR)
        {
            writeIP4Address(pRecordNode, (PUCHAR)pData);
        }
        else
        {
            AnscXmlDomNodeSetDataUlong(pRecordNode, NULL, *(PULONG)pData);
        }
    }
    else if( recType == SYS_REP_RECORD_TYPE_BOOL)
    {
        if( *(PBOOL)pData != 0)
        {
            AnscXmlDomNodeSetDataBoolean(pRecordNode, NULL, TRUE);
        }
        else
        {
            AnscXmlDomNodeSetDataBoolean(pRecordNode, NULL, FALSE);
        }
    }
    else if( recType == SYS_REP_RECORD_TYPE_ASTR)
    {
        AnscXmlDomNodeSetDataString(pRecordNode, NULL,(PCHAR) pData, dataSize);
    }
    else  if( recType == SYS_REP_RECORD_TYPE_BSTR)
    {
        if( contentType == SYS_RECORD_CONTENT_IP4_ADDR)
        {
            writeIP4Address(pRecordNode, (PUCHAR)pData);
        }
        else if( contentType == SYS_RECORD_CONTENT_MAC_ADDR)
        {
            writeMacAddress(pRecordNode, (PUCHAR)pData, dataSize);
        }
        else if( contentType == SYS_RECORD_CONTENT_CALENDAR_TIME)
        {
            writeCalendar(pRecordNode, (PUCHAR)pData, dataSize);
        }
        else if( contentType == SYS_RECORD_CONTENT_SINT_LIST || contentType == SYS_RECORD_CONTENT_UINT_LIST)
        {
            if( writeIntList(pRecordNode, pData, dataSize) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("Invalid int list data.\n"));
                if( pData != NULL)
                {
                    AnscFreeMemory(pData);
                }
                return ANSC_STATUS_FAILURE;
            }
        }
        else if( contentType == SYS_RECORD_CONTENT_IP4_ADDR_LIST)
        {
            if( writeIP4AddrList(pRecordNode, pData, dataSize) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("Invalid ip4 address list.\n"));
                if( pData != NULL)
                {
                    AnscFreeMemory(pData);
                }
                return ANSC_STATUS_FAILURE;
            }
        }
        else
        {
            AnscXmlDomNodeSetDataBinary(pRecordNode, NULL,(PCHAR) pData, dataSize);
        }
    }
    else
    {
        CcspTraceInfo(("Unsurpported record type: %lu\n", recType));
    }

    /* free the allocated memory */
    if( pData != NULL)
    {
        AnscFreeMemory(pData);
    }

    return ANSC_STATUS_SUCCESS;
}

static
ANSC_STATUS
loadRecordFromXML
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ANSC_HANDLE                 hXMLRecord
    )
{
    PSYS_IRA_INTERFACE              pSysIraIf          = (PSYS_IRA_INTERFACE       )hSysIraIf;
    PANSC_XML_DOM_NODE_OBJECT       pChildNode         = (PANSC_XML_DOM_NODE_OBJECT)hXMLRecord;
    CHAR                            pName[SYS_MAX_RECORD_NAME_SIZE + 1];
    CHAR                            pValue[SYS_MAX_RECORD_NAME_SIZE + 1];
    ULONG                           length;
    ULONG                           recordType         = 0;
    ULONG                           permission         = DEFAULT_RRO_PERMISSION;
    ULONG                           contentType        = 0;
    SYS_RRO_RENDER_ATTR             renderAttr         = { 0 };
    ULONG                           longValue          = 0;
    BOOL                            boolValue          = FALSE;
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;

    /* get the name of the record; */
    length = sizeof(pName);
    if ((AnscXmlDomNodeGetAttrString(pChildNode,STR_NAME, pName, &length) != ANSC_STATUS_SUCCESS) || (length >= sizeof(pName)))
    {
        CcspTraceWarning(("Failed to get the 'name' attribute of this node.\n"));

        return ANSC_STATUS_FAILURE;
    }

    /* get the record type */
    length = sizeof(pValue);
    if ((AnscXmlDomNodeGetAttrString(pChildNode,STR_TYPE, pValue, &length) != ANSC_STATUS_SUCCESS) || (length >= sizeof(pValue)))
    {
        recordType = SYS_REP_RECORD_TYPE_SINT;
    }
    else
    {
        recordType = getRecordTypeFromString(pValue);
    }

    if( recordType == 0)
    {
        CcspTraceWarning(("Invalid record type.\n"));

        return ANSC_STATUS_FAILURE;
    }

    /* get the contentType */
    length = sizeof(pValue);
    if ((AnscXmlDomNodeGetAttrString(pChildNode,STR_CONTENT_TYPE, pValue, &length) == ANSC_STATUS_SUCCESS) && (length < sizeof(pValue)))
    {
        contentType = getRecordContentTypeFromString(pValue);
        renderAttr.ContentType = contentType;
    }

    /* get the permission */
    /*Coverity Fix CID:75138 CHECKED_RETURN */
    if(AnscXmlDomNodeGetAttrUlong(pChildNode, STR_ACCESS, &permission) != ANSC_STATUS_SUCCESS)
        CcspTraceDebug(("AnscXmlDomNodeGetAttrUlong is not success \n"));


    /* get the value */
    if( recordType == SYS_REP_RECORD_TYPE_SINT || recordType == SYS_REP_RECORD_TYPE_UINT ||
        recordType == SYS_REP_RECORD_TYPE_HCXT || recordType == SYS_REP_RECORD_TYPE_ENUM)
    {
        if( contentType == SYS_RECORD_CONTENT_IP4_ADDR)
        {
            length = sizeof(pValue);
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pValue, &length);

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length == 0) || (length >= sizeof(pValue)))
            {
                CcspTraceWarning(("Failed to read string value '%p'\n", pChildNode->StringData));

                return returnStatus;
            }

            if( TRUE)
            {
                UCHAR                       pIPAddress[8] = {  0 };

                if( decodeIpAddr( pValue, AnscSizeOfString(pValue), pIPAddress) != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceWarning(("Invalid ip address string '%s'\n", pValue));

                    return ANSC_STATUS_FAILURE;
                }

                /* add the record */
                return
                    pSysIraIf->AddRecord2
                        (
                            pSysIraIf->hOwnerContext,
                            hSysFolder,
                            pName,
                            permission,
                            recordType,
                            &renderAttr,
                            pIPAddress,
                            sizeof(ULONG)
                        );
            }
        }
        else
        {
            returnStatus = AnscXmlDomNodeGetDataUlong(pChildNode, NULL, &longValue);

            if( returnStatus != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("Invalid ULONG value '%p'\n", pChildNode->StringData));

                return returnStatus;
            }

            /* add the record */
            return
                pSysIraIf->AddRecord2
                    (
                        pSysIraIf->hOwnerContext,
                        hSysFolder,
                        pName,
                        permission,
                        recordType,
                        &renderAttr,
                        (PVOID)&longValue,
                        sizeof(ULONG)
                    );
        }
    }
    else if( recordType == SYS_REP_RECORD_TYPE_BOOL)
    {
        returnStatus = AnscXmlDomNodeGetDataBoolean(pChildNode, NULL, &boolValue);

        if( returnStatus != ANSC_STATUS_SUCCESS)
        {
            CcspTraceWarning(("Invalid BOOL value '%p'\n", pChildNode->StringData));

            return returnStatus;
        }

        /* add the record */
        return
            pSysIraIf->AddRecord2
                (
                    pSysIraIf->hOwnerContext,
                    hSysFolder,
                    pName,
                    permission,
                    recordType,
                    &renderAttr,
                    (PVOID)&boolValue,
                    sizeof(BOOL)
                );

    }
    else if( recordType == SYS_REP_RECORD_TYPE_ASTR)
    {
        char *pRecordValue;

        /* AnscXmlDomNodeGetDataString() will return an error here (since
           target buffer pointer is NULL) but we rely on it still returning
           the length of the string.
        */
        length = 0;
        AnscXmlDomNodeGetDataString(pChildNode, NULL, NULL, &length);

        if( length > 0)
        {
            ULONG length2 = length + 1;

            pRecordValue = AnscAllocateMemory(length2);

            if( pRecordValue == NULL)
            {
                return ANSC_STATUS_RESOURCES;
            }

            length = length2;
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pRecordValue, &length);

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length >= length2))
            {
                CcspTraceWarning(("Failed to read string text value '%p'\n", pChildNode->StringData));

                AnscFreeMemory(pRecordValue);
                return returnStatus;
            }
        }
        else
        {
            pRecordValue = NULL;
            length       = 0;
        }

        /* add the record */
        returnStatus =
            pSysIraIf->AddRecord2
                (
                    pSysIraIf->hOwnerContext,
                    hSysFolder,
                    pName,
                    permission,
                    recordType,
                    &renderAttr,
                    (PVOID)pRecordValue,
                    length
                );

        if( pRecordValue != NULL)
        {
            AnscFreeMemory(pRecordValue);
        }

        return returnStatus;
    }
    else  /*  recordType == SYS_REP_RECORD_TYPE_BSTR */
    {
        if( contentType == SYS_RECORD_CONTENT_IP4_ADDR)
        {
            length = sizeof(pValue);
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pValue, &length);

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length == 0) || (length >= sizeof(pValue)))
            {
                CcspTraceWarning(("Failed to read string value '%p'\n", pChildNode->StringData));

                return returnStatus;
            }

            if( TRUE)
            {
                UCHAR                       pIPAddress[8] = {  0 };

                if( decodeIpAddr( pValue, AnscSizeOfString(pValue), pIPAddress) != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceWarning(("Invalid ip address string '%s'\n", pValue));

                    return ANSC_STATUS_FAILURE;
                }

                /* add the record */
                return
                    pSysIraIf->AddRecord2
                        (
                            pSysIraIf->hOwnerContext,
                            hSysFolder,
                            pName,
                            permission,
                            recordType,
                            &renderAttr,
                            pIPAddress,
                            sizeof(ULONG)
                        );
            }
        }
        else if( contentType == SYS_RECORD_CONTENT_CALENDAR_TIME)
        {
            length = sizeof(pValue);
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pValue, &length);

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length == 0) || (length >= sizeof(pValue)))
            {
                CcspTraceWarning(("Failed to read calendar value '%p'\n", pChildNode->StringData));

                return returnStatus;
            }

            if( TRUE)
            {
                ANSC_UNIVERSAL_TIME         time          = { 0 };


                decodeCalendar((PUCHAR) pValue, &time);

                /* add the record */
                return
                    pSysIraIf->AddRecord2
                        (
                            pSysIraIf->hOwnerContext,
                            hSysFolder,
                            pName,
                            permission,
                            recordType,
                            &renderAttr,
                            (PVOID)&time,
                            sizeof(ANSC_UNIVERSAL_TIME)
                        );
            }

        }
        else if( contentType == SYS_RECORD_CONTENT_SINT_LIST || contentType == SYS_RECORD_CONTENT_UINT_LIST)
        {
            length = sizeof(pValue);
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pValue, &length);

#if 0
            /* Fixme: return status from AnscXmlDomNodeGetDataString() is not checked... it's not clear why */

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length == 0) || (length >= sizeof(pValue)))
            {
                return returnStatus;
            }
#endif

            if( TRUE)
            {
                PULONG                              pUlongArray = NULL;
                ULONG                               uTotal      = 0;

                pUlongArray =
                    decodeIntList
                        (
                            pValue,
                            &uTotal
                        );

                if( pUlongArray != NULL)
                {
                    /* add the record */
                    returnStatus =
                        pSysIraIf->AddRecord2
                            (
                                pSysIraIf->hOwnerContext,
                                hSysFolder,
                                pName,
                                permission,
                                recordType,
                                &renderAttr,
                                (PVOID)(PUCHAR)pUlongArray,
                                uTotal * sizeof(ULONG)
                            );

                    AnscFreeMemory(pUlongArray);
                }
                else
                {
                    /* add the record */
                    returnStatus =
                        pSysIraIf->AddRecord2
                            (
                                pSysIraIf->hOwnerContext,
                                hSysFolder,
                                pName,
                                permission,
                                recordType,
                                &renderAttr,
                                NULL,
                                0
                            );
                }

                return returnStatus;
            }
        }
        else if( contentType == SYS_RECORD_CONTENT_IP4_ADDR_LIST)
        {
            length = sizeof(pValue);
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pValue, &length);

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length == 0) || (length >= sizeof(pValue)))
            {
                return returnStatus;
            }

            if( TRUE)
            {
                PUCHAR                                  pCharArray = NULL;

                pCharArray=
                    decodeIP4AddrList
                        (
                            (PVOID)pValue,
                            &length
                        );

                if( pCharArray != NULL)
                {
                    /* add the record */
                    returnStatus =
                        pSysIraIf->AddRecord2
                            (
                                pSysIraIf->hOwnerContext,
                                hSysFolder,
                                pName,
                                permission,
                                recordType,
                                &renderAttr,
                                (PVOID)pCharArray,
                                length
                            );

                    AnscFreeMemory(pCharArray);
                }
                else
                {
                    /* add the record */
                    returnStatus =
                        pSysIraIf->AddRecord2
                            (
                                pSysIraIf->hOwnerContext,
                                hSysFolder,
                                pName,
                                permission,
                                recordType,
                                &renderAttr,
                                NULL,
                                0
                            );
                }
            }

            return returnStatus;
        }
        else if( contentType == SYS_RECORD_CONTENT_MAC_ADDR)
        {
            length = sizeof(pValue);
            returnStatus = AnscXmlDomNodeGetDataString(pChildNode, NULL, pValue, &length);

            if ((returnStatus != ANSC_STATUS_SUCCESS) || (length == 0) || (length >= sizeof(pValue)))
            {
                return returnStatus;
            }

            returnStatus =
                decodeMacAddress
                        (
                            (PVOID)pValue,
                            &length
                        );

            if( returnStatus != ANSC_STATUS_SUCCESS)
            {
                return returnStatus;
            }

            /* add the record */
            returnStatus =
                pSysIraIf->AddRecord2
                    (
                        pSysIraIf->hOwnerContext,
                        hSysFolder,
                        pName,
                        permission,
                        recordType,
                        &renderAttr,
                        (PVOID)(PUCHAR)pValue,
                        length
                    );

            return returnStatus;
        }
        else
        {
            unsigned char *pRecordValue;

            length = 0;
            AnscXmlDomNodeGetDataBinary(pChildNode, NULL, NULL, &length);

            if( length != 0)
            {
                pRecordValue = AnscAllocateMemory(length + 1);

                if( pRecordValue == NULL)
                {
                    return ANSC_STATUS_RESOURCES;
                }

                returnStatus = AnscXmlDomNodeGetDataBinary(pChildNode, NULL, (PCHAR)pRecordValue, &length);

                if( returnStatus != ANSC_STATUS_SUCCESS)
                {
                    CcspTraceWarning(("Failed to read binary text value '%p'\n", pChildNode->StringData));
                    AnscFreeMemory(pRecordValue); /*10-May-2016 RDKB-5568 CID-33233, free pRecordValue before returning*/					

                    return returnStatus;
                }
            }
            else
            {
                pRecordValue = NULL;
                length       = 0;
            }

            /* add the record */
            returnStatus =
                pSysIraIf->AddRecord2
                    (
                        pSysIraIf->hOwnerContext,
                        hSysFolder,
                        pName,
                        permission,
                        recordType,
                        &renderAttr,
                        (PVOID)pRecordValue,
                        length
                    );

            if( pRecordValue != NULL)
            {
                AnscFreeMemory(pRecordValue);
            }
        }

        return returnStatus;
    }

    return returnStatus;
}

static
ANSC_STATUS
loadFolderFromXML
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ANSC_HANDLE                 hXMLFolder
    )
{
    PANSC_XML_DOM_NODE_OBJECT       pChildNode         = (PANSC_XML_DOM_NODE_OBJECT)hXMLFolder;
    PSYS_IRA_INTERFACE              pSysIraIf          = (PSYS_IRA_INTERFACE       )hSysIraIf;
    ANSC_HANDLE                     hChildFolder       = NULL;
    CHAR                            pName[128 + 1];
    ULONG                           length;
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    ULONG                           folderType         = SYS_REP_FOLDER_TYPE_STORAGE;
    ULONG                           permission         = DEFAULT_RFO_PERMISSION;
    ULONG                           contentType        = 0;
    SYS_RFO_RENDER_ATTR             renderAttr         = { 0 };

    /* get the name of the folder */
    length = sizeof(pName);
    if ((AnscXmlDomNodeGetAttrString(pChildNode,STR_NAME, pName, &length) != ANSC_STATUS_SUCCESS) || (length >= sizeof(pName)))
    {
        CcspTraceWarning(("Failed to get the 'name' attribute of this node.\n"));

        return ANSC_STATUS_FAILURE;
    }

    hChildFolder = (ANSC_HANDLE)
        pSysIraIf->AddFolder(pSysIraIf->hOwnerContext, hSysFolder, pName);

    if( hChildFolder == NULL)
    {
        CcspTraceWarning(("Failed to AddFolder '%s'\n", pName));

        return ANSC_STATUS_FAILURE;
    }

    /* get the folder type */
    length = sizeof(pName);
    if ((AnscXmlDomNodeGetAttrString(pChildNode,STR_TYPE, pName, &length) == ANSC_STATUS_SUCCESS) && (length < sizeof(pName)))
    {
        folderType = getFolderTypeFromString(pName);

        if( folderType != 0)
        {
            pSysIraIf->SetRfoFolderType(pSysIraIf, hChildFolder, folderType);
        }
    }

    /* get the contentType */
    length = sizeof(pName);
    if ((AnscXmlDomNodeGetAttrString(pChildNode,STR_CONTENT_TYPE, pName, &length) == ANSC_STATUS_SUCCESS) && (length < sizeof(pName)))
    {
        contentType = getFolderContentTypeFromString(pName);

        if( contentType != 0)
        {
            renderAttr.ContentType = contentType;

            pSysIraIf->SetRfoRenderAttr(pSysIraIf, hChildFolder, &renderAttr);
        }
    }

    /* get the permission */
    if( AnscXmlDomNodeGetAttrUlong(pChildNode, STR_ACCESS, &permission) == ANSC_STATUS_SUCCESS)
    {
        pSysIraIf->SetRfoPermission(pSysIraIf, hChildFolder, permission);
    }

    returnStatus =
        PsmSysFolderFromXMLHandle( hSysIraIf, hChildFolder, hXMLFolder);

    if( hChildFolder != NULL)
    {
        pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hChildFolder);
    }

    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysFolderToXMLHandle
            (
                ANSC_HANDLE                 hSysIraIf,
                ANSC_HANDLE                 hSysFolder,
                ANSC_HANDLE                 hXMLHandle
            );

    description:

        This function is called to write SysFolder to XML Handle.

    argument:   ANSC_HANDLE                 hSysIraIf
                The System IRA Interface.

                ANSC_HANDLE                 hSysFolder
                The system folder.

                ANSC_HANDLE                 hXMLHandle
                The XML Handle

    return:     the status of this operation

**********************************************************************/

ANSC_STATUS
PsmSysFolderToXMLHandle
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ANSC_HANDLE                 hXMLHandle
    )
{
    //CcspTraceInfo(("PsmSysFolderToXMLHandle begins '\n"));  
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PSYS_IRA_INTERFACE              pSysIraIf          = (PSYS_IRA_INTERFACE       )hSysIraIf;
    PANSC_XML_DOM_NODE_OBJECT       pXMLRoot           = (PANSC_XML_DOM_NODE_OBJECT)hXMLHandle;
    PANSC_XML_DOM_NODE_OBJECT       pChildNode         = NULL;
    ULONG                           uSubTotal          = 0;
    ULONG                           uRecTotal          = 0;
    ULONG                           uPermit            = 0;
    ULONG                           i                  = 0;
    ULONG                           contentType        = 0;
    ANSC_HANDLE                     hChildFolder       = NULL;
    ULONG                           nameSize           = SYS_MAX_RECORD_NAME_SIZE;
    CHAR                            pName[SYS_MAX_RECORD_NAME_SIZE];

    returnStatus =
        pSysIraIf->QueryFolder
            (
                pSysIraIf->hOwnerContext,
                hSysFolder,
                NULL,               /* time stamp */
                &uPermit,
                NULL,               /* folder type */
                &uSubTotal,
                &uRecTotal,
                &contentType,       /* content type */
                NULL                /* render  attr */
            );

    if( returnStatus != ANSC_STATUS_SUCCESS)
    {
        return returnStatus;
    }

    if( uPermit != DEFAULT_RFO_PERMISSION)
    {
        AnscXmlDomNodeSetAttrUlong
            (
                pXMLRoot,
                STR_ACCESS,
                uPermit
            );
    }

    if( contentType != SYS_FOLDER_CONTENT_DEFAULT)
    {
        AnscXmlDomNodeSetAttrString
            (
                pXMLRoot,
                STR_CONTENT_TYPE,
                ppFolderContentType[contentType - 1],
                AnscSizeOfString(ppFolderContentType[contentType - 1])
            );
    }

    /* write the record one by one */
    for ( i = 0; i < uRecTotal; i ++)
    {
#if 0
        addRecordToXMLHandle
#else
        addRecordToXMLHandle2
#endif
            (
                hSysIraIf,
                hSysFolder,
                i,
                pXMLRoot
            );
    }

    /* write the sub folder one by one */
    for( i = 0; i < uSubTotal; i ++)
    {
        AnscZeroMemory(pName, SYS_MAX_RECORD_NAME_SIZE);
        nameSize = SYS_MAX_RECORD_NAME_SIZE;

        if( ANSC_STATUS_SUCCESS ==
            pSysIraIf->EnumSubFolder
                (
                    pSysIraIf->hOwnerContext,
                    hSysFolder,
                    i,
                    pName,
                    &nameSize
                ))
        {
            pChildNode = (PANSC_XML_DOM_NODE_OBJECT)AnscCreateXmlDomNode(NULL);

            if( pChildNode == NULL)
            {
                return ANSC_STATUS_RESOURCES;
            }

            AnscXmlDomNodeSetName(pChildNode, STR_FOLDER);
            AnscXmlDomNodeAddChild(pXMLRoot, pChildNode);

            AnscXmlDomNodeSetAttrString
                (
                    pChildNode,
                    STR_NAME,
                    pName,
                    AnscSizeOfString(pName)
                );

            /* open the child folder */
            hChildFolder =
                pSysIraIf->OpenFolder
                    (
                        pSysIraIf->hOwnerContext,
                        hSysFolder,
                        pName
                    );

            if( hChildFolder != NULL)
            {
                returnStatus =
                    PsmSysFolderToXMLHandle
                        (
                            hSysIraIf,
                            hChildFolder,
                            (ANSC_HANDLE)pChildNode
                        );

               pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hChildFolder);
            }
            else
            {
                CcspTraceWarning(("Failed to open folder '%s'\n", pName));
            }

            if( returnStatus != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("Failed to write folder '%s' to XML handle.\n", pName));

                return returnStatus;
            }
        }
        else
        {
            CcspTraceWarning(("Failed to EnumSubFolder - %lu\n", i));
        }
    }
    //CcspTraceInfo(("PsmSysFolderToXMLHandle ends '\n"));

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysFolderFromXMLHandle
            (
                ANSC_HANDLE                 hSysIraIf,
                ANSC_HANDLE                 hSysFolder,
                ANSC_HANDLE                 hXMLHandle
            );

    description:

        This function is called to load the content of the sys folder
        from XML handle.

    argument:   ANSC_HANDLE                 hSysIraIf
                The System IRA Interface.

                ANSC_HANDLE                 hSysFolder
                Current System Folder.

                ANSC_HANDLE                 hXMLHandle
                The XML Handle.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysFolderFromXMLHandle
    (
        ANSC_HANDLE                 hSysIraIf,
        ANSC_HANDLE                 hSysFolder,
        ANSC_HANDLE                 hXMLHandle
    )
{
    //CcspTraceInfo(("PsmSysFolderFromXMLHandle begins '\n"));  
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PANSC_XML_DOM_NODE_OBJECT       pXMLRoot           = (PANSC_XML_DOM_NODE_OBJECT)hXMLHandle;
    PCHAR                           pNodeName          = NULL;
    PANSC_XML_DOM_NODE_OBJECT       pChildNode         = NULL;
    errno_t                         rc                 = -1;
    int                             ind                = -1;

    if( hSysFolder == NULL || hXMLHandle == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    /* get the child node one by one */
    pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
        AnscXmlDomNodeGetHeadChild(pXMLRoot);

    while(pChildNode != NULL)
    {
        /* check the node name */
        pNodeName = AnscXmlDomNodeGetName(pChildNode);

        rc = strcmp_s(STR_FOLDER, strlen(STR_FOLDER), pNodeName, &ind);
	ERR_CHK(rc);
        if((rc == EOK) && (!ind))
        {
            /* load it as a folder */
            returnStatus =
                loadFolderFromXML
                    (
                        hSysIraIf,
                        hSysFolder,
                        (ANSC_HANDLE)pChildNode
                    );
        }
	else
	{
             rc = strcmp_s(STR_RECORD, strlen(STR_RECORD), pNodeName, &ind);
             ERR_CHK(rc);
	     if((rc == EOK) && (!ind))
             {
            /* load it as a record */
            returnStatus =
                loadRecordFromXML
                    (
                        hSysIraIf,
                        hSysFolder,
                        (ANSC_HANDLE)pChildNode
                    );
             }
             else
             {
                  CcspTraceInfo(("Unknown child node name '%s', ignored.\n", pNodeName));

                  continue;
             }
	}

        if( returnStatus != ANSC_STATUS_SUCCESS)
        {
            return returnStatus;
        }

        /* handle the next one */
        pChildNode = (PANSC_XML_DOM_NODE_OBJECT)
            AnscXmlDomNodeGetNextChild(pXMLRoot, pChildNode);
    }
    //CcspTraceInfo(("PsmSysFolderFromXMLHandle ends '\n"));  
    return  returnStatus;
}
