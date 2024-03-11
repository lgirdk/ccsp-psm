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
    Read file into ppBuf and pLen
    If fail, return 1
    Or else, return 0.
*/
ANSC_STATUS PsmReadFile(char * path, char **ppBuf, int *pLen)
{
    ANSC_HANDLE pFile = NULL;

    *ppBuf = NULL;
    *pLen = 0;

    CcspTraceError(("%s: start read file %s\n", __FUNCTION__, path));

    if ((pFile = AnscOpenFile((char *)path, ANSC_FILE_MODE_READ, ANSC_FILE_TYPE_RDWR)) == NULL){
        CcspTraceError(("%s: file is not existing, we keep this as true.\n", __FUNCTION__));
        return 0;
    }

    if ((*pLen = AnscGetFileSize(pFile)) < 0)
        goto errout;

    if ((*ppBuf = AnscAllocateMemory(*pLen)) == NULL)
        goto errout;

    if (AnscReadFile(pFile, *ppBuf, (unsigned long *)pLen) != ANSC_STATUS_SUCCESS)
        goto errout;

    AnscCloseFile(pFile);

    CcspTraceError(("%s: read %s succeed\n", __FUNCTION__, path));
    return 0;

errout:
    CcspTraceError(("%s: read %s failed\n", __FUNCTION__, path));
    if (pFile)
        AnscCloseFile(pFile);

    if (*ppBuf)
    {
        AnscFreeMemory(*ppBuf);
        *ppBuf = NULL;
    }
    *pLen = 0;

    return 1;
}

#define    PSM_CFG_TYPE_default_WIFI     3

/* This function is used to get the configuration size before reading 
 * it into the memory by calling Read Current/Default Config APIs
 */
int  PSM_Get_Config_Size ( unsigned int uConfigType )
{
#if defined(_COSA_INTEL_USG_ARM_)
	int ret = 0;

	switch(uConfigType) {
	
	case PSM_CFG_TYPE_default_WIFI:
		ret =  1+(10*MAX_COUNT_SA_VAPS);
		break;

	default:
		break;
	}

    return ret;
#else
    return 0;
#endif
}

/*
    Parse one line and return result.
    The output values are meaningful only when PSM_LINE_RECORD is returned.

    PSM_LINE_XMLHEADER =1,
    PSM_LINE_PROVISIONBEGIN,
    PSM_LINE_RECORD,
    PSM_LINE_PROVISIONEND,
    PSM_LINE_FILEEND,
    PSM_LINE_EMPTY,
    PSM_LINE_COMMENTBEGIN,
    PSM_LINE_COMMENTEND,
    PSM_LINE_COMMENTBEGINEND,
    PSM_LINE_BAD

Only the following possible cases are supported:
<?xml version="1.0"  encoding="UTF-8" ?>
<Provision>
  <Record name="com.cisco.spvtg.ccsp.tr069pa.PmNotif.Device.ManagementServer.ConnectionRequestURL" type="astr" contentType="ip4Addr">2</Record>
  <Record name="eRT.com.cisco.spvtg.ccsp.tr069pa.PmNotif.Device.ManagementServer.ConnectionRequestURL" type="astr" contentType="ip4Addr">2</Record>
  <!-- LME guest 13 end -->
  
  <!-- LME(5) guest 14-->
  <Record name="dmsb.l2net.1.Members.Gre" type="astr"> </Record>
  <Record name="dmsb.l2net.1.Vlan.2.PrimaryVlan" type="astr" />
  <!--
  <Record name="dmsb.l2net.1.Members.WiFi" type="astr">ath0 ath1</Record>
    -->
  <Record name="dmsb.l3net.19.V6Addr" type="astr"></Record>
  <Record name="dmsb.snmp.ifindexMap" type="astr">114:ath5,34:ath4,32:ath0,112:ath1,33:ath2,35:ath6,113:ath3</Record>
</Provision>

*/

/*Skip any bad words in this line*/
#define PSM_JUMP_NEXTLINE   { \
while( (pLine[idx] != '\n') && (pLine[idx] != '\0') ) idx++; \
if(pLine[idx] == '\n' ) idx++; \
*ppNextLine = &pLine[idx];   }\

PSM_PARSING_LINE_RESULT PsmHashParseOneLine(char *pLine, char **ppName, char **ppValue, char **ppType, char **ppContentType, char **ppNextLine)
{
    int     idx = 0;
    int     i   = 0;

    if ( pLine == NULL ) return PSM_LINE_FILEEND;
    
    *ppName=*ppValue=*ppType=*ppContentType=*ppNextLine=NULL;

    /* <Record name="dmsb.l2net.1.Members.WiFi" type="astr">ath0 ath1</Record> */

    while( (pLine[idx]!='\n') && (pLine[idx]!='\0') && ( pLine[idx] != '<' )) idx++;

    /* For empty pLine or "-->" pLine   */
    if (pLine[idx] == '\n' ){
        pLine[idx] = '\0';
        if( strstr(pLine, "-->") ){
            pLine[idx] = '\n';
            PSM_JUMP_NEXTLINE
            return PSM_LINE_COMMENTEND;
        }else{
            /*We also take Non-"<" line as empty */
            pLine[idx] = '\n';
            PSM_JUMP_NEXTLINE
            return PSM_LINE_EMPTY;
        }
    }else if (pLine[idx] == '\0' ){
        return PSM_LINE_FILEEND;
    }

    idx++;
    /* Record name="dmsb.l2net.1.Members.WiFi" type="astr" contentType="ip4Addr" >ath0 ath1</Record>*/
    while( (pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
    if(strncmp( &pLine[idx], "Record", 6) == 0 ){
        idx += 6;
        while((pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
        
        /*name="dmsb.l2net.1.Members.WiFi" type="astr" contentType="ip4Addr" >ath0 ath1</Record>*/
        if(strncmp( &pLine[idx], "name=\"", 6) == 0 ){
            idx +=6;
            *ppName = &pLine[idx];

            while( (pLine[idx]!='"') && (pLine[idx]!='\0') && (pLine[idx]!='\n') ) idx++;
            if(pLine[idx]=='"') pLine[idx] = '\0'; else goto EXIT;

            idx++;
            
            /* type="astr" contentType="ip4Addr" >ath0 ath1</Record>*/            
            while((pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
            if(strncmp( &pLine[idx], "type=\"", 6) == 0 ){
                idx +=6;
                *ppType = &pLine[idx];
                
                while( (pLine[idx]!='"') && (pLine[idx]!='\0') && (pLine[idx]!='\n') ) idx++;
                if(pLine[idx]=='"') pLine[idx] = '\0'; else goto EXIT;

                idx++;
            }
            
            /* contentType="ip4Addr" >ath0 ath1</Record>*/
            while((pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
            if(strncmp( &pLine[idx], "contentType=\"", 13) == 0 ){
                idx +=13;
                *ppContentType = &pLine[idx];
                
                while( (pLine[idx]!='"') && (pLine[idx]!='\0') && (pLine[idx]!='\n') ) idx++;
                if(pLine[idx]=='"') pLine[idx] = '\0'; else goto EXIT;
            
                idx++;
            }

            /* contentType="ip4Addr" >ath0 ath1</Record>
            while((pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
            if(strncmp( &pLine[idx], "version=\"", 9) == 0 ){
                idx +=9;
                *ppVersion = &pLine[idx];
                
                while( (pLine[idx]!='"') && (pLine[idx]!='\0') && (pLine[idx]!='\n') ) idx++;
                if(pLine[idx]=='"') pLine[idx] = '\0'; else goto EXIT;
            
                idx++;
            }*/

            /* >ath0 ath1</Record>  or />*/
            while((pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
            if (strncmp( &pLine[idx], "/>", 2) == 0 ){
                *ppValue = &pLine[idx];
                pLine[idx] = '\0';
                idx++;
                PSM_JUMP_NEXTLINE
            }else if(strncmp( &pLine[idx], ">", 1) == 0 ){
                idx +=1;
                *ppValue = &pLine[idx];
                
                while( (pLine[idx]!='<') && (pLine[idx]!='\0') && (pLine[idx]!='\n') ) idx++;
                if(pLine[idx]=='<') pLine[idx] = '\0'; else goto EXIT;
            
                idx++;

                /*/Record>\n*/
                while((pLine[idx] == ' ') || (pLine[idx] == '\t')) idx++;
                if(strncmp( &pLine[idx], "/Record>", 8) == 0 ){
                    idx +=8;
                    PSM_JUMP_NEXTLINE
                }else{
                    goto EXIT;
                }
            }else{
                goto EXIT;
            }
        }else{
            goto EXIT;
        }

        return PSM_LINE_RECORD;
    }else if (strncmp( &pLine[idx], "!--", 3) == 0 ){
        idx = idx +3;
        i = idx;
    
        while( (pLine[idx]!='\n') && (pLine[idx]!='\0') ) idx++;        
        if (pLine[idx] == '\n' ){
            pLine[idx] = '\0';
    
            if(strstr(&pLine[i], "-->")){
                pLine[idx] = '\n';
                PSM_JUMP_NEXTLINE
                return PSM_LINE_COMMENTBEGINEND;
            }else{
                pLine[idx] = '\n';
                PSM_JUMP_NEXTLINE
                return PSM_LINE_COMMENTBEGIN;
            }
        }
    }else if (strncmp( &pLine[idx], "Provision", 9) == 0 ){
        PSM_JUMP_NEXTLINE
        return PSM_LINE_PROVISIONBEGIN;
    }else if (strncmp( &pLine[idx], "/Provision", 10) == 0 ){
        PSM_JUMP_NEXTLINE
        return PSM_LINE_PROVISIONEND;
    }else if (strncmp(&pLine[idx], "?xml", 4) == 0 ){
        PSM_JUMP_NEXTLINE
        return PSM_LINE_XMLHEADER;
    }

EXIT:
    CcspTraceError(("%s -- !!!!!!!!!!!!!!!Meet one bad pLine. Be careful:PSM is not fully XML compitable!!!!!!!!!!!!\n", __FUNCTION__));

    if (*ppName ){
        CcspTraceError(("\t\t!!!!!!!!!!(%s).!!!!!!!!!!!!!!\n", *ppName));
        (*ppName)[strlen(*ppName)] = '"';
        *ppName = NULL;
    }
    
    if (*ppValue ){
        CcspTraceError(("\t\t!!!!!!!!!!(%s).!!!!!!!!!!!!!!\n", *ppValue));
        (*ppValue)[strlen(*ppValue)] = '>';
        *ppValue = NULL;
    }
    
    if (*ppType ){
        CcspTraceError(("\t\t!!!!!!!!!!(%s).!!!!!!!!!!!!!!\n", *ppType));
        (*ppType)[strlen(*ppType)] = '"';
        *ppType = NULL;
    }
    
    if (*ppContentType){
        CcspTraceError(("\t\t!!!!!!!!!!(%s).!!!!!!!!!!!!!!\n", *ppContentType));
        (*ppContentType)[strlen(*ppContentType)] = '"';
        *ppContentType = NULL;
    }

    *ppNextLine = NULL;
    
    return PSM_LINE_BAD;
}

