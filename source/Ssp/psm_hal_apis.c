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

    module: psm_hal_apis.c

        For CCSP Persistent Storage Manager component

    ---------------------------------------------------------------

    description:

        This file defines Persistent Storage Manager component 
        specific HAL apis.

    ---------------------------------------------------------------

    environment:

        platform dependent

    ---------------------------------------------------------------

    author:

        Ding Hua

    ---------------------------------------------------------------

    revision:

        04/16/2014    initial revision.

**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <secure_wrapper.h>

#if defined (_SKY_HUB_COMMON_PRODUCT_REQ_)
#include "ssp_global.h"
#endif

#include "ansc_platform.h"
#include "psm_hal_apis.h"

#if defined(CS_XB7)
#include "wifi_oem_api.h"
#define MIN_VAP_INDEX 0
#define MAX_VAP_INDEX 11
#endif

#if defined (_XB6_PRODUCT_REQ_) && !defined (CS_XB7) && !defined (_XB7_PRODUCT_REQ_) && !defined(_LG_OFW_)
#include "qtn_debug.h"
#include "wifi_oem_api.h"
extern int qtn_gen_ssid_default(char *ssid_buff);
#endif 

#if (defined (_XB7_PRODUCT_REQ_) || defined (_CBR_PRODUCT_REQ_)) && defined (_COSA_BCM_ARM_) && !defined(_SCER11BEL_PRODUCT_REQ_) || defined(_XER5_PRODUCT_REQ_)
#include "factory_info_api.h"
#endif
/**********************************************************************
                            HAL API IMPLEMENTATION
**********************************************************************/

/*
 *
    Description:
        This HAL API returns the platform specific persistent configuration
        parameters.

        Memory block of returned params is allocated inside of the routine
        using malloc(). Caller is responsible to call free() to free the
        memory after use.

    Arguments:
        PsmHalParam_t **            params,
        Returns the retrieved configuration parameters. Memory block
        is allocated in the routine and caller is responsible to free it.

        int *                       cnt
        Returns the number of parameters.

    Returns:
        0       - success
        -1      - failure or no custom parameters
 *
 */
#if (defined (_XB7_PRODUCT_REQ_) || defined (_CBR_PRODUCT_REQ_)) && defined (_COSA_BCM_ARM_)
#if defined(_SCER11BEL_PRODUCT_REQ_)
/* extern functions */
int platform_hal_GetWiFiSSID(char *pValue);
int platform_hal_GetWiFiPassword(char *pValue);
int platform_hal_GetWANMacAddress(char *pValue);
int platform_hal_getFactoryPartnerId(char *pValue);
#else
int factory_info_get_default_50_SSID(char *outputData);
#endif

typedef enum {
    PRIMARY_SSID_24_INDEX = 0,
    PRIMARY_SSID_50_INDEX,
    XHS_SSID_24_INDEX,
    XHS_SSID_50_INDEX,
    XFINITY_SSID_24_INDEX,
    XFINITY_SSID_50_INDEX,
#if defined (_CBR_PRODUCT_REQ_)
     ADDITIONAL_XFINITY_5 = 15,
#endif
#if defined (_XB8_PRODUCT_REQ_) || defined(_SCER11BEL_PRODUCT_REQ_)
    PRIMARY_SSID_60_INDEX = 16,
    XHS_SSID_60_INDEX = 17, 
    SSID_Idx_MAX
#endif
} SSID_Idx_enum;


#if defined(_SCER11BEL_PRODUCT_REQ_)
static int factory_default_xhs_ssid(char *xhs_ssid)
{
    char partner[32] = {0};
    char mac_str[32] = {0};
    int mac[6] = {0};

    platform_hal_GetWANMacAddress(&mac_str[0]);
    sscanf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    platform_hal_getFactoryPartnerId(&partner[0]);
    if (!strncmp("cox", partner, strlen(partner))) {
        snprintf(xhs_ssid, MAX_VALUE_SZ-1, "CHL-%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    } else if (!strncmp("shaw", partner, strlen(partner))) {
        snprintf(xhs_ssid, MAX_VALUE_SZ-1, "SHS-%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    } else if (!strncmp("rogers", partner, strlen(partner))) {
        snprintf(xhs_ssid, MAX_VALUE_SZ-1, "RSHM-%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    } else if (!strncmp("videotron", partner, strlen(partner))) {
        snprintf(xhs_ssid, MAX_VALUE_SZ-1, "VLHS-%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    } else {  /* comcast as default */
        snprintf(xhs_ssid, MAX_VALUE_SZ-1, "XHS-%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
    }
    return 0;
}
#endif

int wifi_get_oem_default_AP_ssid_string(int apIndex, char *output_ssid)
{
   switch(apIndex)
   {
#if defined(_SCER11BEL_PRODUCT_REQ_)
     case PRIMARY_SSID_24_INDEX:
     case PRIMARY_SSID_50_INDEX:
     case PRIMARY_SSID_60_INDEX:
       platform_hal_GetWiFiSSID(output_ssid);
       break;
#else
     case PRIMARY_SSID_24_INDEX:
       factory_info_get_default_24_SSID(output_ssid);
     break;
     case PRIMARY_SSID_50_INDEX:
       factory_info_get_default_50_SSID(output_ssid);
     break;
#if defined (_XB8_PRODUCT_REQ_)
     case PRIMARY_SSID_60_INDEX:
       factory_info_get_default_60_SSID(output_ssid);
     break;
#endif
#endif
#if defined (_CBR_PRODUCT_REQ_)
     case ADDITIONAL_XFINITY_5:
       sprintf(output_ssid,"xfinity public");
     break;
#endif
     case XHS_SSID_24_INDEX:
     case XHS_SSID_50_INDEX:
#if defined (_XB8_PRODUCT_REQ_) || defined(_SCER11BEL_PRODUCT_REQ_)
     case XHS_SSID_60_INDEX:
       // XHS SSID based on CM mac address
       // TriBand Radios 2.4GHZ, 5.0GHz, 6.0 GHz  have same SSID
#endif
#if defined(_SCER11BEL_PRODUCT_REQ_)
       factory_default_xhs_ssid(output_ssid);
#else
       factory_info_get_default_xhs_SSID(output_ssid);
#endif
     break;
     case XFINITY_SSID_24_INDEX:
     case XFINITY_SSID_50_INDEX:
       sprintf(output_ssid,"xfinitywifi");
     break;
     default:
       printf("SSID string not found in the OEM database\n");
     break;
   }
   return 0;
}

int wifi_get_oem_default_AP_passphrase_string(int apIndex, char *output_psk)
{
   switch(apIndex)
   {
     case PRIMARY_SSID_24_INDEX:
     case PRIMARY_SSID_50_INDEX:
#if defined (_XB8_PRODUCT_REQ_) || defined(_SCER11BEL_PRODUCT_REQ_)
     case PRIMARY_SSID_60_INDEX:
#endif
#if defined(_SCER11BEL_PRODUCT_REQ_)
        platform_hal_GetWiFiPassword(output_psk);
#else
        factory_info_get_wifi_passwd(output_psk);
#endif
     break;
     case XHS_SSID_24_INDEX:
     case XHS_SSID_50_INDEX:
#if defined (_XB8_PRODUCT_REQ_) || defined(_SCER11BEL_PRODUCT_REQ_)
     case XHS_SSID_60_INDEX:
       //Return the XHS passkey generated using XHS algorithm
       // TriBand Radios 2.4GHZ, 5.0GHz, 6.0 GHz  have same passkey
#endif
#if defined(_SCER11BEL_PRODUCT_REQ_)
        /* set XHS passphrase is same as private default passphrase */
        platform_hal_GetWiFiPassword(output_psk);
#else
        factory_info_get_xhs_passkey(output_psk);
#endif
     break;
     default:
       printf("SSID passphrase not present in the OEM database for apIndex=%d\n",apIndex);
     break;
   }
   return 0;
}
#endif

int PsmHal_GetCustomParams( PsmHalParam_t **params, int *cnt)
{
#if defined (CS_XB7)
    int c = (MAX_VAP_INDEX-MIN_VAP_INDEX+1)*2;
    char buff[MAX_VALUE_SZ] = {0};
    int ret;
    int apIndex;
    FILE *fp=NULL;
    char lnf[MAX_VALUE_SZ] = {0};
    const char *lnfpassphrase = "/lnf_file1";

    char *BssSsid       = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.%d.SSID";
    char *BssPassphrase = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.%d.Passphrase";

    /* ssid and passphrase defaults which have constant hardcoded values and
       are independent of device MACs */
    const char *default_ssids[] = {
        "XFSETUP-xxxx", "XFSETUP-xxxx",
        "XHS-xxxxxxxx", "XHS-xxxxxxxx",
        "OutOfService", "OutOfService",
        "A16746DF2466410CA2ED9FB2E32FE7D9", "A16746DF2466410CA2ED9FB2E32FE7D9",
        "OutOfService", "OutOfService",
        "D375C1D9F8B041E2A1995B784064977B", "D375C1D9F8B041E2A1995B784064977B"
    };
    const char *default_passphrase[] = {
        "xfsetup-01234","xfsetup-01234",
        "xhs-01234","xhs-01234",
        "wave1234","wave1234",
        "lnf-01234","lnf-01234",
        "wave1234","wave1234",
        "wave1234","wave1234"
    };

    *params = (PsmHalParam_t *) calloc(c, sizeof(PsmHalParam_t));
    if(*params == NULL)
    {
        printf("-- %s params memory allocation failed \n", __func__);
        return -1;
    }

    *cnt = c;

    fp=v_secure_popen("r", "/usr/bin/GetConfigFile %s stdout", lnfpassphrase);
    if(fp == NULL)
    {
            printf("Failed to read GetConfigFile File");
            return -1;
    }
    fgets(lnf, sizeof lnf, fp);
    v_secure_pclose(fp);

    for( apIndex = MIN_VAP_INDEX; apIndex <= MAX_VAP_INDEX; apIndex++ )
    {
        //SSIDs
        //fill PSM database field name and default value
        snprintf( (*params)[apIndex*2].name, MAX_NAME_SZ, BssSsid, apIndex+1);
        strncpy ( (*params)[apIndex*2].value, default_ssids[apIndex], MAX_VALUE_SZ);

        //Getting AP and XHS from DB
        if ( apIndex >= 0 && apIndex <= 3 )
        {
            ret = wifi_get_oem_default_AP_ssid_string(apIndex, buff);
            if( ret == 0 && (int)strlen((const char *)buff) >= 0 && strcmp( buff, "Unprogrammed or unknown NVM parameter") != 0 )
            {
                strncpy( (*params)[apIndex*2].value, buff, MAX_VALUE_SZ);
            }
        }
        printf("-- param[%d]: %s=[%s]\n", (apIndex*2), (*params)[apIndex*2].name, (*params)[apIndex*2].value);

        //Passphrases
        //fill PSM database field name and default value
        snprintf( (*params)[apIndex*2+1].name, MAX_NAME_SZ, BssPassphrase, apIndex+1);
        if( apIndex >= 6 && apIndex <= 7){
                strncpy ( (*params)[apIndex*2+1].value, lnf, MAX_VALUE_SZ);
        }else{
                strncpy ( (*params)[apIndex*2+1].value, default_passphrase[apIndex], MAX_VALUE_SZ);
        }

        //Getting AP and XHS from DB
        if ( apIndex >= 0 && apIndex <= 3 )
        {
            ret = wifi_get_oem_default_AP_passphrase_string(apIndex, buff);
            if( ret == 0 && strlen((const char *)buff) >= 8 &&
                    strcmp(buff, "Unprogrammed or unknown NVM parameter") != 0 )
            {
               strncpy( (*params)[apIndex*2+1].value, buff, MAX_VALUE_SZ);
            }
        }
          printf("-- param[%d]: %s=[%s]\n", (apIndex*2+1), (*params)[apIndex*2+1].name, (*params)[apIndex*2+1].value);
    }
    return 0;
#elif (defined (_XB7_PRODUCT_REQ_) || defined (_CBR_PRODUCT_REQ_)) && defined (_COSA_BCM_ARM_)
        int c = 12;
#if defined (_XB8_PRODUCT_REQ_)
        c = 16;
#endif
#if defined (_CBR_PRODUCT_REQ_)
        c = 13;
#endif
        char buff[128] = {0};
        const char *Bss1Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.1.SSID";
        const char *Bss1Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.1.Passphrase";
        const char *Bss2Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.2.SSID";
        const char *Bss2Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.2.Passphrase";
        const char *Bss3Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.3.SSID";
        const char *Bss3Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.3.Passphrase";
        const char *Bss4Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.4.SSID";
        const char *Bss4Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.4.Passphrase";
        const char *Bss5Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.5.SSID";
        const char *Bss5Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.5.Passphrase";
        const char *Bss6Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.6.SSID";
        const char *Bss6Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.6.Passphrase";
#if defined (_XB8_PRODUCT_REQ_)
        const char *Bss17Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.17.SSID"; //6GHz primary SSID
        const char *Bss17Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.17.Passphrase";
        const char *Bss18Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.18.SSID";
        const char *Bss18Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.18.Passphrase";
#endif
        char *ssid_private_default = "XFSETUP-xxxx";
        char *ssid_xhs_default = "XHS-xxxxxxxx";
        char *ssid_xfinity_default = "OutOfService";
        const char *passphrase_private_default = "xfsetup-01234";
        const char *passphrase_xhs_default = "xhs-01234";
        const char *passphrase_xfinity_default = "tcbx601234";
        char ssid_2g_default[65] = {0};
        char ssid_5g_default[65] = {0};
#if defined (_XB8_PRODUCT_REQ_)
        char ssid_6g_default[65] = {0};
#endif
        char ssid_2g_xhs_default[65] = {0};
        char ssid_5g_xhs_default[65] = {0};        
        char ssid_2g_xfinity_default[65] = {0};
        char ssid_5g_xfinity_default[65] = {0};

        char passphrase_2g_default[128] = {0};
        char passphrase_5g_default[128] = {0};
#if defined (_XB8_PRODUCT_REQ_)
        char ssid_6g_xhs_default[65] = {0};
        char passphrase_6g_default[128] = {0};
#endif
        char passphrase_2g_xhs_default[128] = {0};
        char passphrase_5g_xhs_default[128] = {0};
#if defined (_XB8_PRODUCT_REQ_)  
        char passphrase_6g_xhs_default[128] = {0};
#endif  
        char passphrase_2g_xfinity_default[128] = {0};
        char passphrase_5g_xfinity_default[128] = {0};
  
  	int ret;
        int apIndex;
#if defined (_CBR_PRODUCT_REQ_)
        const char *Bss16Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.16.SSID";
        char *additional_xfinity_default = "xfinity public";
        char additional_xfinity_5g_default[65] = {0};
        apIndex = 15;
        {
            ret = wifi_get_oem_default_AP_ssid_string(apIndex, buff);
            if( ret == 0 && (int)strlen((const char *)buff) >= 0 &&
            strcmp(buff, "Unprogrammed or unknown NVM parameter") != 0 )
            {
                strncpy(additional_xfinity_5g_default,  buff, sizeof(additional_xfinity_5g_default));
            }
            else
            {
                strncpy(additional_xfinity_5g_default, additional_xfinity_default, sizeof(additional_xfinity_5g_default));
            }
        }
#endif
#if defined (_XB8_PRODUCT_REQ_)
        for(apIndex = 0 ; apIndex < SSID_Idx_MAX ; apIndex++){
#else
        for(apIndex = 0 ; apIndex <= 5 ; apIndex++){
#endif
                ret = wifi_get_oem_default_AP_passphrase_string(apIndex, buff);
                if( ret == 0 && strlen((const char *)buff) >= 8 &&
                        strcmp(buff, "Unprogrammed or unknown NVM parameter") != 0 )
                {
                        printf("[psm_hal dbg] : func[%s] line[%d], get psk[%s] from OEM\n", __FUNCTION__, __LINE__, buff);
                        if (apIndex == 0)
                                strncpy(passphrase_2g_default, buff, sizeof(passphrase_2g_default));
                        else if (apIndex == 1)
                                strncpy(passphrase_5g_default, buff, sizeof(passphrase_5g_default));
                        else if (apIndex == 2)
                                strncpy(passphrase_2g_xhs_default, buff, sizeof(passphrase_2g_xhs_default));
                        else if (apIndex == 3)
                                strncpy(passphrase_5g_xhs_default, buff, sizeof(passphrase_5g_xhs_default));
                        else if (apIndex == 4)
                                strncpy(passphrase_2g_xfinity_default, buff, sizeof(passphrase_2g_xfinity_default));
                        else if (apIndex == 5)
                                strncpy(passphrase_5g_xfinity_default, buff, sizeof(passphrase_5g_xfinity_default));
#if defined (_XB8_PRODUCT_REQ_)              
                        else if (apIndex == 16)
                                strncpy(passphrase_6g_default, buff, sizeof(passphrase_6g_default));
                        else if (apIndex == 17)
                                strncpy(passphrase_6g_xhs_default, buff, sizeof(passphrase_6g_xhs_default));
#endif
                }
                else
                {
                     /* Failed to get psk from OEM */
                        printf("[psm_hal dbg] : func[%s] line[%d], faild to get psk[%s] from OEM, use RDKB default.\n", __FUNCTION__, __LINE__, buff);
                        if ( apIndex == 0 )
                                strncpy(passphrase_2g_default, passphrase_private_default, sizeof(passphrase_2g_default));
                        else if ( apIndex == 1 )
                                strncpy(passphrase_5g_default, passphrase_private_default, sizeof(passphrase_5g_default));
                        else if ( apIndex == 2 )
                                strncpy(passphrase_2g_xhs_default, passphrase_xhs_default, sizeof(passphrase_2g_xhs_default));
                        else if ( apIndex == 3 )
                                strncpy(passphrase_5g_xhs_default, passphrase_xhs_default, sizeof(passphrase_5g_xhs_default));
                        else if ( apIndex == 4 )
                                strncpy(passphrase_2g_xfinity_default, passphrase_xfinity_default, sizeof(passphrase_2g_xfinity_default));
                        else if ( apIndex == 5 )
                                strncpy(passphrase_5g_xfinity_default, passphrase_xfinity_default, sizeof(passphrase_5g_xfinity_default));
#if defined (_XB8_PRODUCT_REQ_)
                        else if ( apIndex == 16 )
                                strncpy(passphrase_6g_default, passphrase_private_default, sizeof(passphrase_6g_default));
                        else if ( apIndex == 17 )
                                strncpy(passphrase_6g_xhs_default, passphrase_xhs_default, sizeof(passphrase_6g_xhs_default));
#endif
                }
        }
#if defined (_XB8_PRODUCT_REQ_)
        for(apIndex = 0 ; apIndex < SSID_Idx_MAX ; apIndex++){
#else
        for(apIndex = 0 ; apIndex <= 5 ; apIndex++){
#endif
                ret = wifi_get_oem_default_AP_ssid_string(apIndex, buff);
                if( ret == 0 && (int)strlen((const char *)buff) >= 0 &&
                        strcmp(buff, "Unprogrammed or unknown NVM parameter") != 0 ) {
                        printf("[psm_hal dbg] : func[%s] line[%d], get ssid[%s] from OEM\n", __FUNCTION__, __LINE__, buff);
                        if(apIndex == 0)
                                strncpy(ssid_2g_default, buff, sizeof(ssid_2g_default));
                        else if(apIndex == 1)
                                strncpy(ssid_5g_default, buff, sizeof(ssid_5g_default));
                        else if(apIndex == 2)
                                strncpy(ssid_2g_xhs_default, buff, sizeof(ssid_2g_xhs_default));
                        else if(apIndex == 3)
                                strncpy(ssid_5g_xhs_default, buff, sizeof(ssid_5g_xhs_default));
                        else if(apIndex == 4)
                                strncpy(ssid_2g_xfinity_default, buff, sizeof(ssid_2g_xfinity_default));
                        else if(apIndex == 5)
                                strncpy(ssid_5g_xfinity_default, buff, sizeof(ssid_5g_xfinity_default));
#if defined (_XB8_PRODUCT_REQ_)
                        else if(apIndex == 16)
                                strncpy(ssid_6g_default, buff, sizeof(ssid_6g_default));
                        else if(apIndex == 17)
                                strncpy(ssid_6g_xhs_default, buff, sizeof(ssid_6g_xhs_default));
#endif

                }
                else
                {
                        printf("[psm_hal dbg] : func[%s] line[%d], fail to get ssid[%s] from OEM, use RDKB default.\n", __FUNCTION__, __LINE__, buff);
                        if(apIndex == 0)
                                strncpy(ssid_2g_default, ssid_private_default, sizeof(ssid_2g_default));
                        else if(apIndex == 1)
                                strncpy(ssid_5g_default, ssid_private_default, sizeof(ssid_5g_default));
                        else if(apIndex == 2)
                                strncpy(ssid_2g_xhs_default, ssid_xhs_default, sizeof(ssid_2g_xhs_default));
                        else if(apIndex == 3)
                                strncpy(ssid_5g_xhs_default, ssid_xhs_default, sizeof(ssid_5g_xhs_default));
                        else if(apIndex == 4)
                                strncpy(ssid_2g_xfinity_default, ssid_xfinity_default, sizeof(ssid_2g_xfinity_default));
                        else if(apIndex == 5)
                                strncpy(ssid_5g_xfinity_default, ssid_xfinity_default, sizeof(ssid_5g_xfinity_default));
#if defined (_XB8_PRODUCT_REQ_)
                        else if(apIndex == 16)
                                strncpy(ssid_6g_default, ssid_private_default, sizeof(ssid_6g_default));
                        else if(apIndex == 17)
                                strncpy(ssid_6g_xhs_default, ssid_xhs_default, sizeof(ssid_6g_xhs_default));
#endif

                }
        }
        *params = (PsmHalParam_t *) calloc(c, sizeof(PsmHalParam_t));
        *cnt = c;

        memcpy((*params)[0].name, Bss1Ssid, strlen(Bss1Ssid)+1);
        memcpy((*params)[0].value, ssid_2g_default, strlen((const char *)ssid_2g_default)+1);

        memcpy((*params)[1].name, Bss1Passphrase, strlen(Bss1Passphrase)+1);
        memcpy((*params)[1].value, passphrase_2g_default, strlen((const char *)passphrase_2g_default)+1);

        memcpy((*params)[2].name, Bss2Ssid, strlen(Bss2Ssid)+1);
        memcpy((*params)[2].value, ssid_5g_default, strlen((const char *)ssid_5g_default)+1);

        memcpy((*params)[3].name, Bss2Passphrase, strlen(Bss2Passphrase)+1);
        memcpy((*params)[3].value, passphrase_5g_default, strlen((const char *)passphrase_5g_default)+1);

        memcpy((*params)[4].name, Bss3Ssid, strlen(Bss3Ssid)+1);
        memcpy((*params)[4].value, ssid_2g_xhs_default, strlen((const char *)ssid_2g_xhs_default)+1);

        memcpy((*params)[5].name, Bss3Passphrase, strlen(Bss3Passphrase)+1);
        memcpy((*params)[5].value, passphrase_2g_xhs_default, strlen((const char *)passphrase_2g_xhs_default)+1);

        memcpy((*params)[6].name, Bss4Ssid, strlen(Bss4Ssid)+1);
        memcpy((*params)[6].value, ssid_5g_xhs_default, strlen((const char *)ssid_5g_xhs_default)+1);

        memcpy((*params)[7].name, Bss4Passphrase, strlen(Bss4Passphrase)+1);
        memcpy((*params)[7].value, passphrase_5g_xhs_default, strlen((const char *)passphrase_5g_xhs_default)+1);

        memcpy((*params)[8].name, Bss5Ssid, strlen(Bss5Ssid)+1);
        memcpy((*params)[8].value, ssid_2g_xfinity_default, strlen((const char *)ssid_2g_xfinity_default)+1);

        memcpy((*params)[9].name, Bss5Passphrase, strlen(Bss5Passphrase)+1);
        memcpy((*params)[9].value, passphrase_2g_xfinity_default, strlen((const char *)passphrase_2g_xfinity_default)+1);

        memcpy((*params)[10].name, Bss6Ssid, strlen(Bss6Ssid)+1);
        memcpy((*params)[10].value, ssid_5g_xfinity_default, strlen((const char *)ssid_5g_xfinity_default)+1);

        memcpy((*params)[11].name, Bss6Passphrase, strlen(Bss6Passphrase)+1);
        memcpy((*params)[11].value, passphrase_5g_xfinity_default, strlen((const char *)passphrase_5g_xfinity_default)+1);
#if defined (_CBR_PRODUCT_REQ_)        
        memcpy((*params)[12].name,Bss16Ssid, strlen(Bss16Ssid)+1);
        memcpy((*params)[12].value, additional_xfinity_5g_default,  strlen((const char *)additional_xfinity_5g_default) + 1);
#endif          
#if defined (_XB8_PRODUCT_REQ_)
        memcpy((*params)[12].name, Bss17Ssid, strlen(Bss17Ssid)+1);
        memcpy((*params)[12].value, ssid_6g_default, strlen((const char *)ssid_6g_default)+1);
  
        memcpy((*params)[13].name, Bss17Passphrase, strlen(Bss17Passphrase)+1);
        memcpy((*params)[13].value, passphrase_6g_default, strlen((const char *)passphrase_6g_default)+1);

       memcpy((*params)[14].name, Bss18Ssid, strlen(Bss18Ssid)+1);
       memcpy((*params)[14].value, ssid_6g_xhs_default, strlen((const char *)ssid_6g_xhs_default)+1);

       memcpy((*params)[15].name, Bss18Passphrase, strlen(Bss18Passphrase)+1);
       memcpy((*params)[15].value, passphrase_6g_xhs_default, strlen((const char *)passphrase_6g_xhs_default)+1);
#endif
        printf("[psm_hal dbg] : func[%s] line[%d] count[%d]\n", __FUNCTION__, __LINE__, *cnt);
        printf("[psm_hal dbg] : func[%s] line[%d] param0->name[%s] param0->value[%s]\n", __FUNCTION__, __LINE__, (*params)[0].name, (*params)[0].value);
        printf("[psm_hal dbg] : func[%s] line[%d] param1->name[%s] param1->value[%s]\n", __FUNCTION__, __LINE__, (*params)[1].name, (*params)[1].value);
        printf("[psm_hal dbg] : func[%s] line[%d] param2->name[%s] param2->value[%s]\n", __FUNCTION__, __LINE__, (*params)[2].name, (*params)[2].value);
        printf("[psm_hal dbg] : func[%s] line[%d] param3->name[%s] param3->value[%s]\n", __FUNCTION__, __LINE__, (*params)[3].name, (*params)[3].value);

        return  0;
#elif defined (_XB6_PRODUCT_REQ_) && !defined (CS_XB7) && !defined (_XB7_PRODUCT_REQ_) && !defined(_LG_OFW_)
    int c = 16;
    char buff[128] = {0};
    const char *Bss1Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.1.SSID";
    const char *Bss1Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.1.Passphrase";
    const char *Bss2Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.2.SSID";
    const char *Bss2Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.2.Passphrase";
    const char *Bss3Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.3.SSID";
    const char *Bss3Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.3.Passphrase";
    const char *Bss4Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.4.SSID";
    const char *Bss4Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.4.Passphrase";
    const char *Bss5Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.5.SSID";
    const char *Bss5Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.5.Passphrase";
    const char *Bss6Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.6.SSID";
    const char *Bss6Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.6.Passphrase";
    const char *Bss7Ssid = "eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.7.SSID";
    const char *Bss7Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.7.Passphrase";
    const char *Bss8Ssid ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.8.SSID";
    const char *Bss8Passphrase ="eRT.com.cisco.spvtg.ccsp.Device.WiFi.Radio.SSID.8.Passphrase";
    char *ssid_private_default = "XFSETUP-xxxx";
    char *ssid_xhs_default = "XHS-xxxxxxxx";
    char *ssid_xfinity_default = "OutOfService";
    char *ssid_LnF_default = "A16746DF2466410CA2ED9FB2E32FE7D9";
    const char *passphrase_private_default = "xfsetup-01234";
    const char *passphrase_xhs_default = "xhs-01234";
    const char *passphrase_xfinity_default = "qtn01234";
    const char *lnfpassphrase = "/lnf_file1";
    char ssid_2g_default[65] = {0};
    char ssid_5g_default[65] = {0};
    char ssid_2g_xhs_default[65] = {0};
    char ssid_5g_xhs_default[65] = {0};
    char ssid_2g_xfinity_default[65] = {0};
    char ssid_5g_xfinity_default[65] = {0};
    char ssid_2g_LnF_default[65] = {0};
    char ssid_5g_LnF_default[65] = {0};
    char passphrase_2g_default[128] = {0};
    char passphrase_5g_default[128] = {0};
    char passphrase_2g_xhs_default[128] = {0};
    char passphrase_5g_xhs_default[128] = {0};
    char passphrase_2g_xfinity_default[128] = {0};
    char passphrase_5g_xfinity_default[128] = {0};
    char passphrase_2g_LnF_default[128] = {0};
    char passphrase_5g_LnF_default[128] = {0};
    FILE *fp=NULL;
    char lnf[MAX_VALUE_SZ] = {0};
    int ret;
    int apIndex;

    fp=v_secure_popen("r", "/usr/bin/GetConfigFile %s stdout", lnfpassphrase);
    if(fp == NULL)
    {
            printf("Failed to read GetConfigFile File");
            return -1;
    }
    fgets(lnf, sizeof lnf, fp);
    v_secure_pclose(fp);

    for(apIndex = 0 ; apIndex <= 8 ; apIndex++){
        ret = wifi_get_oem_default_AP_passphrase_string(apIndex, buff);
        if( ret == 0 && strlen((const char *)buff) >= 8 &&
            strcmp(buff, "Unprogrammed or unknown NVM parameter") != 0 )
        {
            /*
             * Jackey: need to remove this workaround for specific odm
             */
            qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d], get psk[%s] from OEM\n", __FUNCTION__, __LINE__, buff);
            if (apIndex == 0)
                strncpy(passphrase_2g_default, buff, sizeof(passphrase_2g_default));
            else if (apIndex == 1)
                strncpy(passphrase_5g_default, buff, sizeof(passphrase_5g_default));
            else if (apIndex == 2)
                                strncpy(passphrase_2g_xhs_default, buff, sizeof(passphrase_2g_xhs_default));
            else if (apIndex == 3)
                                strncpy(passphrase_5g_xhs_default, buff, sizeof(passphrase_5g_xhs_default));
            else if (apIndex == 4)
                                strncpy(passphrase_2g_xfinity_default, buff, sizeof(passphrase_2g_xfinity_default));
            else if (apIndex == 5)
                                strncpy(passphrase_5g_xfinity_default, buff, sizeof(passphrase_5g_xfinity_default));
            else if (apIndex == 6)
                                strncpy(passphrase_2g_LnF_default, buff, sizeof(passphrase_2g_LnF_default));
            else if (apIndex == 7)
                                strncpy(passphrase_5g_LnF_default, buff, sizeof(passphrase_5g_LnF_default));
        }else { /* Failed to get psk from OEM */
            qprintf(DBG_LEVEL_ERROR | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d], faild to get psk[%s] from OEM, use QTN default.\n", __FUNCTION__, __LINE__, buff);
            if ( apIndex == 0 )
                strncpy(passphrase_2g_default, passphrase_private_default, sizeof(passphrase_2g_default));
            else if ( apIndex == 1 )
                strncpy(passphrase_5g_default, passphrase_private_default, sizeof(passphrase_5g_default));
            else if ( apIndex == 2 )
                                strncpy(passphrase_2g_xhs_default, passphrase_xhs_default, sizeof(passphrase_2g_xhs_default));
            else if ( apIndex == 3 )
                                strncpy(passphrase_5g_xhs_default, passphrase_xhs_default, sizeof(passphrase_5g_xhs_default));
            else if ( apIndex == 4 )
                                strncpy(passphrase_2g_xfinity_default, passphrase_xfinity_default, sizeof(passphrase_2g_xfinity_default));
                        else if ( apIndex == 5 )
                                strncpy(passphrase_5g_xfinity_default, passphrase_xfinity_default, sizeof(passphrase_5g_xfinity_default));
                        else if ( apIndex == 6 )
                                strncpy(passphrase_2g_LnF_default, lnf, sizeof(passphrase_2g_LnF_default));
                        else if ( apIndex == 7 )
                                strncpy(passphrase_5g_LnF_default, lnf, sizeof(passphrase_5g_LnF_default));
        }
    }
    for(apIndex = 0 ; apIndex <= 8 ; apIndex++){
        ret = wifi_get_oem_default_AP_ssid_string(apIndex, buff);
        if( ret == 0 && (int)strlen((const char *)buff) >= 0 &&
            strcmp(buff, "Unprogrammed or unknown NVM parameter") != 0 ) {
            qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d], get ssid[%s] from OEM\n", __FUNCTION__, __LINE__, buff);
            if(apIndex == 0)
                strncpy(ssid_2g_default, buff, sizeof(ssid_2g_default));
            else if(apIndex == 1)
                strncpy(ssid_5g_default, buff, sizeof(ssid_5g_default));
            else if(apIndex == 2)
                                strncpy(ssid_2g_xhs_default, buff, sizeof(ssid_2g_xhs_default));
            else if(apIndex == 3)
                                strncpy(ssid_5g_xhs_default, buff, sizeof(ssid_5g_xhs_default));
            else if(apIndex == 4)
                                strncpy(ssid_2g_xfinity_default, buff, sizeof(ssid_2g_xfinity_default));
            else if(apIndex == 5)
                                strncpy(ssid_5g_xfinity_default, buff, sizeof(ssid_5g_xfinity_default));
            else if(apIndex == 6)
                                strncpy(ssid_2g_LnF_default, buff, sizeof(ssid_2g_LnF_default));
            else if(apIndex == 7)
                                strncpy(ssid_5g_LnF_default, buff, sizeof(ssid_5g_LnF_default));
        }else{
            qprintf(DBG_LEVEL_ERROR | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d], fail to get ssid[%s] from OEM, use QTN default.\n", __FUNCTION__, __LINE__, buff);
            if(apIndex == 0)
                strncpy(ssid_2g_default, ssid_private_default, sizeof(ssid_2g_default));
            else if(apIndex == 1)
                                strncpy(ssid_5g_default, ssid_private_default, sizeof(ssid_5g_default));
                        else if(apIndex == 2)
                                strncpy(ssid_2g_xhs_default, ssid_xhs_default, sizeof(ssid_2g_xhs_default));
                        else if(apIndex == 3)
                                strncpy(ssid_5g_xhs_default, ssid_xhs_default, sizeof(ssid_5g_xhs_default));
                        else if(apIndex == 4)
                                strncpy(ssid_2g_xfinity_default, ssid_xfinity_default, sizeof(ssid_2g_xfinity_default));
                        else if(apIndex == 5)
                                strncpy(ssid_5g_xfinity_default, ssid_xfinity_default, sizeof(ssid_5g_xfinity_default));
                        else if(apIndex == 6)
                                strncpy(ssid_2g_LnF_default, ssid_LnF_default, sizeof(ssid_2g_LnF_default));
                        else if(apIndex == 7)
                                strncpy(ssid_5g_LnF_default, ssid_LnF_default, sizeof(ssid_5g_LnF_default));
        }
    }
    *params = (PsmHalParam_t *) calloc(c, sizeof(PsmHalParam_t));
    *cnt = c;
    memcpy((*params)[0].name, Bss1Ssid, strlen(Bss1Ssid)+1);
    memcpy((*params)[0].value, ssid_2g_default, strlen((const char *)ssid_2g_default)+1);
    memcpy((*params)[1].name, Bss1Passphrase, strlen(Bss1Passphrase)+1);
    memcpy((*params)[1].value, passphrase_2g_default, strlen((const char *)passphrase_2g_default)+1);
    memcpy((*params)[2].name, Bss2Ssid, strlen(Bss2Ssid)+1);
    memcpy((*params)[2].value, ssid_5g_default, strlen((const char *)ssid_5g_default)+1);
    memcpy((*params)[3].name, Bss2Passphrase, strlen(Bss2Passphrase)+1);
    memcpy((*params)[3].value, passphrase_5g_default, strlen((const char *)passphrase_5g_default)+1);
    memcpy((*params)[4].name, Bss3Ssid, strlen(Bss3Ssid)+1);
    memcpy((*params)[4].value, ssid_2g_xhs_default, strlen((const char *)ssid_2g_xhs_default)+1);
    memcpy((*params)[5].name, Bss3Passphrase, strlen(Bss3Passphrase)+1);
    memcpy((*params)[5].value, passphrase_2g_xhs_default, strlen((const char *)passphrase_2g_xhs_default)+1);
    memcpy((*params)[6].name, Bss4Ssid, strlen(Bss4Ssid)+1);
    memcpy((*params)[6].value, ssid_5g_xhs_default, strlen((const char *)ssid_5g_xhs_default)+1);
    memcpy((*params)[7].name, Bss4Passphrase, strlen(Bss4Passphrase)+1);
    memcpy((*params)[7].value, passphrase_5g_xhs_default, strlen((const char *)passphrase_5g_xhs_default)+1);
    memcpy((*params)[8].name, Bss5Ssid, strlen(Bss5Ssid)+1);
    memcpy((*params)[8].value, ssid_2g_xfinity_default, strlen((const char *)ssid_2g_xfinity_default)+1);
    memcpy((*params)[9].name, Bss5Passphrase, strlen(Bss5Passphrase)+1);
    memcpy((*params)[9].value, passphrase_2g_xfinity_default, strlen((const char *)passphrase_2g_xfinity_default)+1);
    memcpy((*params)[10].name, Bss6Ssid, strlen(Bss6Ssid)+1);
    memcpy((*params)[10].value, ssid_5g_xfinity_default, strlen((const char *)ssid_5g_xfinity_default)+1);
    memcpy((*params)[11].name, Bss6Passphrase, strlen(Bss6Passphrase)+1);
    memcpy((*params)[11].value, passphrase_5g_xfinity_default, strlen((const char *)passphrase_5g_xfinity_default)+1);
    memcpy((*params)[12].name, Bss7Ssid, strlen(Bss7Ssid)+1);
    memcpy((*params)[12].value, ssid_2g_LnF_default, strlen((const char *)ssid_2g_LnF_default)+1);
    memcpy((*params)[13].name, Bss7Passphrase, strlen(Bss7Passphrase)+1);
    memcpy((*params)[13].value, passphrase_2g_LnF_default, strlen((const char *)passphrase_2g_LnF_default)+1);
    memcpy((*params)[14].name, Bss8Ssid, strlen(Bss8Ssid)+1);
    memcpy((*params)[14].value, ssid_5g_LnF_default, strlen((const char *)ssid_5g_LnF_default)+1);
    memcpy((*params)[15].name, Bss8Passphrase, strlen(Bss8Passphrase)+1);
    memcpy((*params)[15].value, passphrase_5g_LnF_default, strlen((const char *)passphrase_5g_LnF_default)+1);
    qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d] count[%d]\n", __FUNCTION__, __LINE__, *cnt);
    qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d] param0->name[%s] param0->value[%s]\n", __FUNCTION__, __LINE__, (*params)[0].name, (*params)[0].value);
    qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d] param1->name[%s] param1->value[%s]\n", __FUNCTION__, __LINE__, (*params)[1].name, (*params)[1].value);
    qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d] param2->name[%s] param2->value[%s]\n", __FUNCTION__, __LINE__, (*params)[2].name, (*params)[2].value);
    qprintf(DBG_LEVEL_DEBUG | QTN_DBG_CCSP_PSM, "[psm_hal dbg] : func[%s] line[%d] param3->name[%s] param3->value[%s]\n", __FUNCTION__, __LINE__, (*params)[3].name, (*params)[3].value);
    return  0;
#else 
    UNREFERENCED_PARAMETER(params);
    UNREFERENCED_PARAMETER(cnt);
    return  -1;
#endif

}


/*
 *
    Description:
        After PSM reset to factory defaults, this HAL API gives the 
        underlying layer an opportunity to restore extra factory defaults 

    Arguments:

    Returns:
        0       - success
        -1      - failure
 *
 */
int
PsmHal_RestoreFactoryDefaults
    (
        void
    )
{
    return  0;
}
