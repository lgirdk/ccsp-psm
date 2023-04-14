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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ccsp_message_bus.h>
#include <ccsp_base_api.h>
#include <sys/time.h>
#include <time.h>
#include <slap_definitions.h>
#include <ccsp_psm_helper.h>
#include "ssp_global.h"
#include "safec_lib_common.h"
#include "ansc_tso_interface.h"
extern  void *bus_handle;
extern  PPSM_SYS_REGISTRY_OBJECT  pPsmSysRegistry;
extern  char  g_Subsystem[32];
extern  char* pComponentName;
extern  BOOL  g_bLogEnable;
#define  CCSP_COMMON_COMPONENT_HEALTH_Red                   1
#define  CCSP_COMMON_COMPONENT_HEALTH_Yellow                2
#define  CCSP_COMMON_COMPONENT_HEALTH_Green                 3

#define  MALLOC_EIGHT_BYTES                            8
#define  MALLOC_SIXTEEN_BYTES                          16
#define  MALLOC_THIRTYTWO_BYTES                        32
#define  MALLOC_ONE_TWENTY_EIGHT_BYTES                 128
#define  MALLOC_TWO_FIFTY_SIX_BYTES                    256

#define  COMPVALUES_SET                                3

BOOLEAN waitConditionReady
    (
        void*                           hMBusHandle,
        const char*                     dst_component_id,
        char*                           dbus_path,
        char*                           src_component_id
    );
int     g_psmHealth = CCSP_COMMON_COMPONENT_HEALTH_Red;
PDSLH_CPE_CONTROLLER_OBJECT     pDslhCpeController        = NULL;

char g_NewConfigPath[256] = "";

typedef  struct
_PARAMETER_INFO
{
    SINGLE_LINK_ENTRY               Linkage;
    parameterInfoStruct_t          *val;
}
PARAMETER_INFO,  *PPARAMETER_INFO;

typedef  struct
_PARAMETER_VALUE
{
    SINGLE_LINK_ENTRY               Linkage;
    parameterValStruct_t           *val;
}
PARAMETER_VALUE,  *PPARAMETER_VALUE;

enum psmIndex_e{
    NONE = 0,
    FACTORY_RESET,
    PSM_NAME,
    PSM_VERSION,
    PSM_AUTHOR,
    PSM_HEALTH,
    PSM_STATE,
    PSM_DTXML,
    PSM_DISABLE_WRITING,
    PSM_LOGGING_ENABLE,
    PSM_LOG_LEVEL,
    PSM_MEM_MINUSAGE,
    PSM_MEM_MAXUSAGE,
    PSM_MEM_CONSUMED,
    PSM_RELOAD_CONFIG,
    PSM_UPDATE_CONFIG,
    PSM_NEW_CONFIGPATH,
};

name_spaceType_t NamespacePsm[] =
{
    {"com.cisco.spvtg.ccsp.psm", ccsp_none},
    {"com.cisco.spvtg.ccsp.command.FactoryReset", ccsp_boolean},
    {"com.cisco.spvtg.ccsp.psm.Name", ccsp_string},
    {"com.cisco.spvtg.ccsp.psm.Version", ccsp_unsignedInt},
    {"com.cisco.spvtg.ccsp.psm.Author", ccsp_string},
    {"com.cisco.spvtg.ccsp.psm.Health", ccsp_string},
    {"com.cisco.spvtg.ccsp.psm.State", ccsp_unsignedInt},
    {"com.cisco.spvtg.ccsp.psm.DTXml", ccsp_string},
    {"com.cisco.spvtg.ccsp.psm.DisableWriting", ccsp_boolean},
    {"com.cisco.spvtg.ccsp.psm.Logging.Enable", ccsp_boolean},
    {"com.cisco.spvtg.ccsp.psm.Logging.LogLevel", ccsp_unsignedInt},
    {"com.cisco.spvtg.ccsp.psm.Memory.MinUsage", ccsp_unsignedInt},
    {"com.cisco.spvtg.ccsp.psm.Memory.MaxUsage", ccsp_unsignedInt},
    {"com.cisco.spvtg.ccsp.psm.Memory.Consumed", ccsp_unsignedInt},
    {"com.cisco.spvtg.ccsp.psm.ReloadConfig", ccsp_boolean},
    {"com.cisco.spvtg.ccsp.psm.UpdateConfigs", ccsp_boolean},
    {"com.cisco.spvtg.ccsp.psm.NewConfigPath", ccsp_string},
};

static const char* PSM_Introspect_msg =
    "<xml version=\"1.0\" encoding=\"UTF-8\">\n"
    "<node name=\"/com/cisco/spvtg/ccsp/PersistentStorage\">\n"
    "    <interface name=\"com.cisco.spvtg.ccsp.baseInterface\">\n"
    "        \n"
    "        <method name=\"initialize\">\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        <method name=\"finalize\">\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!--\n"
    "            This API frees up resources such as allocated memory, flush caches etc, if possible. \n"
    "            This is invoked by Test and Diagnostic Manager, as a proactive measure, when it \n"
    "            detects low memory conditions.     \n"
    "        -->\n"
    "        <method name=\"freeResources\">\n"
    "            <arg type=\"i\" name=\"priority\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!--\n"
    "            DEPRECATED\n"
    "            This API is used to retrieve the Component Metadata. The Component Metadata \n"
    "            includes the following information: \n"
    "            - Component Name\n"
    "            - Component Author \n"
    "            - Component Version \n"
    "        -->\n"
    "        <method name=\"getComponentMetadata\">\n"
    "            <arg type=\"s\" name=\"component_name\" direction=\"out\" />\n"
    "            <arg type=\"s\" name=\"component_author\" direction=\"out\" />\n"
    "            <arg type=\"s\" name=\"component_version\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!-- \n"
    "            DEPRECATED \n"
    "            Logging APIs  \n"
    "        -->\n"
    "        <method name=\"enableLogging\">\n"
    "            <arg type=\"b\" name=\"enable\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        <!-- DEPRECATED   -->\n"
    "        <method name=\"setLoggingLevel\">\n"
    "            <arg type=\"i\" name=\"level\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "\n"
    "        <!-- \n"
    "            DEPRECATED\n"
    "            This API returns the internal state of the component. The state reflects the \n"
    "            Component\'s internal lifecycle state\n"
    "        -->\n"
    "        <method name=\"queryStatus\">\n"
    "            <arg type=\"i\" name=\"internalState\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method> \n"
    "        \n"
    "        <!-- \n"
    "            DEPRECATED \n"
    "            This API returns the health of the component as \'Red/Bad\', \'Yellow/warning\', \'Green/good\' \n"
    "        -->\n"
    "        <method name=\"healthCheck\">\n"
    "            <arg type=\"i\" name=\"health\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>  \n"
    "        \n"
    "        <!--\n"
    "            DEPRECATED\n"
    "            This API returns the amount of direct memory allocated by the component. Typically the \n"
    "            Process statistics can be retrieved by querying the /proc/<PID> file system under    \n"
    "            Linux OS. However, in cases where more than one component are grouped into a \n"
    "            single process, this API provides component level memory usage which can be very \n"
    "            useful to isolate low memory conditions. \n"
    "        -->\n"
    "        <method name=\"getAllocatedMemory\">\n"
    "            <arg type=\"i\" name=\"directAllocatedMemory\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>  \n"
    "        \n"
    "        <!-- \n"
    "            DEPRECATED\n"
    "            This API returns the mmaximum memory requirements for the component. It is the \n"
    "            component owner\'s best estimates   \n"
    "        -->\n"
    "        <method name=\"getMaxMemoryUsage\">\n"
    "            <arg type=\"i\" name=\"memoryUsage\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>  \n"
    " \n"
    "        <!-- \n"
    "            DEPRECATED\n"
    "            This API returns the minimum memory requirements for the component. It is the \n"
    "            component owner\'s best estimates   \n"
    "        -->\n"
    "        <method name=\"getMinMemoryUsage\">\n"
    "            <arg type=\"i\" name=\"memoryUsage\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>  \n"
    " \n"
    "        <!-- Data model parameters \"set\" APIs\n"
    "            typedef struct {\n"
    "                const char *parameterName; \n"
    "                unsigned char *parameterValue;\n"
    "                dataType_e type; \n"
    "            } parameterValStruct_t; \n"
    "            \n"
    "            typedef enum {\n"
    "                ccsp_string = 0, \n"
    "                ccsp_int,\n"
    "                ccsp_unsignedInt,\n"
    "                ccsp_boolean,\n"
    "                ccsp_dateTime,\n"
    "                ccsp_base64,\n"
    "                ccsp_long, \n"
    "                ccsp_unsignedLong, \n"
    "                ccsp_float, \n"
    "                ccsp_double,\n"
    "                ccsp_byte,  // char \n"
    "                (any other simple type that I may have missed),   \n"
    "                ccsp_none \n"
    "            } datatype_e\n"
    "        -->\n"
    "        <method name=\"setParameterValues\">\n"
    "            <arg type=\"i\" name=\"sessionId\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"writeID\" direction=\"in\" />\n"
    "            <arg type=\"a(ssi)\" name=\"parameterValStruct\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"size\" direction=\"in\" />\n"
    "            <arg type=\"b\" name=\"commit\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <method name=\"setCommit\">\n"
    "            <arg type=\"i\" name=\"sessionId\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"writeID\" direction=\"in\" />\n"
    "            <arg type=\"b\" name=\"commit\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "                \n"
    "        <!-- Data model parameters \"get\" APIs   -->\n"
    "        <method name=\"getParameterValues\">\n"
    "            <arg type=\"as\" name=\"parameterNames\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"size\" direction=\"in\" />\n"
    "            <arg type=\"a(ss)\" name=\"parameterValStruct\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!-- \n"
    "            This API sets the attributes on data model parameters\n"
    "            typedef struct { \n"
    "                const char* parameterName; \n"
    "                boolean notificationChanged; \n"
    "                boolean notification; \n"
    "                enum access_e access; // (CCSP_RO, CCSP_RW, CCSP_WO)\n"
    "                boolean accessControlChanged; \n"
    "                unsigned int accessControlBitmask;\n"
    "                  //  0x00000000 ACS\n"
    "                  //  0x00000001 XMPP\n"
    "                  //  0x00000002 CLI\n"
    "                  //  0x00000004 WebUI\n"
    "                  //  ... \n"
    "                  //  0xFFFFFFFF  ANYBODY (reserved and default value for all parameters)\n"
    "           } parameterAttribStruct_t; \n"
    "        -->\n"
    "        <method name=\"setParameterAttributes\">\n"
    "            <arg type=\"i\" name=\"sessionId\" direction=\"in\" />\n"
    "            <arg type=\"a(sbbibi)\" name=\"parameterAttributeStruct\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"size\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <method name=\"getParameterAttributes\">\n"
    "            <arg type=\"as\" name=\"parameterNames\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"size\" direction=\"in\" />\n"
    "            <arg type=\"a(sbii)\" name=\"parameterAttributeStruct\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!-- \n"
    "            This API adds a row to a table object. The object name is a partial path \n"
    "            and must end with a \".\" (dot). The API returns the instance number of the \n"
    "            row.\n"
    "        -->\n"
    "        <method name=\"AddTblRow\">\n"
    "            <arg type=\"i\" name=\"sessionId\" direction=\"in\" />\n"
    "            <arg type=\"s\" name=\"objectName\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"instanceNumber\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!-- \n"
    "            This API deletes a row from the table object. The object name is a partial \n"
    "            path and must end with a \".\" (dot) after the instance number.\n"
    "        -->\n"
    "        <method name=\"DeleteTblRow\">\n"
    "            <arg type=\"i\" name=\"sessionId\" direction=\"in\" />\n"
    "            <arg type=\"s\" name=\"objectName\" direction=\"in\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!--\n"
    "            This API is used to return the supported parameter names under a data model object\n"
    "            parameterName is either a complete Parameter name, or a partial path name of an object.            \n"
    "            nextLevel \n"
    "                If false, the response MUST contain the Parameter or object whose name exactly\n"
    "                matches the ParameterPath argument, plus all Parameters and objects that are\n"
    "                descendents of the object given by the ParameterPath argument, if any (all levels\n"
    "                below the specified object in the object hierarchy).\n"
    "            \n"
    "                If true, the response MUST contain all Parameters and objects that are next-level\n"
    "                children of the object given by the ParameterPath argument, if any.\n"
    "            parameterInfoStruct is defined as: \n"
    "                typedef struct {\n"
    "                    comst char *name; \n"
    "                    boolean writable; \n"
    "                }\n"
    "        -->\n"
    "        <method name=\"getParameterNames\">\n"
    "            <arg type=\"s\" name=\"parameterName\" direction=\"in\" />\n"
    "            <arg type=\"b\" name=\"nextLevel\" direction=\"in\" />\n"
    "            <arg type=\"a(sb)\" name=\"parameterInfoStruct\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!-- \n"
    "            This API is used in diagnostic mode. This must be used asynchronously. \n"
    "            The use case is that the Test and Diagnostic Manager (TDM) CCSP component can leverage this feature \n"
    "            in the Component Registrar to validate parameter types. The TDM sends commands to other components to \n"
    "            run diagnostics. The TDM invokes a buscheck() request to each component one at a time in diagnostic mode. \n"
    "            When each component receives buscheck(), it invokes the namespace type check API in the Component \n"
    "            Registrar for each of the data model parameters accessed by this component and owned by another component. \n"
    "            The Component Registrar verifies that each data model parameter is registered by a component and that the \n"
    "            data model type specified in the API is the same as the data model type registered by the \'owner\' component. \n"
    "            The component sends TDM a response to buscheck() with all checked parameter names and PASS/FAIL for each \n"
    "            parameter. If during buscheck(), it is found that there are missing or unregistered parameters, \n"
    "            appropriate errors are flagged. \n"
    "        -->\n"
    "        <method name=\"busCheck\">\n"
    "            <arg type=\"i\" name=\"status\" direction=\"out\" />\n"
    "        </method>\n"
    "        \n"
    "        <!--\n"
    "            Signal contains the following information: \n"
    "            typedef struct {\n"
    "                const char *parameterName; \n"
    "                const char* oldValue; \n"
    "                const char* int newValue;\n"
    "                unsigned int writeID; \n"
    "            } parameterSigStruct_t; \n"
    "        -->\n"
    "        <signal name=\"parameterValueChangeSignal\">\n"
    "            <arg type=\"a(sssi)\" name=\"parameterSigStruct\" direction=\"out\" />\n"
    "            <arg type=\"i\" name=\"size\" direction=\"out\" />\n"
    "        </signal>\n"
    "        \n"
    "       \n"
    "    </interface>\n"
    "</node>\n"
    ;

int get_psm_type_from_name(char *name, enum dataType_e *type_ptr, enum psmIndex_e *index)
{
  int rc = -1;
  int ind = -1;
  unsigned int i = 0;
  if((name == NULL) || (type_ptr == NULL) || (index == NULL))
     return 0;
  for (i = 0 ; i < sizeof(NamespacePsm)/sizeof(name_spaceType_t) ; ++i)
  {
      rc = strcmp_s(name, strlen(name), NamespacePsm[i].name_space, &ind);
      ERR_CHK(rc);
      if((rc == EOK) && (!ind))
      {
          *type_ptr = NamespacePsm[i].dataType;
          *index = i;
          return 1;
      }
  }
  return 0;
}

int getCompareValue(char *name)
{
    const char *compValues[COMPVALUES_SET] = {"1", "true", "TRUE"};

    int i = 0;
    int rc = -1;
    int ind = -1;

    if(name == NULL)
         return 0;
    for(i = 0; i < COMPVALUES_SET; i++)
    {
       rc = strcmp_s(compValues[i], strlen(compValues[i]), name, &ind);
       ERR_CHK(rc);
       if((rc == EOK) && (!ind))
       {
	   return 1;
       }
    }
    return 0;
}


void free_commParam_pointers(PPARAMETER_VALUE pParameterValue)
{
    if(pParameterValue)
    {
        if(pParameterValue->val)
        {
            if(pParameterValue->val->parameterName)
            {
                free(pParameterValue->val->parameterName);
            }

            if(pParameterValue->val->parameterValue)
            {
                free(pParameterValue->val->parameterValue);
            }
            free(pParameterValue->val);
        }
        free(pParameterValue);
    }
    return;
}


ANSC_STATUS getCommParam(
        char *paramName,
        PPARAMETER_VALUE *ppParameterValue
)
{
    PPARAMETER_VALUE    pParameterValue;
    enum dataType_e type;
    enum psmIndex_e index;
    errno_t rc = -1;
   /* CcspTraceInfo((" inside getCommParam\n")); */
    pParameterValue = AnscAllocateMemory(sizeof(PARAMETER_VALUE));
    if(pParameterValue == NULL)
    {
        CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pParameterValue->val = AnscAllocateMemory(sizeof(parameterValStruct_t));
    if(pParameterValue->val == NULL)
    {
        CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
        free_commParam_pointers(pParameterValue);
        return ANSC_STATUS_FAILURE;
    }

    pParameterValue->val->parameterName = AnscAllocateMemory(strlen(paramName)+1);
    if(pParameterValue->val->parameterName == NULL)
    {
        CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
        free_commParam_pointers(pParameterValue);
        return ANSC_STATUS_FAILURE;
    }
    rc = strcpy_s(pParameterValue->val->parameterName, strlen(paramName)+1,  paramName);
    if(rc != EOK)
    {
	ERR_CHK(rc);
        free_commParam_pointers(pParameterValue);
	return ANSC_STATUS_FAILURE;
    }

    if (get_psm_type_from_name(paramName, &type, &index))
    {
	    pParameterValue->val->type = type;

	    if(index == FACTORY_RESET)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }

                rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "false");
		if(rc != EOK)
		{
		    ERR_CHK(rc);
                    free_commParam_pointers(pParameterValue);
		    return ANSC_STATUS_FAILURE;
		}
	    }
	    else if(index == PSM_NAME)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_ONE_TWENTY_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }

		rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_ONE_TWENTY_EIGHT_BYTES, pComponentName);
		if(rc != EOK)
		{
                    ERR_CHK(rc);
                    free_commParam_pointers(pParameterValue);
		    return ANSC_STATUS_FAILURE;
		}
	    }
	    else if(index == PSM_VERSION)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }
		sprintf(pParameterValue->val->parameterValue, "%d", CCSP_COMPONENT_VERSION_PSM);
	    }
	    else if(index == PSM_AUTHOR)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_THIRTYTWO_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }

                rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_THIRTYTWO_BYTES, CCSP_COMPONENT_AUTHOR_PSM);
		if(rc != EOK)
		{
		    ERR_CHK(rc);
                    free_commParam_pointers(pParameterValue);
		    return ANSC_STATUS_FAILURE;
		}
	    }
	    else if(index == PSM_HEALTH)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }

                rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES,  "Green");
		if(rc != EOK)
		{
                    ERR_CHK(rc);
                    free_commParam_pointers(pParameterValue);
		    return ANSC_STATUS_FAILURE;
		}
	    }
	    else if(index == PSM_STATE)
            {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }
		sprintf(pParameterValue->val->parameterValue, "%d", 1);
	    }
	    else if(index == PSM_DTXML)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }

		rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "Null");
		if(rc != EOK)
		{
		    ERR_CHK(rc);
                    free_commParam_pointers(pParameterValue);
		    return ANSC_STATUS_FAILURE;
		}
	    }
	    else if(index == PSM_DISABLE_WRITING)
	    {
		pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                if(pParameterValue->val->parameterValue == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                }

                if ( pPsmSysRegistry && (pPsmSysRegistry->FileSyncRefCount > 0) )
		{
		    rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "true");
		    if(rc != EOK)
		    {
			ERR_CHK(rc);
                        free_commParam_pointers(pParameterValue);
			return ANSC_STATUS_FAILURE;
		    }
		}
		else
		{
		    rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "false");
		    if(rc != EOK)
		    {
			ERR_CHK(rc);
                        free_commParam_pointers(pParameterValue);
			return ANSC_STATUS_FAILURE;
		    }
		}
	     }
	     else if(index == PSM_LOGGING_ENABLE)
	     {
		 pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }

		 if ( g_bLogEnable )
		 {
                     rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "true");
		     if(rc != EOK)
		     {
                         ERR_CHK(rc);
                         free_commParam_pointers(pParameterValue);
			 return ANSC_STATUS_FAILURE;
		     }
		 }
		 else
		 {
		     rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "false");
		     if(rc != EOK)
		     {
			 ERR_CHK(rc);
                         free_commParam_pointers(pParameterValue);
			 return ANSC_STATUS_FAILURE;
		     }
		 }
	     }
	     else if(index == PSM_LOG_LEVEL)
	     {
                 pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_ONE_TWENTY_EIGHT_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }
		 sprintf(pParameterValue->val->parameterValue, "%d", g_iTraceLevel);
	     }
	     else if(index == PSM_MEM_MINUSAGE)
	     {
                 pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_ONE_TWENTY_EIGHT_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }
		 sprintf(pParameterValue->val->parameterValue, "%d", 0);
	     }
	     else if(index == PSM_MEM_MAXUSAGE)
	     {
                 pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_ONE_TWENTY_EIGHT_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }
		 sprintf(pParameterValue->val->parameterValue, "%lu", g_ulAllocatedSizePeak);
	     }
	     else if(index == PSM_MEM_CONSUMED)
	     {
		 LONG  lMemSize = 0;

		 lMemSize = AnscGetComponentMemorySize(pComponentName);

		 if ( lMemSize == -1 )
		     lMemSize = 0;

		 pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_ONE_TWENTY_EIGHT_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }
		 sprintf(pParameterValue->val->parameterValue, "%lu", (ULONG)lMemSize);
	     }
	     else if(index == PSM_RELOAD_CONFIG)
	     {
		 pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_EIGHT_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }

		 rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_EIGHT_BYTES, "false");
		 if(rc != EOK)
		 {
		     ERR_CHK(rc);
                     free_commParam_pointers(pParameterValue);
		     return ANSC_STATUS_FAILURE;
		}
	     }
             else if(index == PSM_UPDATE_CONFIG)
	     {
	         pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_SIXTEEN_BYTES);
                 if(pParameterValue->val->parameterValue == NULL)
                 {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                 }

		 rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_SIXTEEN_BYTES, "false");
		 if(rc != EOK)
		 {
		     ERR_CHK(rc);
                     free_commParam_pointers(pParameterValue);
		     return ANSC_STATUS_FAILURE;
		 }
	      }
	      else if(index == PSM_NEW_CONFIGPATH)
	      {
	          pParameterValue->val->parameterValue = AnscAllocateMemory(MALLOC_TWO_FIFTY_SIX_BYTES);
                  if(pParameterValue->val->parameterValue == NULL)
                  {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free_commParam_pointers(pParameterValue);
                      return ANSC_STATUS_FAILURE;
                  }
		  char *pNewConfigPath = g_NewConfigPath;
		  rc = strcpy_s(pParameterValue->val->parameterValue, MALLOC_TWO_FIFTY_SIX_BYTES, pNewConfigPath);
		  if(rc != EOK)
		  {
		      ERR_CHK(rc);
                      free_commParam_pointers(pParameterValue);
		      return ANSC_STATUS_FAILURE;
		  }
	      }
     }

    *ppParameterValue = pParameterValue;
   /* CcspTraceInfo((" getCommParam exit\n"));  */
    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS doFactoryResetTask
    (        
        ANSC_HANDLE     hContext
    )
{
    PPSM_SYS_REGISTRY_OBJECT        pSroHandle     = (PPSM_SYS_REGISTRY_OBJECT)hContext;
    ANSC_STATUS                     returnStatus   = ANSC_STATUS_SUCCESS;
    char                            CrName[256];
    parameterValStruct_t            val[1];
    /* Coverity Issue Fix - CID:119052 : UnInitialised variable */
    char*                           pStr = NULL;
    errno_t                         rc = -1;

    /* factory reset the PSM */
    returnStatus = pSroHandle->ResetToFactoryDefault((ANSC_HANDLE)pSroHandle);
   CcspTraceInfo((" doFactoryResetTask begins\n"));
    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        PsmHal_RestoreFactoryDefaults();

        /* reboot the box */
        if ( g_Subsystem[0] != 0 )
        {
            _ansc_sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
        }
        else
        {
            rc = strcpy_s(CrName, sizeof(CrName), CCSP_DBUS_INTERFACE_CR);
            if(rc != EOK)
	    {
		ERR_CHK(rc);
		return ANSC_STATUS_FAILURE;
	    }
        }

        val[0].parameterName  = "com.cisco.spvtg.ccsp.rm.Reboot.Enable";
        val[0].parameterValue = "true";
        val[0].type           = ccsp_boolean;

        CcspBaseIf_SetRemoteParameterValue
            (
                bus_handle,
                CrName,
                "com.cisco.spvtg.ccsp.rm.Reboot.Enable",
                g_Subsystem,
                0,
                0xFFFF,
                val,
                1,
                1,
                &pStr
            );

        if ( pStr )
        {
            AnscFreeMemory(pStr);
        }
    }
   CcspTraceInfo((" doFactoryResetTask exit\n"));
    return returnStatus;
}


int  doFactoryReset
    (
        ANSC_HANDLE hContext
    )
{
   CcspTraceInfo((" doFactoryReset begins\n"));
    /* since reboot will invoke from reboot manager, need to start another thread to make new DBus call */
    AnscSpawnTask
        (
            (void*)doFactoryResetTask,
            hContext,
            "CcspPsmFactoryResetTask"
        );
   CcspTraceInfo((" doFactoryReset exit\n"));
    return 0;
}


int  getParameterValues(
    unsigned int writeID,
    char * parameterNames[],
    int size,
    int *val_size,
    parameterValStruct_t ***param_val,
    void * user_data
)
{
    UNREFERENCED_PARAMETER(writeID);
    UNREFERENCED_PARAMETER(user_data);
    parameterValStruct_t          **val                 = NULL;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    ULONG                           ulRecordType;
    ULONG                           ulRecordSize;
    PSYS_RRO_RENDER_ATTR            pRroRenderAttr      = (PSYS_RRO_RENDER_ATTR)NULL;
    PPARAMETER_VALUE                pParameterValue     = (PPARAMETER_VALUE)NULL;
    SLIST_HEADER                    ParameterValueList;
    int                             i;
    errno_t                         rc                  = -1;
    int                             ind                 = -1;
    int                             ret                 = CCSP_SUCCESS;
 //  CcspTraceInfo((" inside getParameterValues\n"));
    if ( g_psmHealth != CCSP_COMMON_COMPONENT_HEALTH_Green )
    {
        CcspTraceInfo(("!!! PSM is not ready !!!\n"));
        return CCSP_FAILURE;
    }
    if ( pPsmSysRegistry == NULL )
    {
    	CcspTraceInfo(("getParameterValues- pPsmSysRegistry is NULL\n"));
        return CCSP_FAILURE;
    }

    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;

    if ( pSysInfoRepository == NULL )
    {
       	CcspTraceInfo(("getParameterValues- pSysInfoRepository is NULL\n"));
        return CCSP_FAILURE;
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
        CcspTraceInfo(("getParameterValues- hSysRoot is NULL\n"));
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        return CCSP_FAILURE;
    }

    AnscSListInitializeHeader(&ParameterValueList);
    *val_size = 0;

    for ( i = 0; i < size; i++ )
    {
        int bComm = 0;
        unsigned int k= 0;

        for ( k = 1; k < sizeof(NamespacePsm)/sizeof(name_spaceType_t); k++ )
        {
            rc = strcmp_s(parameterNames[i], sizeof(parameterNames[i]), NamespacePsm[k].name_space, &ind);
	    ERR_CHK(rc);
	    if((rc == EOK) && (ind == 0))
            {
                returnStatus = getCommParam(parameterNames[i], &pParameterValue);

                *val_size = *val_size + 1;
                AnscSListPushEntry(&ParameterValueList, &pParameterValue->Linkage);

                bComm = 1;

                break;
            }
        }

        if ( bComm )
        {
            continue;
        }
        
        //CcspTraceWarning(("call get record value for %s +++\n", parameterNames[i]));
        returnStatus =
            pSysIraIf->GetRecord
                (
                    pSysIraIf->hOwnerContext,
                    hSysRoot,
                    parameterNames[i],
                    &ulRecordType,
                    NULL,
                    NULL,
                    &ulRecordSize
                );

        if ( returnStatus != ANSC_STATUS_BAD_SIZE )
        {
            
            if(returnStatus != ANSC_STATUS_SUCCESS)
            {
                CcspTraceWarning(("++++ getParameterValues Failed for %s , returnStatus %lu +++\n", parameterNames[i], returnStatus));
                //CcspTraceWarning(("getParameterValues- returnStatus %d\n",returnStatus));
                //CcspTraceInfo(("Release Thread Lock %d\n"));
                pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
                /*Coverity Fix CID:57874 RESOURCE_LEAK */
                if(pParameterValue != NULL)
		{
			AnscFreeMemory(pParameterValue);
		}
                ret = CCSP_FAILURE;
                goto EXIT;
            }
        }
        else
        {
            *val_size = *val_size + 1;
            returnStatus = ANSC_STATUS_FAILURE;

            if((pParameterValue = AnscAllocateMemory(sizeof(PARAMETER_VALUE))))
            {
                if((pParameterValue->val = AnscAllocateMemory(sizeof(parameterValStruct_t))))
                {
                    if((pParameterValue->val->parameterName = AnscAllocateMemory(strlen(parameterNames[i])+1)))
                    {
                        rc = strcpy_s(pParameterValue->val->parameterName, strlen(parameterNames[i])+1, parameterNames[i]);
			if(rc != EOK)
			{
		            ERR_CHK(rc);
                            free_commParam_pointers(pParameterValue);
			    ret = CCSP_FAILURE;
			    goto EXIT;
			}
                        if((pParameterValue->val->parameterValue = AnscAllocateMemory(ulRecordSize+1)))
                        {
                            returnStatus =
                                pSysIraIf->GetRecord
                                    (
                                        pSysIraIf->hOwnerContext,
                                        hSysRoot,
                                        parameterNames[i],
                                        &ulRecordType,
                                        (PANSC_HANDLE)&pRroRenderAttr,
                                        pParameterValue->val->parameterValue,
                                        &ulRecordSize
                                    );
                            if(returnStatus != ANSC_STATUS_SUCCESS)
                            {
                                //CcspTraceWarning(("++++ Failed for %s +++\n", parameterNames[i]));
                                CcspTraceInfo(("getParameterValues- returnStatus %lu\n",returnStatus));
                               // CcspTraceInfo(("Release Thread Lock %d\n"));

                                /* RDKB-6908, CID-33005, free unused resource before exit, 
                                ** if checks are added to avoid crashed in case malloc fails.
                                */
                                pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
                                AnscFreeMemory(pParameterValue->val->parameterValue);
                                AnscFreeMemory(pParameterValue->val->parameterName);
                                AnscFreeMemory(pParameterValue->val);
                                AnscFreeMemory(pParameterValue);
                                ret = CCSP_FAILURE;
                                goto EXIT;
                            }
                            pParameterValue->val->type = pRroRenderAttr->ContentType;
                            AnscSListPushEntry(&ParameterValueList, &pParameterValue->Linkage);
                        }
                        else
                        {
                            AnscFreeMemory(pParameterValue->val->parameterName);
                            AnscFreeMemory(pParameterValue->val);
                            AnscFreeMemory(pParameterValue);
                        }
                    }
                    else
                    {
                        AnscFreeMemory(pParameterValue->val);
                        AnscFreeMemory(pParameterValue);
                    }
                }
                else
                {
                    AnscFreeMemory(pParameterValue);
                }
            }
        }
    }

    if ( *val_size > 0 )
    {
        int                     k           = *val_size;
        PSINGLE_LINK_ENTRY      pSLinkEntry = NULL;

        val = AnscAllocateMemory(*val_size*sizeof(parameterValStruct_t *));
        if(val == NULL)
        {
             CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
             ret = CCSP_FAILURE;
             goto EXIT;
        }
        rc = memset_s(val, *val_size*sizeof(parameterValStruct_t *), 0, *val_size*sizeof(parameterValStruct_t *));
	ERR_CHK(rc);

        for ( ; k > 0; k-- )
        {
            pSLinkEntry = AnscSListPopEntry(&ParameterValueList);
            pParameterValue = ACCESS_CONTAINER(pSLinkEntry, PARAMETER_VALUE, Linkage);
            val[k-1] = pParameterValue->val;
            AnscFreeMemory(pParameterValue);

            /* Security Requiremnt: Log messages must not disclose any confidential data
               like cryptographic keys and password. So don't save Passphrase on log message.
             */
            if ( NULL == strstr(val[k-1]->parameterName, "Passphrase" ) ) {
                CcspTraceDebug(("getParameterValues -- *val_size:%d, %s: %s\n", *val_size, val[k-1]->parameterName, val[k-1]->parameterValue ));
           }
           else {
                CcspTraceDebug(("getParameterValues – Not printing the value of parameter %s as it will disclose the confidential information.\n", val[k-1]->parameterName ));
           }
        }
    }

    *param_val = val;

EXIT:

    if ( hSysRoot )
    {
          // CcspTraceInfo((" getParameterValues -hSysRoot\n"));
        pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);
    }

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
     //  CcspTraceInfo((" getParameterValues exit\n"));
    return ret;
}


int  setParameterValues(
    int sessionId,
    unsigned int writeID,
    parameterValStruct_t *val,
    int size,
    dbus_bool commit,
    char **str,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(sessionId);
    UNREFERENCED_PARAMETER(writeID);
    UNREFERENCED_PARAMETER(commit);
    UNREFERENCED_PARAMETER(str);
    UNREFERENCED_PARAMETER(user_data);
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    SYS_RRO_RENDER_ATTR             rroRenderAttr;
    PPSM_FILE_LOADER_OBJECT         pPsmFileLoader      = (PPSM_FILE_LOADER_OBJECT    )pPsmSysRegistry->hPsmFileLoader;
    PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj    = (PANSC_TIMER_DESCRIPTOR_OBJECT)pPsmSysRegistry->hRegTimerObj;
    int                             i;
    char		oldValBuf[512] = {0};
    ULONG		ulRecordType;
    ULONG		ulRecordSize;
    errno_t             rc = -1;
    int                 ind = -1;

    //   CcspTraceInfo((" inside setParameterValues \n"));
    if ( g_psmHealth != CCSP_COMMON_COMPONENT_HEALTH_Green )
    {
        CcspTraceInfo(("!!! PSM is not ready !!!\n"));
        return CCSP_FAILURE;
    }
    if ( pPsmSysRegistry == NULL )
    {
       CcspTraceInfo(("setParameterValues- pPsmSysRegistry is NULL\n"));
        return CCSP_FAILURE;
    }

    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;

    if ( pSysInfoRepository == NULL )
    {
           CcspTraceInfo(("setParameterValues- pSysInfoRepository is NULL\n"));
        return CCSP_FAILURE;
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
               CcspTraceInfo(("setParameterValues- hSysRoot is NULL\n"));
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        return CCSP_FAILURE;
    }
 
    SysInitRroRenderAttr((&rroRenderAttr));

    enum dataType_e type;
    //int index = 0;
    enum psmIndex_e index;

    for ( i = 0; i < size; i++ )
    {
	if (get_psm_type_from_name(val[i].parameterName, &type, &index))
        {
	    if(index == FACTORY_RESET)
	    {
		if(getCompareValue(val[i].parameterValue))
		{
                    doFactoryReset((ANSC_HANDLE)pPsmSysRegistry);
		}
		continue;
	    }
	    else if(index == PSM_LOGGING_ENABLE)
	    {
                if(getCompareValue(val[i].parameterValue))
                {
                    g_bLogEnable = TRUE;
		}
		else
		{
		    g_bLogEnable = FALSE;
		}
		continue;
	    }
	    else if(index == PSM_LOG_LEVEL)
	    {
		g_iTraceLevel = strtol(val[i].parameterValue, NULL, 10);

                continue;
	    }
	    else if(index == PSM_DISABLE_WRITING)
	    {
                if(getCompareValue(val[i].parameterValue))
		{
                    pPsmSysRegistry->SysRamEnableFileSync((ANSC_HANDLE)pPsmSysRegistry, TRUE);
                }
                else
                {
                    pPsmSysRegistry->SysRamEnableFileSync((ANSC_HANDLE)pPsmSysRegistry, FALSE);
                }
                continue;
	    }
	    else if(index == PSM_RELOAD_CONFIG)
	    {
                if(getCompareValue(val[i].parameterValue))
		{
                    // pSysIraIf->ClearFolder(pSysIraIf->hOwnerContext, hSysRoot);
                    pPsmSysRegistry->SaveConfigToFlash(pPsmSysRegistry);
                    pPsmSysRegistry->bNoSave = FALSE;
                    pRegTimerObj->Start((ANSC_HANDLE)pRegTimerObj);
		            pPsmFileLoader->LoadRegFile((ANSC_HANDLE)pPsmFileLoader);
                }
               continue;
            }
            else if(index == PSM_UPDATE_CONFIG)
	    {
		if(getCompareValue(val[i].parameterValue))
                {
                    PSM_CFM_INTERFACE   *cfmif = (PSM_CFM_INTERFACE *)pPsmSysRegistry->hPsmCfmIf;

                    if (cfmif && cfmif->UpdateConfigs)
                    cfmif->UpdateConfigs(pPsmSysRegistry, g_NewConfigPath);
                }
                continue;
            }
	    else if(index == PSM_NEW_CONFIGPATH)
	    {
                snprintf(g_NewConfigPath, sizeof(g_NewConfigPath), "%s", val[i].parameterValue);
               continue;
            }
	}

	 /*RDKB-24884 : PSM set operation with same value should not update bbhm xml*/
	 rroRenderAttr.ContentType = val[i].type;

         rc =  memset_s(oldValBuf, sizeof(oldValBuf), 0, sizeof(oldValBuf));
	 ERR_CHK(rc);

	 returnStatus =
		  pSysIraIf->GetRecord
				(
					 pSysIraIf->hOwnerContext,
					 hSysRoot,
					 val[i].parameterName,
					 &ulRecordType,
					 (PANSC_HANDLE)&rroRenderAttr,
					 oldValBuf,
					 &ulRecordSize
				);

	if ( returnStatus == ANSC_STATUS_SUCCESS )
	{
	    rc = strcmp_s(oldValBuf, sizeof(oldValBuf), val[i].parameterValue, &ind);
	    ERR_CHK(rc);
	    if((rc == EOK) && (ind == 0))
	    {
		CcspTraceWarning(("setParameterValues: +++ Add entry:  is already present-- size:%d, %s: %s \n", size, val[i].parameterName, val[i].parameterValue));
		continue;
	    }
	}

        rroRenderAttr.ContentType = val[i].type;
	returnStatus	= ANSC_STATUS_SUCCESS;
        
        returnStatus =
            pSysIraIf->AddRecord2
                (
                    pSysIraIf->hOwnerContext,
                    hSysRoot,
                    val[i].parameterName,
                    SYS_RRO_PERMISSION_ALL,
                    SYS_REP_RECORD_TYPE_ASTR,
                    (ANSC_HANDLE)&rroRenderAttr,
                    (PVOID)val[i].parameterValue,
                    strlen(val[i].parameterValue) 
                );


        CcspTraceWarning(("setParameterValues -- size:%d, %s: %s\n", size, val[i].parameterName, val[i].parameterValue));


        if ( returnStatus != ANSC_STATUS_SUCCESS )
        {
            CcspTraceError(("+++ Add entry: size:%d , param : %s , val : %s failed! , return %lu +++\n", size, val[i].parameterName, val[i].parameterValue, returnStatus));
        }
    }

    pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
         //  CcspTraceInfo((" setParameterValues exit\n"));
    return CCSP_SUCCESS;
}

int setCommit(
    int sessionId,
    unsigned int writeID,
    dbus_bool commit,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(sessionId);
    UNREFERENCED_PARAMETER(writeID);
    UNREFERENCED_PARAMETER(commit);
    UNREFERENCED_PARAMETER(user_data);
       CcspTraceInfo((" setCommit!!\n"));
    return CCSP_ERR_NOT_SUPPORT;
}

int  setParameterAttributes(
    int sessionId,
    parameterAttributeStruct_t *val,
    int size,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(sessionId);
    UNREFERENCED_PARAMETER(user_data);
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    int                             i;
 //      CcspTraceInfo(("inside setParameterAttributes\n"));
    if ( g_psmHealth != CCSP_COMMON_COMPONENT_HEALTH_Green )
    {
        CcspTraceInfo(("!!! PSM is not ready !!!\n"));
        return CCSP_FAILURE;
    }
    if ( pPsmSysRegistry == NULL )
    {
        CcspTraceInfo(("setParameterAttributes- pPsmSysRegistry is NULL\n"));
        return CCSP_FAILURE;
    }

    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;

    if ( pSysInfoRepository == NULL )
    {
        CcspTraceInfo(("setParameterAttributes- pSysInfoRepository is NULL\n"));
        return CCSP_FAILURE;
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
        CcspTraceInfo(("setParameterAttributes- hSysRoot is NULL\n"));
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        return CCSP_FAILURE;
    }
 
    for ( i = 0; i < size; i++ )
    {
        returnStatus =
            pSysIraIf->DelRecord
                (
                    pSysIraIf->hOwnerContext,
                    hSysRoot,
                    val[i].parameterName
                );
/*
        CcspTraceWarning(("delParameterValues -- size:%d, %s\n", size, val[i].parameterName));
*/

        if ( returnStatus != ANSC_STATUS_SUCCESS )
        {
            CcspTraceError(("+++ Delete entry: %s failed! +++\n", val[i].parameterName));
        }
    }

    pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
 //   CcspTraceInfo(("setParameterAttributes exit\n"));
    return CCSP_SUCCESS;
}

int  getParameterAttributes(
    char * parameterNames[],
    int size,
    int *val_size,
    parameterAttributeStruct_t ***param_val,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(parameterNames);
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(val_size);
    UNREFERENCED_PARAMETER(param_val);
    UNREFERENCED_PARAMETER(user_data);
    CcspTraceInfo(("!!getParameterAttributes!!!!!\n"));
    return CCSP_ERR_NOT_SUPPORT;
}

int getParameterNames(
    char * parameterName,
    dbus_bool nextLevel,
    int *val_size ,
    parameterInfoStruct_t ***param_val,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(user_data);
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository  = (PSYS_INFO_REPOSITORY_OBJECT)NULL;
    PSYS_IRA_INTERFACE              pSysIraIf           = (PSYS_IRA_INTERFACE         )NULL;
    ANSC_HANDLE                     hSysRoot            = NULL;
    ULONG                           ulRecordCount;
    ULONG                           i, j;
    char                            recordName[SYS_MAX_RECORD_NAME_SIZE + 1];
    ULONG                           ulNameSize          = SYS_MAX_RECORD_NAME_SIZE;
    PPARAMETER_INFO                 pParameterInfo      = NULL;
    SLIST_HEADER                    ParameterInfoList;
    PSINGLE_LINK_ENTRY              pSLinkEntry         = NULL;
    parameterInfoStruct_t         **val                 = NULL;
    errno_t                         rc                  = -1;
    int                             ind                 = -1;
    int                             ret                 = CCSP_SUCCESS;
    //CcspTraceInfo(("getParameterNames begins\n"));
    if ( g_psmHealth != CCSP_COMMON_COMPONENT_HEALTH_Green )
    {
        CcspTraceInfo(("!!! PSM is not ready !!!\n"));
        return CCSP_FAILURE;
    }
    if ( !parameterName )
    {
        CcspTraceError(("RDKB_SYSTEM_BOOT_UP_LOG : PSM Input parameter invalid for getParameterNames!\n"));
        return CCSP_FAILURE;
    }

    if ( pPsmSysRegistry == NULL )
    {
        CcspTraceInfo(("getParameterNames- pPsmSysRegistry is NULL\n"));    
        return CCSP_FAILURE;
    }

    pSysInfoRepository = pPsmSysRegistry->hSysInfoRepository;

    if ( pSysInfoRepository == NULL )
    {    
    	CcspTraceInfo(("getParameterNames- pSysInfoRepository is NULL\n"));    
        return CCSP_FAILURE;
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
        pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        CcspTraceInfo(("getParameterNames- hSysRoot is NULL\n"));    
        return CCSP_FAILURE;
    }

    ulRecordCount = pSysIraIf->GetRecordCount
            (
                pSysIraIf->hOwnerContext,
                hSysRoot
            );

    *val_size = 0;
    AnscSListInitializeHeader(&ParameterInfoList);

    for ( i = 0; i < ulRecordCount; i++ )
    {
        ulNameSize = SYS_MAX_RECORD_NAME_SIZE;

         pSysIraIf->EnumRecord
                (
                    pSysIraIf->hOwnerContext,
                    hSysRoot,
                    i,
                    recordName,
                    &ulNameSize,
                    NULL,
                    NULL
                );
        if ( strstr(recordName, parameterName) == recordName )
        {
            if ( nextLevel )
            {
                if ( strlen(recordName) > strlen(parameterName) )
                {
                    char *p1, *p2;

                    p1 = recordName + strlen(parameterName);
                    if ( (p2 = strchr(p1, '.')) != NULL )
                    {
                        p2++;
                        p2[0] = '\0';
                    }
                }
                else
                {
                    continue;
                }
            }

            j = AnscSListQueryDepth(&ParameterInfoList);

            for ( ; j > 0; j-- )
            {
                pSLinkEntry = AnscSListSearchEntryByIndex(&ParameterInfoList, (j-1));
                if ( pSLinkEntry == NULL )
                {
                    CcspTraceInfo(("Get Null entry!\n"));
                    continue;
                }

                pParameterInfo = ACCESS_CONTAINER(pSLinkEntry, PARAMETER_INFO, Linkage);

		rc = strcmp_s(recordName, sizeof(recordName), pParameterInfo->val->parameterName, &ind);
		ERR_CHK(rc);
                if((rc == EOK) &&(ind == 0 ))
                {
                    break;
                }
            }

            if ( j == 0 )
            {
                *val_size = *val_size + 1;

                pParameterInfo = AnscAllocateMemory(sizeof(PARAMETER_INFO));
                if(pParameterInfo == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      ret = CCSP_FAILURE;
                      goto EXIT;
                }
                pParameterInfo->val = AnscAllocateMemory(sizeof(parameterInfoStruct_t));
                if(pParameterInfo->val == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free(pParameterInfo);
                      ret = CCSP_FAILURE;
                      goto EXIT;
                }

                pParameterInfo->val->parameterName = AnscAllocateMemory(ulNameSize+1);
                if(pParameterInfo->val->parameterName == NULL)
                {
                      CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
                      free(pParameterInfo->val);
                      free(pParameterInfo);
                      ret = CCSP_FAILURE;
                      goto EXIT;
                }
                char *pRecordName = recordName;
                rc = strcpy_s(pParameterInfo->val->parameterName,  ulNameSize+1, pRecordName);
		if(rc != EOK)
		{
		    ERR_CHK(rc);
                    free(pParameterInfo->val->parameterName);
                    free(pParameterInfo->val);
                    free(pParameterInfo);
                    ret = CCSP_FAILURE;
                    goto EXIT;
		}

                AnscSListPushEntry(&ParameterInfoList, &pParameterInfo->Linkage);
            }
        }
    }

 
    if ( *val_size > 0 )
    {
        i = *val_size;

        val = AnscAllocateMemory(*val_size*sizeof(parameterInfoStruct_t *));
        if(val == NULL)
        {
             CcspTraceInfo(("Memory Allocation failed - %s : %d\n", __FUNCTION__, __LINE__));
             ret = CCSP_FAILURE;
             goto EXIT;
        }
        rc = memset_s(val, *val_size*sizeof(parameterInfoStruct_t *), 0, *val_size*sizeof(parameterInfoStruct_t *));
	ERR_CHK(rc);

        for ( ; i > 0; i-- )
        {
            pSLinkEntry = AnscSListPopEntry(&ParameterInfoList);
            pParameterInfo = ACCESS_CONTAINER(pSLinkEntry, PARAMETER_INFO, Linkage);
            val[i-1] = pParameterInfo->val;
            AnscFreeMemory(pParameterInfo);

            //            CcspTraceDebug(("getParameterNames -- *val_size:%d, %s: %s\n", *val_size, parameterName, val[i-1]->parameterName));
        }
    }

    *param_val = val;

EXIT:

    if ( hSysRoot )
    {
        pSysIraIf->CloseFolder(pSysIraIf->hOwnerContext, hSysRoot);
    }

    pSysIraIf->RelThreadLock(pSysIraIf->hOwnerContext);
        //CcspTraceInfo(("getParameterNames ends\n"));
    return ret;
}

int  AddTblRow(
    int sessionId,
    char * objectName,
    int * instanceNumber,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(sessionId);
    UNREFERENCED_PARAMETER(objectName);
    UNREFERENCED_PARAMETER(instanceNumber);
    UNREFERENCED_PARAMETER(user_data);
    return CCSP_ERR_NOT_SUPPORT;
}

int  DeleteTblRow(
    int sessionId,
    char * objectName,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(user_data);
    UNREFERENCED_PARAMETER(objectName);
    UNREFERENCED_PARAMETER(sessionId);
    return CCSP_ERR_NOT_SUPPORT;
}

int freeResources(
    int priority,
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(user_data);
    UNREFERENCED_PARAMETER(priority);
    return CCSP_ERR_NOT_SUPPORT;
}


int busCheck(
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(user_data);
    return CCSP_SUCCESS;
}

int initialize(
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(user_data);
    return CCSP_SUCCESS;
}

int finalize(
    void            *user_data
)
{
    UNREFERENCED_PARAMETER(user_data);
    return CCSP_SUCCESS;
}

int getHealth()
{
    return g_psmHealth;
}

DBusHandlerResult
path_message_func (DBusConnection  *conn,
                   DBusMessage     *message,
                   void            *user_data)
{
    CCSP_MESSAGE_BUS_INFO *bus_info =(CCSP_MESSAGE_BUS_INFO *) user_data;
    const char *interface = dbus_message_get_interface(message);
    const char *method   = dbus_message_get_member(message);
    errno_t  rc = -1;
    int ind  = -1;
    DBusMessage *reply;
    reply = dbus_message_new_method_return (message);
    if (reply == NULL)
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

	rc = strcmp_s("org.freedesktop.DBus.Introspectable", strlen("org.freedesktop.DBus.Introspectable"), interface, &ind);
	ERR_CHK(rc);
	if((rc == EOK) && (!ind))
        {
	    rc = strcmp_s("Introspect", strlen("Introspect"), method, &ind);
	    ERR_CHK(rc);
	    if((rc == EOK) && (!ind))
	    {
	        if ( !dbus_message_append_args (reply, DBUS_TYPE_STRING, &PSM_Introspect_msg, DBUS_TYPE_INVALID))
                  printf ("No memory\n");
        
                if (!dbus_connection_send (conn, reply, NULL))
                   printf ("No memory\n");

                dbus_message_unref (reply);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
	}

    return CcspBaseIf_base_path_message_func (conn,
            message,
            reply,
            interface,
            method,
            bus_info);

}

int PsmDbusInit()
{
    int         ret ;
    char        CName[256];
    char        CrName[256];
    errno_t     rc = -1;
    CcspTraceWarning(("RDKB_SYSTEM_BOOT_UP_LOG : PsmDBusInit Entry\n"));
    if ( g_Subsystem[0] != 0 )
    {
        _ansc_sprintf(CName, "%s%s", g_Subsystem, CCSP_DBUS_PSM);
        _ansc_sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
    }
    else
    {
        rc = strcpy_s(CName, sizeof(CName), CCSP_DBUS_PSM);
	if(rc != EOK)
	{
	     ERR_CHK(rc);
	     return -1;
	}

        rc = strcpy_s(CrName, sizeof(CrName), CCSP_DBUS_INTERFACE_CR);
	if(rc != EOK)
	{
	    ERR_CHK(rc);
	    return -1;
	}
    }

    CCSP_Message_Bus_Init(CName, CCSP_MSG_BUS_CFG, &bus_handle,(CCSP_MESSAGE_BUS_MALLOC) Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
    g_psmHealth = CCSP_COMMON_COMPONENT_HEALTH_Yellow;
    /* Wait for CR ready */
    waitConditionReady(bus_handle, CrName, CCSP_DBUS_PATH_CR, CName);
    
    CCSP_Base_Func_CB cb;
    rc = memset_s(&cb, sizeof(cb), 0 , sizeof(cb));
    ERR_CHK(rc);
    cb.getParameterValues = getParameterValues;
    cb.setParameterValues = setParameterValues;
    cb.setCommit = setCommit;
    cb.setParameterAttributes = setParameterAttributes;
    cb.getParameterAttributes = getParameterAttributes;
    cb.AddTblRow = AddTblRow;
    cb.DeleteTblRow = DeleteTblRow;
    cb.getParameterNames = getParameterNames;
    cb.freeResources = freeResources;
    cb.busCheck = busCheck;
    cb.initialize = initialize;
    cb.finalize = finalize;
    cb.getHealth = getHealth;

    CcspBaseIf_SetCallback
    (
        bus_handle,
        &cb
    );
  
    /*Coverity Fix CID:61898  CHECKED_RETURN */
    if(CCSP_Message_Bus_Register_Path(bus_handle, CCSP_DBUS_PATH_PSM, path_message_func, bus_handle)!= CCSP_Message_Bus_OK) {
       CcspTraceError((" !!! CCSP_Message_Bus_Register_Path ERROR!!!\n"));
    }

    do
    {
        ULONG uWait = 10; /* 10 seconds */

        CcspTraceWarning(("PsmSsp register capabilities of %s to %s: \n", CName, CrName));
        CcspTraceWarning(("PsmSsp registering %zu items like %s with prefix='%s' ... \n", 
                          sizeof(NamespacePsm)/sizeof(name_spaceType_t), NamespacePsm[0].name_space, g_Subsystem));

        ret =
            CcspBaseIf_registerCapabilities
                (
                    bus_handle,
                    CrName, 
                    CName,
                    CCSP_COMPONENT_VERSION_PSM,
                    CCSP_DBUS_PATH_PSM,
                    g_Subsystem,
                    NamespacePsm,
                    sizeof(NamespacePsm)/sizeof(name_spaceType_t)
                );

        if ( CCSP_SUCCESS != ret )
        {
            CcspTraceWarning(("RDKB_SYSTEM_BOOT_UP_LOG : PsmSsp register capabilities failed with code %d! Waiting for retry...\n", ret));
            AnscSleep(uWait * 1000);
        }
        else
        {
            CcspTraceWarning((" RDKB_SYSTEM_BOOT_UP_LOG : PsmSsp register capabilities successful with ret=%d.\n", ret));
            break;
        }
    } while ( TRUE );

    pDslhCpeController = DslhCreateCpeController(NULL, NULL, NULL);
    if ( !pDslhCpeController )
    {
        CcspTraceWarning(("CANNOT Create pDslhCpeController... Exit!\n"));
    }

    pDslhCpeController->SetDbusHandle((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)bus_handle);
    pDslhCpeController->Engage((ANSC_HANDLE)pDslhCpeController);
    CcspTraceWarning(("RDKB_SYSTEM_BOOT_UP_LOG : PSM Health set to Green\n"));
    g_psmHealth = CCSP_COMMON_COMPONENT_HEALTH_Green;

    return 0;
}
