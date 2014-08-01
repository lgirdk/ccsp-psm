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

    module:	ssp_cfmif.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

                    Copyright (c) 2011, Cisco Systems, Inc.

                            CISCO CONFIDENTIAL
              Unauthorized distribution or copying is prohibited
                            All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Cisco Systems, Inc.

    ---------------------------------------------------------------

    description:

        This module implements the advanced interface functions
        of the Psm Sys Registry Object.

        *   ssp_CfmReadCurConfig
        *   ssp_CfmReadDefConfig
        *   ssp_CfmSaveCurConfig
        *   ssp_CfmUpdateConfigs

    ---------------------------------------------------------------

    environment:

        platform independent

    ---------------------------------------------------------------

    author:

        Jian Wu
        Lei Chen <leichen2@cisco.com>

    ---------------------------------------------------------------

    revision:

        04/25/12    initial revision.
        08/06/12    adjusting the function to support backup/default config,
                    adding codes to support custom logic.
        08/28/12    adding "Update Configs" and some internal functions.

**********************************************************************/


#include "psm_hal_apis.h"
#include "ssp_global.h"

#include "ansc_xml_dom_parser_interface.h"
#include "ansc_xml_dom_parser_external_api.h"
#include "ansc_xml_dom_parser_status.h"

/*
 * here use printf instead of AnscTraceWarning
 * just for debug purpose
 */
#if defined(_COSA_SIM_)
#define PsmHalDbg(x)    printf x
#else
#define PsmHalDbg(x)    AnscTraceFlow(x)  //AnscTraceWarning(x)
#endif

#define ELEM_PROVISION  "Provision"
#define ELEM_RECORD     "Record"

#define ATTR_NAME       "name"
#define ATTR_TYPE       "type"
#define ATTR_CTYPE      "contentType"
#define ATTR_OVERWRITE  "overwrite"

#define ATTRV_ASTR      "astr"

#define ATTRV_MACADDR   "macAddr"
#define ATTRV_IPV4ADDR  "ip4Addr"
#define ATTRV_IPV6ADDR  "ip6Addr"

#define ATTRV_ALWAYS    "always"
#define ATTRV_COND      "cond"
#define ATTRV_NEVER     "never"

#define xml_for_each_child(child, parent) \
    for (child = parent->GetHeadChild(parent); \
            child; child = parent->GetNextChild(parent, child))

typedef enum PsmOverwrite_e {
    PSM_OVERWRITE_ALWAYS,
    PSM_OVERWRITE_COND,
    PSM_OVERWRITE_NEVER,
} PsmOverwrite_t;

typedef enum PsmRecType_e {
    PSM_REC_TYPE_ASTR,
} PsmRecType_t;

/* content type */
typedef enum PsmRecCType_e {
    PSM_REC_CTYPE_NONE,
    PSM_REC_CTYPE_IPV4ADDR,
    PSM_REC_CTYPE_IPV6ADDR,
    PSM_REC_CTYPE_MACADDR,
} PsmRecCType_t;

typedef struct PsmRecord_s {
    char            name[MAX_NAME_SZ];
    char            value[MAX_NAME_SZ];
    PsmRecType_t    type;
    PsmRecCType_t   ctype;
    PsmOverwrite_t  overwrite;
} PsmRecord_t;

static PANSC_XML_DOM_NODE_OBJECT
CfgBufferToXml(const void *buf, ULONG size)
{
    char                        *tmpBuf;
    ULONG                       tmpSize;
    PANSC_XML_DOM_NODE_OBJECT   rootNode;

    /* ppCfgBuffer may modified by parser, so we have to get a copy first */
    tmpSize = size;
    if ((tmpBuf = AnscAllocateMemory(tmpSize + 1)) == NULL)
    {
        PsmHalDbg(("%s: no memory\n", __FUNCTION__));
        return NULL;
    }
    AnscCopyMemory(tmpBuf, (void *)buf, tmpSize);
    tmpBuf[tmpSize] = '\0';

    if ((rootNode = (PANSC_XML_DOM_NODE_OBJECT)AnscXmlDomParseString(NULL, 
            (PCHAR *)&tmpBuf, tmpSize)) == NULL)
    {
        PsmHalDbg(("%s: parse error\n", __FUNCTION__));
        /*AnscFreeMemory(tmpBuf); already freed */
        return NULL;
    }

    /*AnscFreeMemory(tmpBuf); */
    return rootNode;
}

static ANSC_STATUS
FileReadToBuffer(const char *file, char **buf, ULONG *size)
{
    ANSC_HANDLE pFile;

    if ((pFile = AnscOpenFile((char *)file, ANSC_FILE_MODE_READ, ANSC_FILE_TYPE_RDWR)) == NULL)
        return ANSC_STATUS_FAILURE;

    if ((*size = AnscGetFileSize(pFile)) < 0)
    {
        AnscCloseFile(pFile);
        return ANSC_STATUS_FAILURE;
    }

    if ((*buf = AnscAllocateMemory(*size + 1)) == NULL)
    {
        AnscCloseFile(pFile);
        return ANSC_STATUS_FAILURE;
    }

    /* this API need ULONG * for third param */
    if (AnscReadFile(pFile, *buf, size) != ANSC_STATUS_SUCCESS)
    {
        AnscFreeMemory(*buf);
        AnscCloseFile(pFile);
        return ANSC_STATUS_FAILURE;
    }

    (*buf)[*size] = '\0';

    AnscCloseFile(pFile);
    return ANSC_STATUS_SUCCESS;
}

static void
DumpRecord(const PsmRecord_t *rec)
{
    PsmHalDbg(("  Rec.name   %s\n", rec->name));
    PsmHalDbg(("  Rec.value  %s\n", rec->value));

    switch (rec->type)
    {
        case PSM_REC_TYPE_ASTR:
            PsmHalDbg(("  Rec.type   %s\n", ATTRV_ASTR));
            break;
        default:
            PsmHalDbg(("  Rec.type   %s\n", "<unknow>"));
            break;
    }

    switch (rec->ctype)
    {
        case PSM_REC_CTYPE_IPV4ADDR:
            PsmHalDbg(("  Rec.ctype  %s\n", ATTRV_IPV4ADDR));
            break;
        case PSM_REC_CTYPE_IPV6ADDR:
            PsmHalDbg(("  Rec.ctype  %s\n", ATTRV_IPV6ADDR));
            break;
        case PSM_REC_CTYPE_MACADDR:
            PsmHalDbg(("  Rec.ctype  %s\n", ATTRV_MACADDR));
            break;
        case PSM_REC_CTYPE_NONE:
        default:
            PsmHalDbg(("  Rec.ctype  %s\n", "none"));
            break;
    }

    switch (rec->overwrite)
    {
        case PSM_OVERWRITE_ALWAYS:
            PsmHalDbg(("  Rec.overw  %s\n", ATTRV_ALWAYS));
            break;
        case PSM_OVERWRITE_COND:
            PsmHalDbg(("  Rec.overw  %s\n", ATTRV_COND));
            break;
        case PSM_OVERWRITE_NEVER:
        default:
            PsmHalDbg(("  Rec.overw  %s\n", ATTRV_NEVER));
            break;
    }

    return;
}

static ANSC_STATUS
RecordSetNode(const PsmRecord_t *rec, PANSC_XML_DOM_NODE_OBJECT node)
{
    if (AnscSizeOfString(rec->name) == 0 || AnscSizeOfString(rec->value) == 0)
    {
        PsmHalDbg(("%s: Empty name or value, Skip it !!!\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    node->SetAttrString(node, ATTR_NAME, (char *)rec->name, AnscSizeOfString(rec->name));
    /* only one type */
    node->SetAttrString(node, ATTR_TYPE, ATTRV_ASTR, AnscSizeOfString(ATTRV_ASTR));

    switch (rec->ctype) {
    case PSM_REC_CTYPE_IPV4ADDR:
        node->SetAttrString(node, ATTR_CTYPE, ATTRV_IPV4ADDR, 
                AnscSizeOfString(ATTRV_IPV4ADDR));
        break;
    case PSM_REC_CTYPE_IPV6ADDR:
        node->SetAttrString(node, ATTR_CTYPE, ATTRV_IPV6ADDR, 
                AnscSizeOfString(ATTRV_IPV6ADDR));
        break;
    case PSM_REC_CTYPE_MACADDR:
        node->SetAttrString(node, ATTR_CTYPE, ATTRV_MACADDR, 
                AnscSizeOfString(ATTRV_MACADDR));
        break;
    case PSM_REC_CTYPE_NONE:
    default:
        /* no "DelAttr" available */
#if 0 /* empty string will cause SIGSEGV at AnscXmlGetAttr2BufSize() */
        node->SetAttrString(node, ATTR_CTYPE, "", AnscSizeOfString(""));
#else
        node->SetAttrString(node, ATTR_CTYPE, "none", AnscSizeOfString("none"));
#endif
        break;
    }

    switch (rec->overwrite) {
    case PSM_OVERWRITE_ALWAYS:
        node->SetAttrString(node, ATTR_OVERWRITE, ATTRV_ALWAYS, 
                AnscSizeOfString(ATTRV_ALWAYS));
        break;
    case PSM_OVERWRITE_COND:
        node->SetAttrString(node, ATTR_OVERWRITE, ATTRV_COND,
                AnscSizeOfString(ATTRV_COND));
        break;
    case PSM_OVERWRITE_NEVER:
    default:
        node->SetAttrString(node, ATTR_OVERWRITE, ATTRV_NEVER, 
                AnscSizeOfString(ATTRV_NEVER));
        break;
    }

    node->SetDataString(node, NULL, (char *)rec->value, AnscSizeOfString(rec->value));

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
NodeGetRecord(PANSC_XML_DOM_NODE_OBJECT node, PsmRecord_t *rec)
{
    ULONG size;
    char buf[MAX_NAME_SZ];

    if (!AnscEqualString(node->GetName(node), ELEM_RECORD, TRUE))
        return ANSC_STATUS_FAILURE;
    
    AnscZeroMemory(rec, sizeof(PsmRecord_t));

    /* name */
    size = sizeof(rec->name) - 1;
    if (node->GetAttrString(node, ATTR_NAME, rec->name, &size) != ANSC_STATUS_SUCCESS)
        return ANSC_STATUS_FAILURE;
    rec->name[size] = '\0';

    /* type */
    /* only one type
    size = sizeof(buf);
    if (node->GetAttrString(node, ATTR_TYPE, buf, &size) != ANSC_STATUS_SUCCESS)
        return ANSC_STATUS_FAILURE;
    buf[size] = '\0';
    */
    rec->type = PSM_REC_TYPE_ASTR;

    /* content type */
    size = sizeof(buf) - 1;
    rec->ctype = PSM_REC_CTYPE_NONE; /* no this attr is ok */
    if (node->GetAttrString(node, ATTR_CTYPE, buf, &size) == ANSC_STATUS_SUCCESS)
    {
        buf[size] = '\0';
        if (AnscEqualString(buf, ATTRV_MACADDR, TRUE))
            rec->ctype = PSM_REC_CTYPE_MACADDR;
        else if (AnscEqualString(buf, ATTRV_IPV4ADDR, TRUE))
            rec->ctype = PSM_REC_CTYPE_IPV4ADDR;
        else if (AnscEqualString(buf, ATTRV_IPV6ADDR, TRUE))
            rec->ctype = PSM_REC_CTYPE_IPV6ADDR;
    }

    /* overwrite */
    size = sizeof(buf) - 1;
    rec->overwrite = PSM_OVERWRITE_NEVER; /* no this attr is ok */
    if (node->GetAttrString(node, ATTR_OVERWRITE, buf, &size) == ANSC_STATUS_SUCCESS)
    {
        buf[size] = '\0';
        if (AnscEqualString(buf, ATTRV_ALWAYS, TRUE))
            rec->overwrite = PSM_OVERWRITE_ALWAYS;
        else if (AnscEqualString(buf, ATTRV_COND, TRUE))
            rec->overwrite = PSM_OVERWRITE_COND;
        else if (AnscEqualString(buf, ATTRV_NEVER, TRUE))
            rec->overwrite = PSM_OVERWRITE_NEVER;
    }

    /* value */
    size = sizeof(rec->value) - 1;
    if (node->GetDataString(node, NULL, rec->value, &size) != ANSC_STATUS_SUCCESS)
        return ANSC_STATUS_FAILURE;
    rec->value[size] = '\0';

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
AddRecToXml(const PsmRecord_t *rec, PANSC_XML_DOM_NODE_OBJECT xml)
{
    PANSC_XML_DOM_NODE_OBJECT node;

    if ((node = xml->AddChildByName(xml, ELEM_RECORD)) == NULL)
        return ANSC_STATUS_FAILURE;

    if (RecordSetNode(rec, node) != ANSC_STATUS_SUCCESS)
    {
        xml->DelChild(xml, node);
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
XmlToBuffer(PANSC_XML_DOM_NODE_OBJECT xml, char **buf, ULONG *size)
{
    char *newBuf = NULL;
    ULONG newSize;

    if ((*size = newSize = xml->GetEncodedSize(xml)) == 0
            /* any one tell me the reason magic 16 ? */
            || (newBuf = AnscAllocateMemory(newSize + 16)) == NULL 
            || xml->Encode(xml, (PVOID)newBuf, &newSize) != ANSC_STATUS_SUCCESS)
    {
        if (newBuf)
            AnscFreeMemory(newBuf);
        return ANSC_STATUS_FAILURE;
    }

    *buf = newBuf;
    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
XmlToFile(PANSC_XML_DOM_NODE_OBJECT xml, const char *file)
{
    char *buf;
    ULONG size;
    ANSC_HANDLE pFile;

    if (XmlToBuffer(xml, &buf, &size) != ANSC_STATUS_SUCCESS)
        return ANSC_STATUS_FAILURE;

    if ((pFile = AnscOpenFile((char *)file, 
                    ANSC_FILE_TYPE_RDWR, ANSC_FILE_TYPE_RDWR)) == NULL)
    {
        AnscFreeMemory(buf);
        return ANSC_STATUS_FAILURE;
    }

    if (AnscWriteFile(pFile, buf, &size) != ANSC_STATUS_SUCCESS)
    {
        AnscCloseFile(pFile);
        AnscFreeMemory(buf);
        return ANSC_STATUS_FAILURE;
    }

    AnscCloseFile(pFile);
    AnscFreeMemory(buf);
    return ANSC_STATUS_SUCCESS;
}

static BOOL
IsRecChangedFromXml(const PsmRecord_t *rec, PANSC_XML_DOM_NODE_OBJECT xml)
{
    PANSC_XML_DOM_NODE_OBJECT node;
    PsmRecord_t cur;

    xml_for_each_child(node, xml)
    {
        if (NodeGetRecord(node, &cur) != ANSC_STATUS_SUCCESS)
            continue;

        if (!AnscEqualString(cur.name, (char *)rec->name, TRUE))
            continue;

        if (AnscEqualString(cur.value, (char *)rec->value, TRUE))
            return FALSE;
        else
            return TRUE;
    }

    /* if no correspand record in xml, means "not changed" ? */
    return FALSE;
}

/* 
 * return generated XML root node if success and NULL if error.
 * here @overwrite means whether to use the custom to 
 * overwrite the XML file's config if exist 
 */
static PANSC_XML_DOM_NODE_OBJECT
ReadCfgXmlWithCustom(const char *path, int overwrite)
{
    PANSC_XML_DOM_NODE_OBJECT   root = NULL;
    PANSC_XML_DOM_NODE_OBJECT   node = NULL;
    PsmRecord_t                 rec;
    PsmHalParam_t               *cusParams = NULL;
    int                         cusCnt, i;
    char                        *buf = NULL;
    int                         success = 0;
    int                         missing;
    ULONG                       size;

    if (PsmHal_GetCustomParams(&cusParams, &cusCnt) != 0)
        cusParams = NULL, cusCnt = 0;

    if (FileReadToBuffer(path, &buf, &size) != ANSC_STATUS_SUCCESS)
        goto done;

    if ((root = CfgBufferToXml(buf, size)) == NULL)
        goto done;

    for (i = 0; i < cusCnt; i++)
    {
        missing = 1;
        xml_for_each_child(node, root)
        {
            if (NodeGetRecord(node, &rec) != ANSC_STATUS_SUCCESS)
                continue;

            if (!AnscEqualString(rec.name, cusParams[i].name, TRUE))
                continue;

            missing = 0;
            if (overwrite)
            {
                snprintf(rec.value, sizeof(rec.value), "%s", cusParams[i].value);

                if (RecordSetNode(&rec, node) != ANSC_STATUS_SUCCESS)
                    PsmHalDbg(("%s: fail to set %s with custom value\n", 
                                __FUNCTION__, rec.name));
            }

            break;
        }

        if (missing)
        {
            snprintf(rec.name, sizeof(rec.name), "%s", cusParams[i].name);
            snprintf(rec.value, sizeof(rec.value), "%s", cusParams[i].value);
            rec.type = PSM_REC_TYPE_ASTR;
            rec.ctype = PSM_REC_CTYPE_NONE;
            rec.overwrite = PSM_OVERWRITE_NEVER;

            if (AddRecToXml(&rec, root) != ANSC_STATUS_SUCCESS)
                PsmHalDbg(("%s: fail to add custom config %s\n", 
                            __FUNCTION__, cusParams[i].name));
        }
    }

    success = 1;

done:
    if (!success && root)
    {
        root->Remove(root);
        root = NULL;
    }
    if (buf)
        AnscFreeMemory(buf);
    if (cusParams)
        free(cusParams);

    return root;
}

static ANSC_STATUS
CfmMergeMissingConfigs(void **ppToBuffer, ULONG *pulToSize, const void *pFromBuffer, ULONG ulFromSize)
{
    PANSC_XML_DOM_NODE_OBJECT   toRoot = NULL;
    PANSC_XML_DOM_NODE_OBJECT   toRec = NULL;
    char                        toName[MAX_NAME_SZ];
    ULONG                       toNsize;
    char                        *newBuf = NULL;
    ULONG                       newSize;
    PANSC_XML_DOM_NODE_OBJECT   newRec = NULL;

    PANSC_XML_DOM_NODE_OBJECT   fromRoot = NULL;
    PANSC_XML_DOM_NODE_OBJECT   fromRec = NULL;
    char                        fromName[MAX_NAME_SZ];
    char                        fromValue[MAX_NAME_SZ];
    ULONG                       fromNsize, fromVsize;
    ANSC_STATUS                 ret = ANSC_STATUS_FAILURE;
    int                         missing, changed = 0;

    PsmHalDbg(("%s: Try to merge configs\n", __FUNCTION__));

    if ((fromRoot = CfgBufferToXml(pFromBuffer, ulFromSize)) == NULL)
        goto done;
    if ((toRoot = CfgBufferToXml(*ppToBuffer, *pulToSize)) == NULL)
        goto done;

    /* for each <Record/> in from config */
    for (fromRec = fromRoot->GetHeadChild(fromRoot);
            fromRec != NULL; fromRec = fromRoot->GetNextChild(fromRoot, fromRec))
    {
        if (!AnscEqualString(fromRec->GetName(fromRec), ELEM_RECORD, TRUE))
            continue;

        AnscZeroMemory(fromName, sizeof(fromName));
        fromNsize = sizeof(fromName) - 1;
        AnscZeroMemory(fromValue, sizeof(fromValue));
        fromVsize = sizeof(fromValue) - 1;
        if (fromRec->GetAttrString(fromRec, ATTR_NAME, fromName, &fromNsize) != ANSC_STATUS_SUCCESS
                || fromRec->GetDataString(fromRec, NULL, fromValue, &fromVsize) != ANSC_STATUS_SUCCESS)
        {
            PsmHalDbg(("%s: fail to get record's name/value", __FUNCTION__));
            continue;
        }

        if (fromName[0] == '\0')
            continue;

        /* find the name in to config, if missing add it then */
        missing = 1;
        for (toRec = toRoot->GetHeadChild(toRoot);
                toRec != NULL; toRec = toRoot->GetNextChild(toRoot, toRec))
        {
            if (!AnscEqualString(toRec->GetName(toRec), ELEM_RECORD, TRUE))
                continue;

            AnscZeroMemory(toName, sizeof(toName));
            toNsize = sizeof(toName) - 1;
            if (toRec->GetAttrString(toRec, ATTR_NAME, toName, &toNsize) != ANSC_STATUS_SUCCESS)
            {
                PsmHalDbg(("%s: fail to get record's name/value", __FUNCTION__));
                continue;
            }

            if (AnscEqualString(fromName, toName, TRUE))
            {
                missing = 0;
                break;
            }
        }

        /* add it to "to" config */
        if (missing)
        {
            PsmHalDbg(("%s: MISSING %s:%s\n", __FUNCTION__, fromName, fromValue));

            if ((newRec = toRoot->AddChildByName(toRoot, ELEM_RECORD)) != NULL)
            {
                newRec->SetAttrString(newRec, ATTR_NAME, 
                        fromName, AnscSizeOfString(fromName));
                newRec->SetAttrString(newRec, ATTR_TYPE, 
                        ATTRV_ASTR, AnscSizeOfString(ATTRV_ASTR));
                newRec->SetDataString(newRec, NULL, 
                        fromValue, AnscSizeOfString(fromValue));

                PsmHalDbg(("%s: new record %s:%s\n", __FUNCTION__, fromName, fromValue));

                changed = 1;
            }
        }
        else
        {
            ;//PsmHalDbg(("%s: EXIST   %s:%s\n", __FUNCTION__, fromName, fromValue));
        }
    }

    if (changed)
    {
        /* regenarate the config buffer according to new XML tree */
        if ((*pulToSize = newSize = toRoot->GetEncodedSize(toRoot)) == 0
                || (newBuf = AnscAllocateMemory(newSize + 16)) == NULL
                || toRoot->Encode(toRoot, (PVOID)newBuf, &newSize) != ANSC_STATUS_SUCCESS)
            goto done;

        /* if regenarate sccuess free old buffer */
        AnscFreeMemory(*ppToBuffer);
        *ppToBuffer = newBuf;
        //*pulToSize = newSize;

        /*
        PsmHalDbg(("%s: The config after merge: %lu bytes %d (strlen)\n%s\n", 
                    __FUNCTION__, *pulToSize, strlen((char *)*ppToBuffer), (char *)*ppToBuffer));
        */
    }

    /* everything is OK */
    ret = ANSC_STATUS_SUCCESS;

done:
    if (ret != ANSC_STATUS_SUCCESS && newBuf)
        AnscFreeMemory(newBuf);
    if (toRoot)
        toRoot->Remove(toRoot);
    if (fromRoot)
        fromRoot->Remove(fromRoot);

    return ret;
}

/**
 * @ppCfgBuffer and @ulCfgSize are [in-out] arguments
 */
static ANSC_STATUS
CfmImportCustomConfig(void **ppCfgBuffer, ULONG *pulCfgSize, BOOL overwrite)
{
    PANSC_XML_DOM_NODE_OBJECT   rootNode = NULL;
    PANSC_XML_DOM_NODE_OBJECT   recNode = NULL;
    PsmHalParam_t               *cusParams;
    int                         cusCnt, i, changed = 0;
    ANSC_STATUS                 ret = ANSC_STATUS_FAILURE;
    char                        name[MAX_NAME_SZ];
    char                        value[MAX_NAME_SZ];
    ULONG                       nsize, vsize;
    char                        *newBuf = NULL;
    ULONG                       newSize;

    if (PsmHal_GetCustomParams(&cusParams, &cusCnt) != 0 || cusCnt == 0)
    {
        PsmHalDbg(("%s: no custom config need import\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS; /* nothing to do */
    }

    PsmHalDbg(("%s: Try to import custom config\n", __FUNCTION__));

    /* I prefer not to use XML tree, but manipulater XML string is more complicated */
    if ((rootNode = CfgBufferToXml(*ppCfgBuffer, *pulCfgSize)) == NULL)
        goto done;

    if (!AnscEqualString(rootNode->GetName(rootNode), ELEM_PROVISION, TRUE))
        goto done;

    /* for each <Record/> */
    for (recNode = rootNode->GetHeadChild(rootNode);
            recNode != NULL; recNode = rootNode->GetNextChild(rootNode, recNode))
    {
        if (!AnscEqualString(rootNode->GetName(recNode), ELEM_RECORD, TRUE))
            continue;

        AnscZeroMemory(name, sizeof(name));
        AnscZeroMemory(value, sizeof(value));
        nsize = sizeof(name) - 1;
        vsize = sizeof(value) - 1;
        if (recNode->GetAttrString(recNode, ATTR_NAME, name, &nsize) != ANSC_STATUS_SUCCESS
                || recNode->GetDataString(recNode, NULL, value, &vsize) != ANSC_STATUS_SUCCESS)
        {
            PsmHalDbg(("%s: fail to get record's name/value", __FUNCTION__));
            continue;
        }

        for (i = 0; i < cusCnt; i++)
        {
            if (cusParams[i].name[0] == '\0'
                    || !AnscEqualString(name, cusParams[i].name, TRUE))
                continue;

            cusParams[i].name[0] = '\0'; /* clear it for later reference */
            if (vsize == 0 || !AnscEqualString(value, cusParams[i].value, TRUE))
            {
                PsmHalDbg(("%s: new  value for %s %s, overwrite %d\n", __FUNCTION__, name, value, overwrite));
                if (overwrite)
                {
                    recNode->SetDataString(recNode, NULL, 
                            cusParams[i].value, AnscSizeOfString(cusParams[i].value));
                    changed = 1;
                }
            }
            else
            {
                PsmHalDbg(("%s: same value for %s\n", __FUNCTION__, name));
            }
        }
    }

    /* for these missing params in current config (but exist in custom param) */
    for (i = 0; i < cusCnt; i++)
    {
        if (cusParams[i].name[0] == '\0')
            continue;

        recNode = rootNode->AddChildByName(rootNode, ELEM_RECORD);
        if (!recNode)
        {
            PsmHalDbg(("%s: fail to add new record", __FUNCTION__));
            continue;
        }

        recNode->SetAttrString(recNode, ATTR_NAME, 
                cusParams[i].name, AnscSizeOfString(cusParams[i].name));
        recNode->SetAttrString(recNode, ATTR_TYPE, 
                ATTRV_ASTR, AnscSizeOfString(ATTRV_ASTR));
        recNode->SetDataString(recNode, NULL, 
                cusParams[i].value, AnscSizeOfString(cusParams[i].value));
        
        PsmHalDbg(("%s: new record %s:%s\n", __FUNCTION__, cusParams[i].name, cusParams[i].value));

        changed = 1;
    }

    if (changed)
    {
        /* regenarate the config buffer according to new XML tree */
        if ((*pulCfgSize = newSize = rootNode->GetEncodedSize(rootNode)) == 0
                || (newBuf = AnscAllocateMemory(newSize + 16)) == NULL
                || rootNode->Encode(rootNode, (PVOID)newBuf, &newSize) != ANSC_STATUS_SUCCESS)
            goto done;

        /* if regenarate sccuess free old buffer */
        AnscFreeMemory(*ppCfgBuffer);
        *ppCfgBuffer = newBuf;
        //*pulCfgSize = newSize;

        /*
        PsmHalDbg(("%s: The config after import: %lu bytes %d (strlen)\n%s\n", 
                    __FUNCTION__, *pulCfgSize, strlen((char *)*ppCfgBuffer), (char *)*ppCfgBuffer));
        */
    }

    /* everything is OK */
    ret = ANSC_STATUS_SUCCESS;

done:
    if (ret != ANSC_STATUS_SUCCESS && newBuf)
        AnscFreeMemory(newBuf);
    if (rootNode)
        rootNode->Remove(rootNode);
    if (cusParams)
        free(cusParams); /* use free() instead of AnscFreeMemory() */

    return ret;
}

/* read config file from path then import custum parameters into it */
static ANSC_STATUS
CfmReadConfigFile(const char *path, void **ppCfgBuffer, PULONG pulCfgSize, 
        BOOL overwrite, PFN_PSMFLO_TEST pfVerify)
{
    ANSC_HANDLE pFile = NULL;
    PPSM_FILE_LOADER_OBJECT pFlo = NULL;

    *ppCfgBuffer = NULL;
    *pulCfgSize = 0;

    PsmHalDbg(("%s: start read file %s\n", __FUNCTION__, path));

    if ((pFile = AnscOpenFile((char *)path, ANSC_FILE_MODE_READ, ANSC_FILE_TYPE_RDWR)) == NULL)
        return ANSC_STATUS_FAILURE;

    if ((*pulCfgSize = AnscGetFileSize(pFile)) < 0)
        goto errout;

    if ((*ppCfgBuffer = AnscAllocateMemory(*pulCfgSize)) == NULL)
        goto errout;

    if (AnscReadFile(pFile, *ppCfgBuffer, pulCfgSize) != ANSC_STATUS_SUCCESS)
        goto errout;

    if (CfmImportCustomConfig(ppCfgBuffer, pulCfgSize, overwrite) != ANSC_STATUS_SUCCESS)
    {
        PsmHalDbg(("%s: fail to import custum config\n", __FUNCTION__));
        goto errout;
    }

    PsmHalDbg(("%s: start verification\n", __FUNCTION__));

    if (pfVerify && (pFlo = ACCESS_CONTAINER(pfVerify, PSM_FILE_LOADER_OBJECT, TestRegFile))
            && pfVerify(pFlo, *ppCfgBuffer, *pulCfgSize) != PSM_FLO_ERROR_CODE_noError)
        goto errout;

    AnscCloseFile(pFile);

    PsmHalDbg(("%s: read %s succeed\n", __FUNCTION__, path));
    return ANSC_STATUS_SUCCESS;

errout:
    if (pFile)
        AnscCloseFile(pFile);
    if (*ppCfgBuffer)
        AnscFreeMemory(*ppCfgBuffer);

    return ANSC_STATUS_FAILURE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        ssp_CfmReadCurConfig
            (
                ANSC_HANDLE                 hThisObject,
                void**                      ppCfgBuffer,
                PULONG                      pulCfgSize
            );

    description:

        This function is called to read the current Psm configuration
        into the memory.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void**                      ppCfgBuffer
                Specifies the configuration file content to be returned.

                PULONG                      pulCfgSize
                Specifies the size of the file content to be returned.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
ssp_CfmReadCurConfig
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    )
{
    PPSM_SYS_REGISTRY_OBJECT        pPsm = (PPSM_SYS_REGISTRY_OBJECT)hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY      pProp = (PPSM_SYS_REGISTRY_PROPERTY)&pPsm->Property;
    PPSM_FILE_LOADER_OBJECT         pFlo = (PPSM_FILE_LOADER_OBJECT)pPsm->hPsmFileLoader;
    PFN_PSMFLO_TEST                 pfTest = (PFN_PSMFLO_TEST)pFlo->TestRegFile;
    char                            curPath[256];
    char                            bakPath[256];
    char                            defPath[256];
    ANSC_HANDLE                     pBakFile = NULL;
    void                            *pDefBuf;
    ULONG                           ulDefSize;

    *ppCfgBuffer = NULL;
    *pulCfgSize = 0;

    snprintf(curPath, sizeof(curPath), "%s%s", pProp->SysFilePath, pProp->CurFileName);
    snprintf(bakPath, sizeof(bakPath), "%s%s", pProp->SysFilePath, pProp->BakFileName);
    snprintf(defPath, sizeof(defPath), "%s%s", pProp->SysFilePath, pProp->DefFileName);

    /**
     * process Current Config File first
     */
    PsmHalDbg(("%s: Try to read current config\n", __FUNCTION__));

    if (CfmReadConfigFile(curPath, ppCfgBuffer, pulCfgSize, FALSE, pfTest) == ANSC_STATUS_SUCCESS)
    {
        /* merge missing config from default config to current config */ 
        if (CfmReadConfigFile(defPath, &pDefBuf, &ulDefSize, FALSE, pfTest) == ANSC_STATUS_SUCCESS)
        {
            if (CfmMergeMissingConfigs(ppCfgBuffer, pulCfgSize, pDefBuf, ulDefSize) == ANSC_STATUS_SUCCESS)
            {
                AnscFreeMemory(pDefBuf);
                PsmHalDbg(("%s: success\n", __FUNCTION__));
                return ANSC_STATUS_SUCCESS;
            }

            AnscFreeMemory(pDefBuf);
        }

        AnscFreeMemory(*ppCfgBuffer);
        *ppCfgBuffer = NULL;
    }

    /**
     * some problem happened for corruent config
     * use backup file if exist
     */
    PsmHalDbg(("%s: Try to read backup config\n", __FUNCTION__));

    if ((pBakFile = AnscOpenFile(bakPath, ANSC_FILE_TYPE_RDWR, ANSC_FILE_TYPE_RDWR)) != NULL)
    {
        AnscCloseFile(pBakFile);
        if (AnscCopyFile(bakPath, curPath, TRUE) == ANSC_STATUS_SUCCESS)
        {
            if (CfmReadConfigFile(curPath, ppCfgBuffer, pulCfgSize, FALSE, pfTest) == ANSC_STATUS_SUCCESS)
            {
                /* merge missing config from default config to current config */ 
                if (CfmReadConfigFile(defPath, &pDefBuf, &ulDefSize, FALSE, pfTest) == ANSC_STATUS_SUCCESS)
                {
                    if (CfmMergeMissingConfigs(ppCfgBuffer, pulCfgSize, pDefBuf, ulDefSize) == ANSC_STATUS_SUCCESS)
                    {
                        AnscFreeMemory(pDefBuf);
                        PsmHalDbg(("%s: success\n", __FUNCTION__));
                        return ANSC_STATUS_SUCCESS;
                    }

                    AnscFreeMemory(pDefBuf);
                }

                AnscFreeMemory(*ppCfgBuffer);
                *ppCfgBuffer = NULL;
            }
        }
    }

    /**
     * now we could only use default config file 
     */
    PsmHalDbg(("%s: Try to read default config\n", __FUNCTION__));

    /* we use "overwrite" mode here, to use devcie specific config anyway */
    if (CfmReadConfigFile(defPath, ppCfgBuffer, pulCfgSize, TRUE, pfTest) == ANSC_STATUS_SUCCESS)
    {
        AnscCopyFile(defPath, curPath, TRUE);
        PsmHalDbg(("%s: success\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    PsmHalDbg(("%s: Fail to read config file (all attempt failed)\n", __FUNCTION__));

    /* all attempt failed */
    return ANSC_STATUS_FAILURE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        ssp_CfmReadDefConfig
            (
                ANSC_HANDLE                 hThisObject,
                void**                      ppCfgBuffer,
                PULONG                      pulCfgSize
            );

    description:

        This function is called to read the default Psm configuration
        into the memory.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void**                      ppCfgBuffer
                Specifies the configuration file content to be returned.

                PULONG                      pulCfgSize
                Specifies the size of the file content to be returned.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
ssp_CfmReadDefConfig
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    )
{
    PPSM_SYS_REGISTRY_OBJECT        pPsm = (PPSM_SYS_REGISTRY_OBJECT)hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY      pProp = (PPSM_SYS_REGISTRY_PROPERTY)&pPsm->Property;
    PPSM_FILE_LOADER_OBJECT         pFlo = (PPSM_FILE_LOADER_OBJECT)pPsm->hPsmFileLoader;
    PFN_PSMFLO_TEST                 pfTest = (PFN_PSMFLO_TEST)pFlo->TestRegFile;
    char                            defPath[256];

    PsmHalDbg(("%s: called\n", __FUNCTION__));

    snprintf(defPath, sizeof(defPath), "%s%s", pProp->SysFilePath, pProp->DefFileName);
    if (CfmReadConfigFile(defPath, ppCfgBuffer, pulCfgSize, TRUE, pfTest) != ANSC_STATUS_SUCCESS)
        return ANSC_STATUS_FAILURE;

    return ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        ssp_CfmSaveCurConfig
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pCfgBuffer,
                ULONG                       ulCfgSize
            );

    description:

        This function is called to save the current Psm configuration
        into the file system.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pCfgBuffer
                Specifies the configuration file content to be saved.

                ULONG                       ulCfgSize
                Specifies the size of the file content to be saved.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
ssp_CfmSaveCurConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    )
{
    PPSM_SYS_REGISTRY_OBJECT        pPsm = (PPSM_SYS_REGISTRY_OBJECT)hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY      pProp = (PPSM_SYS_REGISTRY_PROPERTY)&pPsm->Property;
    char                            curPath[256], bakPath[256];
    ANSC_HANDLE                     pFile;

    PsmHalDbg(("%s: called\n", __FUNCTION__));

    /* save current to backup config */
    snprintf(curPath, sizeof(curPath), "%s%s", pProp->SysFilePath, pProp->CurFileName);
    snprintf(bakPath, sizeof(bakPath), "%s%s", pProp->SysFilePath, pProp->BakFileName);

    if (AnscCopyFile(curPath, bakPath, TRUE) != ANSC_STATUS_SUCCESS)
        PsmHalDbg(("%s: fail to backup current config\n", __FUNCTION__));

    if ((pFile = AnscOpenFile(curPath, ANSC_FILE_MODE_WRITE | ANSC_FILE_MODE_TRUNC,
            ANSC_FILE_TYPE_RDWR)) == NULL)
    {
        PsmHalDbg(("%s: fail open current config\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    if (AnscWriteFile(pFile, pCfgBuffer, &ulCfgSize) != ANSC_STATUS_SUCCESS)
    {
        AnscCloseFile(pFile);
        return ANSC_STATUS_FAILURE;
    }

    AnscCloseFile(pFile);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
ssp_CfmUpdateConfigs(ANSC_HANDLE hThisObject, const char *newConfPath)
{
    PPSM_SYS_REGISTRY_OBJECT        pPsm = (PPSM_SYS_REGISTRY_OBJECT)hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY      pProp = (PPSM_SYS_REGISTRY_PROPERTY)&pPsm->Property;
    PANSC_XML_DOM_NODE_OBJECT       newXml = NULL;
    PANSC_XML_DOM_NODE_OBJECT       curXml = NULL;
    PANSC_XML_DOM_NODE_OBJECT       defXml = NULL;
    char                            curPath[256];
    char                            defPath[256];
    PANSC_XML_DOM_NODE_OBJECT       newNode; /* record node for "new config" */
    PANSC_XML_DOM_NODE_OBJECT       curNode; /* record node for "current config" */
    PsmRecord_t                     newRec, curRec;
    int                             missing;
    ANSC_STATUS                     err = ANSC_STATUS_FAILURE;

    if (!newConfPath || AnscSizeOfString(newConfPath) <= 0)
    {
        PsmHalDbg(("%s: bad param\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    snprintf(curPath, sizeof(curPath), "%s%s", pProp->SysFilePath, pProp->CurFileName);
    snprintf(defPath, sizeof(defPath), "%s%s", pProp->SysFilePath, pProp->DefFileName);

    PsmHalDbg(("=======  %s  =======\n", __FUNCTION__));
    PsmHalDbg(("New Cfg: %s\n", newConfPath));
    PsmHalDbg(("Cur Cfg: %s\n", defPath));
    PsmHalDbg(("Dev Cfg: %s\n", curPath));
    PsmHalDbg(("\n"));

    /* 
     * load different config file to XML tree for later use.
     * when loading, the custom param (device specific params) 
     * should be merged into XML Tree. For default config (both orig/new), 
     * custom param should 'overwrite' the param existed.
     */
    newXml = ReadCfgXmlWithCustom(newConfPath, TRUE);
    curXml = ReadCfgXmlWithCustom(curPath, FALSE);
    defXml = ReadCfgXmlWithCustom(defPath, TRUE);
    if (!newXml || !curXml || !defXml)
    {
        PsmHalDbg(("%s: fail to read new/def/cur config\n", __FUNCTION__));
        goto done;
    }

    xml_for_each_child(newNode, newXml)
    {
        if (NodeGetRecord(newNode, &newRec) != ANSC_STATUS_SUCCESS)
            continue;

        if (AnscSizeOfString(newRec.name) == 0 || AnscSizeOfString(newRec.value) == 0)
        {
            PsmHalDbg(("%s: empty name or value new cfg, skip it !!!!\n", __FUNCTION__));
            continue;
        }

        PsmHalDbg(("\n[ New Rec ] ...\n"));
        DumpRecord(&newRec);

        missing = 1;
        xml_for_each_child(curNode, curXml)
        {
            if (NodeGetRecord(curNode, &curRec) != ANSC_STATUS_SUCCESS)
                continue;

            if (!AnscEqualString(newRec.name, curRec.name, TRUE))
                continue;

            PsmHalDbg(("  -- Exist in cur cec ...\n"));
            DumpRecord(&curRec);

            /* found the same param, process it according to 
             * current rec's overwrite type */
            missing = 0;

            /* 
             * XXX: despite the value, how about the "attr",
             * shouldn't we overwrite them ???
             */

            switch (curRec.overwrite)
            {
            case PSM_OVERWRITE_ALWAYS:
            case PSM_OVERWRITE_COND:
                if (curRec.overwrite == PSM_OVERWRITE_ALWAYS)
                    PsmHalDbg(("Need overwrite ... (always)\n"));

                if (curRec.overwrite == PSM_OVERWRITE_COND)
                {
                    if (IsRecChangedFromXml(&curRec, defXml))
                        break;
                    else
                        PsmHalDbg(("Need overwrite ... (cond && not-changed)\n"));
                }

                /* do overwrite */
                if (RecordSetNode(&newRec, curNode) != ANSC_STATUS_SUCCESS)
                    PsmHalDbg(("%s: fail to overwrite cur rec\n", __FUNCTION__));
                break;
            case PSM_OVERWRITE_NEVER:
            default:
                /* nothing */
                break;
            }

            break; /* stop search cur xml recs */
        }

        if (missing)
        {
            PsmHalDbg(("Missing, need to be added ...\n"));
            if (AddRecToXml(&newRec, curXml) != ANSC_STATUS_SUCCESS)
                PsmHalDbg(("%s: fail to add new rec\n", __FUNCTION__));
        }
    }

    PsmHalDbg(("Write back cur/def config ...\n"));

    /* all changes are done, it's time to write back to file */
    if (XmlToFile(curXml, curPath) != ANSC_STATUS_SUCCESS)
        PsmHalDbg(("%s: fail to save cur config\n", __FUNCTION__));
    /* new config means new default config */
    if (XmlToFile(newXml, defPath) != ANSC_STATUS_SUCCESS)
        PsmHalDbg(("%s: fail to save def config\n", __FUNCTION__));

    err = ANSC_STATUS_SUCCESS;

done:
    if (newXml)
        newXml->Remove(newXml);
    if (curXml)
        curXml->Remove(curXml);
    if (defXml)
        defXml->Remove(defXml);

    return err;
}
