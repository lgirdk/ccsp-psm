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

    module: psm_flo_operation.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the advanced operation functions
        of the Psm File Loader Object.

        *   PsmFloEngage
        *   PsmFloCancel

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


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloEngage
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to engage the object activity.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloEngage
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT  )hThisObject;
    //CcspTraceInfo(("PsmFloEngage begins \n"));
    if ( pMyObject->bActive )
    {
        return  ANSC_STATUS_SUCCESS;
    }
    else if( pMyObject->hPsmCfmIf == NULL || pMyObject->hSysIraIf == NULL)
    {
        CcspTraceError(("Failed to engage in 'PsmFloEngage', either 'hPsmCfmIf' or 'hSysIraIf' is not configured yet.\n"));

        return ANSC_STATUS_FAILURE;
    }
    else
    {
        pMyObject->bActive = TRUE;
    }
	 CcspTraceInfo((" Engage in PsmFloEngage \n"));
	 //CcspTraceInfo(("PsmFloEngage ends '\n"));
    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloCancel
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to cancel the object activity.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloCancel
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT  )hThisObject;
    //CcspTraceInfo(("PsmFloCancel begins '\n"));
    if ( !pMyObject->bActive )
    {
    	CcspTraceInfo(("PsmFloCancel, Object is not active so cancelled\n"));
        return  ANSC_STATUS_SUCCESS;
    }
    else
    {
    		CcspTraceInfo(("PsmFloCancel, Object is deactivated\n"));
        pMyObject->bActive = FALSE;
    }

	CcspTraceInfo((" Cancelled in PsmFloCancel\n"));
	//CcspTraceInfo(("PsmFloCancel ends '\n"));
    return  returnStatus;
}
