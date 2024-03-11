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


#ifndef SSP_HASH_H
#define SSP_HASH_H

typedef enum 
_PSM_FILE_TYPE
{
    PSM_FILETYPE_DEF,
    PSM_FILETYPE_CUR,
    PSM_FILETYPE_FULL  //Only it's used when unload record
}
PSM_FILE_TYPE, *PPPSM_FILE_TYPE;

/*
    This function can save content(full/user-changed) into flash 
*/
int PsmHashPeriodicSaving(unsigned int bUserChanged, char * filepath);

#define PSM_HASH_TABLE_LEN     1024
#define PSM_RECORD_ITEM_NUM    1024
#define PSM_RECORD_ADD_NUM     50

extern            ANSC_LOCK         PsmAccessReadLock;
#define           PSMHASHACCESSLOCK         {AnscAcquireLock(&PsmAccessAccessLock);}
#define           PSMHASHACCESSUNLOCK       {AnscReleaseLock(&PsmAccessAccessLock);}

#define  PSM_PARAMETER_NAME_LEN     160

struct _PSM_INFO_RECORD
{
    struct _PSM_INFO_RECORD *pHashNext;
    struct _PSM_INFO_RECORD *pHashPre;
    struct _PSM_INFO_RECORD *pGroupNext;
    struct _PSM_INFO_RECORD *pGroupPre;
    char            *pName;
    char            *pDefVal;
    char            *pCurVal;
    char            *pType;
    char            *pContentType;
/*
    unsigned char	 bComName:1,  //This is for Name, type and ContentType
                     bComDefVal:1,  //When it's true, the memory of pDefVal is using file content by gPsmDefFileBuffer.
                     bComCurVal:1;
*/
};

typedef  struct _PSM_INFO_RECORD    PSM_INFO_RECORD;
typedef  struct _PSM_INFO_RECORD  * PPSM_INFO_RECORD;


typedef  struct
_PSM_GROUP_TABLE
{
    PSM_INFO_RECORD   GroupHeader;
    char             *pGroupName;  //This is like dmsb.l2net.
}
PSM_GROUP_TABLE, *PPSM_GROUP_TABLE;


extern PSM_GROUP_TABLE psmGroupTable[];
 
extern ANSC_LOCK         PsmAccessAccessLock;

PPSM_INFO_RECORD PsmHashRecordFind(char *pName);
int PsmHashRecordSetRecord(char * pName, char * pType, char *pValue);
int PsmHashRecordDelRecord(PPSM_INFO_RECORD pRecord);
PPSM_INFO_RECORD PsmHashRecordCachePop();
int PsmHashRecordFreeString(char * pStr);
int PsmHashRecordAddNewRecord(PPSM_INFO_RECORD pRecord);
int  PsmHashLoadFile(PSM_FILE_TYPE eType, char *buf, int length);
int PsmHashUnloadFile(PSM_FILE_TYPE eType);
int PsmHashInit();

#endif
