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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include <ansc_platform.h>
#include <ccsp_base_api.h>
#include <ccsp_memory.h>
#include <ccsp_custom.h>

#include "ssp_global.h"
#include "ssp_hash.h"
#include "ssp_utils.h"

/*
    This is the hash table for psm record.
*/
PSM_INFO_RECORD  PsmHashTable[PSM_HASH_TABLE_LEN] = {{0}};

/*
    This is the hash table for psm record.
*/
PSM_INFO_RECORD  PsmRecordCache[PSM_RECORD_ITEM_NUM] = {{0}};
PPSM_INFO_RECORD PsmRecordCacheHeader = NULL;

/*
    The  config will be set only in default file
*/
unsigned char     bPsmPreconfiguredNextLveel = 1;

/*We're better to make a RW lock in the future to speed the multiple read*/
ANSC_LOCK         PsmAccessAccessLock;

/*This group tables only for getParameterNames with nextlevel(true/false)
    The feature PreconfiguredNextLevel feature will speed this before this group tables.
    
    For all prefix with nextlevel(true) under dmsb.l2net., 
        dmsb.l3net.,
        dmsb.EthLink.,
        eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.,
        eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.
    They may be preconfigured in default config file.
    
    So just need to get that directly.
*/
PSM_GROUP_TABLE psmGroupTable[]=
    {
        {{0}, "dmsb.firewall."},
        {{0}, "dmsb.dhcpv4."},
        {{0}, "Device.NAT."},
        {{0}, "eRT.com.cisco.spvtg.ccsp.tr069pa."},
        {{0}, "dmsb.cisco.gre."},
        {{0}, "dmsb.truestaticip."},
        {{0}, "dmsb.l2net."},  //Not used when PreconfiguredNextLevel is enabled.
        {{0}, "dmsb.l3net."},  //Not used when PreconfiguredNextLevel is enabled 
        {{0}, "dmsb.EthLink."}, //Not used when PreconfiguredNextLevel is enabled
        {{0}, "eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi."}, //Not used when PreconfiguredNextLevel is enabled 
        {{0}, NULL}  //This is for default parameter chains
    };

int PsmHashInit()
{
    int i = 0;
    
    AnscInitializeLock(&PsmAccessAccessLock);
    
    memset(PsmHashTable, 0, sizeof(PsmHashTable));
    memset(PsmRecordCache, 0, sizeof(PsmRecordCache));

    while(psmGroupTable[i].pGroupName ){
        memset((void *)&psmGroupTable[i].GroupHeader, 0, sizeof(psmGroupTable[i].GroupHeader));
        i++;
    }
    memset((void *)&psmGroupTable[i].GroupHeader, 0, sizeof(psmGroupTable[i].GroupHeader));

    PsmRecordCacheHeader = &PsmRecordCache[0];
    for(i=0; i<PSM_RECORD_ITEM_NUM-1; i++){
        PsmRecordCache[i].pHashNext = &PsmRecordCache[i+1];
    }
    PsmRecordCache[i].pHashNext = NULL;
        
    return 0;
}

int PsmHashRecordFreeString(char * pStr)
{
    if (pStr == NULL ){
        return 0;
    }
    
    if ( ( (unsigned long )pStr >= (unsigned long )pPsmDefFileBuffer ) && (  (unsigned long )pStr <= (unsigned long )(pPsmDefFileBuffer+PsmDefFileLen) ) ){
        //This memory is in file memory range. Not free.
    }else if ( ( (unsigned long )pStr >= (unsigned long )pPsmCurFileBuffer ) && (  (unsigned long )pStr <= (unsigned long )(pPsmCurFileBuffer+PsmCurFileLen) ) ){
        //This memory is in file memory range. Not free.
    }else{
        AnscFreeMemory(pStr);
    }
    
    return 0;
}

PPSM_INFO_RECORD PsmHashRecordCachePop()
{
    PPSM_INFO_RECORD pRecord = NULL;
    int              i       = 0;

    if (PsmRecordCacheHeader == NULL ){
        pRecord = (PPSM_INFO_RECORD)AnscAllocateMemory(sizeof(PSM_INFO_RECORD)* PSM_RECORD_ADD_NUM );
        if ( pRecord == NULL ) return NULL;
        
        memset(pRecord, 0, sizeof(PSM_INFO_RECORD)* PSM_RECORD_ADD_NUM );
        
        PsmRecordCacheHeader = &pRecord[0];
        for(i=0; i<PSM_RECORD_ADD_NUM-1; i++){
            pRecord[i].pHashNext = &pRecord[i+1];
        }
        pRecord[i].pHashNext = NULL;
    }    

    if (PsmRecordCacheHeader){
        pRecord = PsmRecordCacheHeader;
        PsmRecordCacheHeader = pRecord->pHashNext;
    }
    
    return pRecord;
}


int PsmHashRecordClearContent(PPSM_INFO_RECORD pRecord)
{
    if(pRecord->pName){
        PsmHashRecordFreeString(pRecord->pName);
        pRecord->pName = NULL;
    }
    
    if(pRecord->pType){
        PsmHashRecordFreeString(pRecord->pType);
        pRecord->pType = NULL;
    }
    
    if(pRecord->pContentType){
        PsmHashRecordFreeString(pRecord->pContentType);
        pRecord->pContentType = NULL;
    }    

    if(pRecord->pCurVal){
        PsmHashRecordFreeString(pRecord->pCurVal);
        pRecord->pCurVal = NULL;
    }    

    if(pRecord->pDefVal){
        PsmHashRecordFreeString(pRecord->pDefVal);
        pRecord->pDefVal = NULL;
    }    
    
    return 0;
}

int PsmHashRecordCachePush(PPSM_INFO_RECORD pRecord)
{
    PsmHashRecordClearContent(pRecord);
    memset(pRecord, 0, sizeof(PSM_INFO_RECORD));

    pRecord->pHashNext = PsmRecordCacheHeader;
    PsmRecordCacheHeader = pRecord;

    return 0;
}

PPSM_INFO_RECORD PsmHashRecordFind(char *pName)
{
    PPSM_INFO_RECORD pRecord = NULL;
    unsigned int     hashIdx = 0;
    
    if ( pName == NULL || pName[0] == '\0' ){
        CcspTraceError(("%s -- record find error.\n", __FUNCTION__));
        return 0;
    }
    
    hashIdx = AnscHashString(pName, strlen(pName), PSM_HASH_TABLE_LEN);

    pRecord = PsmHashTable[hashIdx].pHashNext;
    while(pRecord){
        if (strcmp(pRecord->pName, pName) == 0 )
            break;
        else{
            pRecord = pRecord->pHashNext;
        }
    }

    return pRecord;

}

int PsmHashRecordInsertHashTable(PPSM_INFO_RECORD pRecord)
{
    unsigned int hashIdx = 0;
    if ( pRecord->pName == NULL || pRecord->pName[0] == '\0' ){
        CcspTraceError(("%s -- record Insert error.\n", __FUNCTION__));
        return 0;
    }
    
    hashIdx = AnscHashString(pRecord->pName, strlen(pRecord->pName), PSM_HASH_TABLE_LEN);

    pRecord->pHashNext = NULL;
    pRecord->pHashPre = NULL;
    
    if( PsmHashTable[hashIdx].pHashNext ){
        pRecord->pHashNext = PsmHashTable[hashIdx].pHashNext ;
        pRecord->pHashNext->pHashPre = pRecord;

    }

    pRecord->pHashPre = &PsmHashTable[hashIdx];
    PsmHashTable[hashIdx].pHashNext = pRecord;
    
    return 0;
}

int PsmHashRecordInsertGroupTable(PPSM_INFO_RECORD pRecord)
{
    int               i                 = 0;
    PSM_INFO_RECORD   GroupHeader       = {0};
    char             *pGroupName        = NULL;
    
    while(psmGroupTable[i].pGroupName){
        if ( strncmp(pRecord->pName, psmGroupTable[i].pGroupName, strlen(psmGroupTable[i].pGroupName) )== 0 )
            break;

        i++;
    }

    pRecord->pGroupNext  = NULL;
    pRecord->pGroupPre   = NULL;

    if( psmGroupTable[i].GroupHeader.pGroupNext ){
        pRecord->pGroupNext = psmGroupTable[i].GroupHeader.pGroupNext;
        pRecord->pGroupNext->pGroupPre = pRecord;
    }

    pRecord->pGroupPre = &psmGroupTable[i].GroupHeader;
    psmGroupTable[i].GroupHeader.pGroupNext = pRecord;

    /*We switch this group to first place in order to speed next finding*/
    if ( (i!= 0) && psmGroupTable[i].pGroupName){
        GroupHeader = psmGroupTable[i].GroupHeader;
        pGroupName   = psmGroupTable[i].pGroupName;
        psmGroupTable[i].GroupHeader    = psmGroupTable[0].GroupHeader;
        psmGroupTable[i].pGroupName     = psmGroupTable[0].pGroupName;
        psmGroupTable[0].GroupHeader    = GroupHeader;
        psmGroupTable[0].pGroupName     = pGroupName;
    }
    
    return 0;
}

/*
    All input memory should be malloced in advance.
    This function will use them directly.
*/
int PsmHashRecordAddNewRecord(PPSM_INFO_RECORD pRecord)
{
    PsmHashRecordInsertGroupTable(pRecord);
    PsmHashRecordInsertHashTable(pRecord);
 
    return 0;
}

/*
    Unlink it from hash table.
*/
int PsmHashRecordUnlinkHashTable(PPSM_INFO_RECORD pRecord)
{
    
    if ( pRecord->pHashNext ){ 
        pRecord->pHashNext->pHashPre = pRecord->pHashPre;
    }

    pRecord->pHashPre->pHashNext = pRecord->pHashNext;
    
    return 0;
}

/*
    Unlink it from group table.
*/
int PsmHashRecordUnlinkGroupTable(PPSM_INFO_RECORD pRecord)
{
    if ( pRecord->pGroupNext ){ 
        pRecord->pGroupNext->pGroupPre = pRecord->pGroupPre;
    }
    
    pRecord->pGroupPre->pGroupNext = pRecord->pGroupNext;

    return 0;
}

/*
    All input memory should be malloced in advance.
    This function will use them directly.
*/
int PsmHashRecordDelRecord(PPSM_INFO_RECORD pRecord)
{
    PsmHashRecordUnlinkHashTable(pRecord);
    PsmHashRecordUnlinkGroupTable(pRecord);
    
    PsmHashRecordCachePush(pRecord);
    return 0;
}

/*
    Set new values(two cases: New user added value, changed value).
*/
int PsmHashRecordSetRecord(char * pName, char * pType, char *pValue)
{
    PPSM_INFO_RECORD pRecord     = NULL;

    pRecord = PsmHashRecordFind(pName);
    if ( pRecord ){
        if (pRecord->pDefVal && (strcmp(pRecord->pDefVal, pValue) == 0 )){
            if ( pRecord->pCurVal ){
                PsmHashRecordFreeString(pRecord->pCurVal);
                pRecord->pCurVal = NULL;
            }
        }else{
            if (pRecord->pCurVal && strlen(pRecord->pCurVal) >= strlen(pValue) ){
                strcpy(pRecord->pCurVal, pValue);
            }else{
                if ( pRecord->pCurVal ){
                    PsmHashRecordFreeString(pRecord->pCurVal);
                    pRecord->pCurVal = NULL;
                }
                pRecord->pCurVal        =  AnscAllocateMemory(strlen(pValue)+1);
                if( pRecord->pCurVal == NULL ){
                    CcspTraceError(("%s -- record allocation error (%d).\n", __FUNCTION__, __LINE__));
                }else{
                    strcpy(pRecord->pCurVal, pValue);
                }
            }
        }
        return 0;
    }else {
        pRecord = PsmHashRecordCachePop();
        if ( pRecord == NULL ){
            CcspTraceError(("%s -- record allocation error (%d).\n", __FUNCTION__, __LINE__));
            return 0;
        }
        
        pRecord->pName          = AnscAllocateMemory(strlen(pName)+1);
        if ( pRecord->pName == NULL ) goto FAILURE;
        strcpy(pRecord->pName, pName);
        
        pRecord->pCurVal        =  AnscAllocateMemory(strlen(pValue)+1);
        if ( pRecord->pCurVal == NULL ) goto FAILURE;
        strcpy(pRecord->pCurVal, pValue);
        
        pRecord->pType          = AnscAllocateMemory(strlen(pType)+1);;
        if ( pRecord->pType == NULL ) goto FAILURE;
        strcpy(pRecord->pType, pType);
        
        pRecord->pContentType   = NULL;

        PsmHashRecordAddNewRecord(pRecord);
        return 0;
        
FAILURE:
        CcspTraceError(("%s -- record allocation error (%d).\n", __FUNCTION__, __LINE__));
        if(pRecord->pCurVal == NULL )
            AnscFreeMemory(pRecord->pCurVal);
        if(pRecord->pName == NULL )
            AnscFreeMemory(pRecord->pName);
        if(pRecord)
            AnscFreeMemory(pRecord);
    }    
    
    return 0;
}


/*
    Remove values/record from hashTable for dev/cur files
*/
int PsmHashUnloadFile(PSM_FILE_TYPE eType)
{
    int              i           = 0;
    PPSM_INFO_RECORD pRecordNext = NULL;
    PPSM_INFO_RECORD pRecord     = NULL;

    do{
        pRecordNext = psmGroupTable[i].GroupHeader.pGroupNext;
        
        while(pRecordNext){
            pRecord      =  pRecordNext;
            pRecordNext  =  pRecordNext->pGroupNext;

            //unlink it in HashTable
            if ( ( eType == PSM_FILETYPE_DEF || eType == PSM_FILETYPE_FULL ) && pRecord->pDefVal ){
                PsmHashRecordFreeString(pRecord->pDefVal);
                pRecord->pDefVal = NULL;
            }

            //unlink it in Group Table
            if ( ( eType == PSM_FILETYPE_CUR ||  eType == PSM_FILETYPE_FULL ) && pRecord->pCurVal ){
                PsmHashRecordFreeString(pRecord->pCurVal);
                pRecord->pCurVal = NULL;
            }

            if ( !pRecord->pDefVal && !pRecord->pCurVal ){
                PsmHashRecordDelRecord(pRecord);
            }

        }

        i++;
    }while(psmGroupTable[i].pGroupName);
    
    return 0;
}

/*
    Decode file and add them into hash table.
    We only support existing config XML format, Not support more XML official format.
    
    If one record format is not right, rollback all content and return 1.

    If all is good, return 0.

    eType only accepts PSM_FILETYPE_DEF and PSM_FILETYPE_CUR.
    
    return value: 0  ---- file format is good, file content is decoded 
                  1  ---- file format is not good and file is not used.


*/
int  PsmHashLoadFile(PSM_FILE_TYPE eType, char *buf, int length)
{
    PPSM_INFO_RECORD pRecord            = NULL;
    PSM_PARSING_LINE_RESULT Result      = PSM_LINE_RECORD;
    unsigned int     bCommentBegin      = 0;
    unsigned int     NextLine           = PSM_LINE_XMLHEADER;
    char            *pNextLine          = NULL;
    char            *pName              = NULL;
    char            *pValue             = NULL;
    char            *pType              = NULL;
    char            *pContentType       = NULL;
    int              len                = 0;
    
    pNextLine = buf;
    
    do {
        Result = PsmHashParseOneLine(pNextLine, &pName, &pValue, &pType, &pContentType, &pNextLine);

        //Remove this. TBD
        //CcspTraceInfo(("%s ---- Result:%d, pName:%s, pValue:%s, pType:%s, pContentType:%s, pNextLine:%d \n", __FUNCTION__, Result, pName, pValue, pType, pContentType, (unsigned int)pNextLine));

        //Skip all comment line -- begin
        if(Result == PSM_LINE_COMMENTEND){
            if( bCommentBegin == 1 ){
                bCommentBegin = 0;
                continue;
            }else{
                goto ERROR;
            }
        }if (bCommentBegin){
            continue;
        }

        if(Result == PSM_LINE_COMMENTBEGIN){
            bCommentBegin = 1;
            continue;
        }
        //Skip all commented line -- end

        switch(Result){
            case PSM_LINE_XMLHEADER :
                if ( NextLine == PSM_LINE_XMLHEADER ){
                    NextLine = PSM_LINE_PROVISIONBEGIN;
                }else{
                    goto ERROR;
                }
                break;
            case PSM_LINE_PROVISIONBEGIN :
                if ( NextLine == PSM_LINE_PROVISIONBEGIN ){
                    NextLine = PSM_LINE_RECORD;
                }else{
                    goto ERROR;
                }
                break;
            case PSM_LINE_RECORD :
                if ( NextLine == PSM_LINE_RECORD ){
                    /*Translate pValue of XML content*/    
                    len = strlen(pValue);
                    AnscXmlRemoveCharReference(pValue, &len );

                    pRecord = PsmHashRecordFind(pName);
                    if ( pRecord ){
                        if ( eType == PSM_FILETYPE_DEF ){
                            //This case should never happen.
                        }else{
                            if (pRecord->pDefVal && (strcmp(pRecord->pDefVal, pValue) == 0 )){
                                if ( pRecord->pCurVal ){
                                    //This case should never happen
                                    PsmHashRecordFreeString(pRecord->pCurVal);
                                    pRecord->pCurVal = NULL;
                                }
                            }else{
                                if (pRecord->pCurVal && strlen(pRecord->pCurVal) >= strlen(pValue) ){
                                    //This case should never happen
                                    strcpy(pRecord->pCurVal, pValue);
                                }else{
                                    PsmHashRecordFreeString(pRecord->pCurVal);
                                    pRecord->pCurVal    = pValue;
                                }
                            }
                        }
                    }else {
                        pRecord = PsmHashRecordCachePop();
                        if ( pRecord == NULL ){
                            CcspTraceError(("%s -- record allocation error.\n", __FUNCTION__));
                            goto ERROR;
                        }
                        pRecord->pName          = pName;
                        pRecord->pType          = pType;
                        pRecord->pContentType   = pContentType;
                        if ( eType == PSM_FILETYPE_DEF ){
                            pRecord->pDefVal    = pValue;
                        }else{
                            pRecord->pCurVal    = pValue;
                        }
                        PsmHashRecordAddNewRecord(pRecord);
                    }
                    
                }
                
                break;
            case PSM_LINE_PROVISIONEND :
                if ( NextLine == PSM_LINE_RECORD ){
                    NextLine = PSM_LINE_FILEEND;
                }else{
                    goto ERROR;
                }
                break;
            case PSM_LINE_FILEEND :
                if ( NextLine != PSM_LINE_FILEEND ){
                    goto ERROR;
                }
                break;
            case PSM_LINE_BAD :
                goto ERROR;
            case PSM_LINE_EMPTY :
                break;
            case PSM_LINE_COMMENTBEGINEND :
                break;
            default :
                goto ERROR;
        }

    }while(Result != PSM_LINE_FILEEND );

    return 0;

ERROR:
    PsmHashUnloadFile(eType);
    CcspTraceError(("%s(%d) -- file parsing error(Result:%d) and Rollback.\n", __FUNCTION__, eType, Result));

    return 1;    
}

/*
    Save into current file in flash.
    bUserChanged  --- If it's true, only save user changed content into that file
                                This should be default behavior.
                      If it's false, save all content into that file.
                                This is for old format.
*/
int PsmHashPeriodicSaving(unsigned int bUserChanged, char * filepath)
{
    int              i           = 0;
    PPSM_INFO_RECORD pRecord     = NULL;
    FILE            *fp          = fopen(filepath, "w"); 
    char            *p           = NULL;
    int              len         = 0;

    if (NULL == fp) {
        CcspTraceError(("%s(%d, %s) -- file saving error.\n", __FUNCTION__, bUserChanged, filepath));
        return 0;
    }

    //TBD Remove this.
    //CcspTraceInfo(("%s(%d, %s) -- file saving.\n", __FUNCTION__, bUserChanged, filepath));

    //Write file header
    fprintf(fp,"<?xml version=\"1.0\"  encoding=\"UTF-8\" ?>\n");
    fprintf(fp,"<Provision Version=\"%d\">\n", CCSP_PSM_FORMAT_VERSION);

    do{
        pRecord = psmGroupTable[i].GroupHeader.pGroupNext;
        
        while(pRecord){
            if ( pRecord->pCurVal || !bUserChanged ){
                if (pRecord->pName){
                    fprintf(fp,"  <Record name=\"%s\"",pRecord->pName);
                }
            
                if(pRecord->pType){
                    fprintf(fp," type=\"%s\"",pRecord->pType);
                }

                if(pRecord->pContentType){
                    fprintf(fp," contentType=\"%s\"",pRecord->pContentType);
                }

                if (pRecord->pCurVal){
                    p = pRecord->pCurVal;
                    //fprintf(fp,">%s</Record>\n",pRecord->pCurVal);
                }else if (!bUserChanged){
                    p= pRecord->pDefVal;
                    //fprintf(fp,">%s</Record>\n",pRecord->pDefVal);
                }else{
                    p = NULL;
                }

                /*Translate the special character*/
                if ( p && p[0] ){
                    len  = strlen(p);
                    p    = AnscXmlEscapeChars(p, len, &len);
                }else{
                    len = 0;
                    p   = NULL;
                }
                
                if (p && p[0] ){
                    fprintf(fp," >%s</Record>\n",p);
                    AnscFreeMemory(p);
                    p = NULL; len = 0;
                }else{
                    fprintf(fp," />\n");
                }

            }
            
            pRecord  =  pRecord->pGroupNext;
        }

    }while(psmGroupTable[i++].pGroupName);

    //Write the file end
    fprintf(fp,"</Provision>\n");
    fclose(fp);
    
    return 0;
}

