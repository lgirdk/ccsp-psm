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

#ifndef SSP_UTILS_H
#define SSP_UTILS_H

#include "ccsp_trace.h"

typedef enum 
_PSM_PARSING_LINE_RESULT
{
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
}
PSM_PARSING_LINE_RESULT, *PPSM_PARSING_LINE_RESULT;

#define MAX_NAME_LENGTH 512
#define MAX_VAL_LENGTH  256

PSM_PARSING_LINE_RESULT PsmHashParseOneLine(char *pLine, char **ppName, char **ppValue, char **ppType, char **ppContentType, char **ppNextLine);
int PsmHal_GetCustomParams_1( PsmHalParam_t **params, int *cnt);
ANSC_STATUS PsmReadFile(char * path, char **ppBuf, int *pLen);

#define SSPPSMLITE_POINTER_VALIDATION(param, gotopos) { \
    if (param == NULL ) \
    {  \
        CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));  \
        goto gotopos;  \
    }  \
}

#endif
