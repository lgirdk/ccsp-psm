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

    module: psm_flo_base.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the basic container object functions
        of the Psm Sys Registry Object.

        *   PsmFloCreate
        *   PsmFloRemove
        *   PsmFloEnrollObjects
        *   PsmFloInitialize

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

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        PsmFloCreate
            (
                ANSC_HANDLE                 hContainerContext,
                ANSC_HANDLE                 hOwnerContext,
                ANSC_HANDLE                 hAnscReserved
            );

    description:

        This function constructs the Psm File Loader Object and
        initializes the member variables and functions.

    argument:   ANSC_HANDLE                 hContainerContext
                This handle is used by the container object to interact
                with the outside world. It could be the real container
                or an target object.

                ANSC_HANDLE                 hOwnerContext
                This handle is passed in by the owner of this object.

                ANSC_HANDLE                 hAnscReserved
                This handle is passed in by the owner of this object.

    return:     newly created container object.

**********************************************************************/

ANSC_HANDLE
PsmFloCreate
    (
        ANSC_HANDLE                 hContainerContext,
        ANSC_HANDLE                 hOwnerContext,
        ANSC_HANDLE                 hAnscReserved
    )
{
    UNREFERENCED_PARAMETER(hAnscReserved);
    PANSC_COMPONENT_OBJECT          pBaseObject  = NULL;
    PPSM_FILE_LOADER_OBJECT         pMyObject    = NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PPSM_FILE_LOADER_OBJECT)AnscAllocateMemory(sizeof(PSM_FILE_LOADER_OBJECT));
    	//CcspTraceInfo(("PsmFloCreate begins \n"));
    if ( !pMyObject )
    {
    	CcspTraceInfo(("%s, Failed to Create pMyObject \n",__FUNCTION__));
        return  (ANSC_HANDLE)NULL;
    }
    else
    {
    	//CcspTraceInfo(("%s, Created pMyObject\n",__FUNCTION__));
        pBaseObject = (PANSC_COMPONENT_OBJECT)pMyObject;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    /* AnscCopyString(pBaseObject->Name, PSM_FILE_LOADER_NAME); */

    pBaseObject->hContainerContext = hContainerContext;
    pBaseObject->hOwnerContext     = hOwnerContext;
    pBaseObject->Oid               = PSM_FILE_LOADER_OID;
    pBaseObject->Create            = (ANSC_HANDLE)PsmFloCreate;
    pBaseObject->Remove            = PsmFloRemove;
    pBaseObject->EnrollObjects     = PsmFloEnrollObjects;
    pBaseObject->Initialize        = PsmFloInitialize;

    pBaseObject->EnrollObjects((ANSC_HANDLE)pBaseObject);
    pBaseObject->Initialize   ((ANSC_HANDLE)pBaseObject);

	CcspTraceInfo(("Initialized Common Variables'\n"));
	//CcspTraceInfo(("PsmFloCreate ends '\n"));
    return  (ANSC_HANDLE)pMyObject;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function destroys the object.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
	//CcspTraceInfo(("PsmFloRemove begins \n"));
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT)hThisObject;

    pMyObject->Cancel((ANSC_HANDLE)pMyObject);
    pMyObject->Reset ((ANSC_HANDLE)pMyObject);

    AnscCoRemove((ANSC_HANDLE)pMyObject);
	CcspTraceInfo(("PsmFloRemove Removed pMyObject\n"));
	//CcspTraceInfo(("PsmFloRemove ends\n"));
    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloEnrollObjects
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function enrolls all the objects required by this object.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloEnrollObjects
    (
        ANSC_HANDLE                 hThisObject
    )
{
	//CcspTraceInfo(("PsmFloEnrollObjects begins '\n"));
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT)hThisObject;

    AnscCoEnrollObjects((ANSC_HANDLE)pMyObject);
	CcspTraceInfo(("PsmFloEnrollObjects, Enrolled objectes required by pMyObject'\n"));
	//CcspTraceInfo(("PsmFloEnrollObjects ends '\n"));
    return  ANSC_STATUS_SUCCESS;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmFloInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function first calls the initialization member function
        of the base class object to set the common member fields
        inherited from the base class. It then initializes the member
        fields that are specific to this object.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmFloInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PPSM_FILE_LOADER_OBJECT         pMyObject    = (PPSM_FILE_LOADER_OBJECT)hThisObject;

    /*
     * Until you have to simulate C++ object-oriented programming style with standard C, you don't
     * appreciate all the nice little things come with C++ language and all the dirty works that
     * have been done by the C++ compilers. Member initialization is one of these things. While in
     * C++ you don't have to initialize all the member fields inherited from the base class since
     * the compiler will do it for you, such is not the case with C.
     */
   // CcspTraceInfo(("PsmFloInitialize begins '\n"));
    AnscCoInitialize((ANSC_HANDLE)pMyObject);
    CcspTraceInfo(("PsmFloInitialize, initialization member function of the base class'\n"));

    /*
     * Although we have initialized some of the member fields in the "create" member function, we
     * repeat the work here for completeness. While this simulation approach is pretty stupid from
     * a C++/Java programmer perspective, it's the best we can get for universal embedded network
     * programming. Before we develop our own operating system (don't expect that to happen any
     * time soon), this is the way things gonna be.
     */
    pMyObject->Oid           = PSM_FILE_LOADER_OID;
    pMyObject->Create        = (ANSC_HANDLE)PsmFloCreate;
    pMyObject->Remove        = PsmFloRemove;
    pMyObject->EnrollObjects = PsmFloEnrollObjects;
    pMyObject->Initialize    = PsmFloInitialize;

    pMyObject->hPsmCfmIf     = (ANSC_HANDLE)NULL;
    pMyObject->hSysIraIf     = (ANSC_HANDLE)NULL;
    pMyObject->bActive       = FALSE;

    pMyObject->GetPsmCfmIf   = PsmFloGetPsmCfmIf;
    pMyObject->SetPsmCfmIf   = PsmFloSetPsmCfmIf;
    pMyObject->GetSysIraIf   = PsmFloGetSysIraIf;
    pMyObject->SetSysIraIf   = PsmFloSetSysIraIf;

    pMyObject->GetProperty   = PsmFloGetProperty;
    pMyObject->SetProperty   = PsmFloSetProperty;
    pMyObject->ResetProperty = PsmFloResetProperty;
    pMyObject->Reset         = PsmFloReset;

    pMyObject->Engage        = PsmFloEngage;
    pMyObject->Cancel        = PsmFloCancel;

    pMyObject->TestRegFile   = PsmFloTestRegFile;
    pMyObject->LoadRegFile   = PsmFloLoadRegFile;
    pMyObject->SaveRegFile   = PsmFloSaveRegFile;

    /*
     * We shall initialize the object properties to the default values, which may be changed later
     * via the exposed member functions. If any of the future extensions needs to change the object
     * property, the following code also needs to be changed.
     */
    pMyObject->ResetProperty((ANSC_HANDLE)pMyObject);
	CcspTraceInfo(("PsmFloInitialize, initialization of member fields'\n"));
	 //   CcspTraceInfo(("PsmFloInitialize ends '\n"));
    return  ANSC_STATUS_SUCCESS;
}
