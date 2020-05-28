/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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

    module:	ssp_cfmif.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <syscfg/syscfg.h>
#include "cjson/cJSON.h"
#include "psm_hal_apis.h"
#include "ssp_global.h"
#include <unistd.h>
#include "pthread.h"

#ifdef _COSA_SIM_
#define cfm_log_dbg(x)          printf x
#define cfm_log_err(x)          printf x
#else
#define cfm_log_dbg(x)          
#define cfm_log_ercfm_log_errprintf x
#endif

#include <sys/time.h>
static void print_time(const char *msg)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    fprintf(stderr, "[PSM-CFM] %-20s: %6d.%06d\n", msg ? msg : "", tv.tv_sec, tv.tv_usec);
}
int Psm_GetCustomPartnersParams( PsmHalParam_t **params, int *cnt1 );
int Psm_ApplyCustomPartnersParams( PsmHalParam_t **params, int *cnt2 );

#define PSM_REC_HASH_SIZE       1024

#define PSM_REC_TEMP            "  <Record name=\"%s\" type=\"%s\" contentType=\"%s\">%s</Record>\n"
#define PSM_REC_TEMP_NOCTYPE    "  <Record name=\"%s\" type=\"%s\">%s</Record>\n"
#define PARTNER_DEFAULT_APPLY_FILE  	"/nvram/.apply_partner_defaults"
#define PSM_CUR_CONFIG_FILE_NAME        "/nvram/bbhm_cur_cfg.xml"
#define PSM_BAK_CONFIG_FILE_NAME        "/nvram/bbhm_bak_cfg.xml"
#define PARTNER_DEFAULT_MIGRATE_PSM  	"/tmp/.apply_partner_defaults_psm"
#define PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER  	"/tmp/.apply_partner_defaults_new_psm_member"

struct psm_record {
    struct psm_record   *next;
    char                *name;
    char                *type;
    char                *ctype;
    char                *value;
};

static struct psm_record *rec_hash[PSM_REC_HASH_SIZE] = {0};
static pthread_mutex_t  rec_hash_lock = PTHREAD_MUTEX_INITIALIZER;

static inline char *remove_quote(char *buf)
{
    if (buf[0] == '"' && strlen(buf) >= 2 && buf[strlen(buf) - 1] == '"') {
        buf[strlen(buf) - 1] = '\0';
        return ++buf;
    }

    return buf;
}

static inline unsigned int hash(const char *str)
{
    unsigned int hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }

    return hash;
}

static inline unsigned int record_hash(const char *str)
{
    return hash(str) % PSM_REC_HASH_SIZE;
}

static void record_free(struct psm_record *rec)
{
    if (!rec)
        return;

    if (rec->name)
        free(rec->name);
    if (rec->type)
        free(rec->type);
    if (rec->ctype)
        free(rec->ctype);
    if (rec->value)
        free(rec->value);
    free(rec);
}

static struct psm_record *record_create(const char *name, 
        const char *type, const char *ctype, const char *value)
{
    struct psm_record *rec;
    
    if (!name || !strlen(name) || !type || !strlen(type))
        return NULL;

    if ((rec = malloc(sizeof(struct psm_record))) == NULL)
        return NULL;
    rec->name = strdup(name);
    rec->type = strdup(type);
    rec->ctype = ctype ? strdup(ctype) : NULL;
    rec->value = value ? strdup(value) : NULL;
    rec->next = NULL;

    if (!rec->name || !rec->type || (ctype && !rec->ctype) 
            || (value && !rec->value)) {
        record_free(rec);
        return NULL; /* no memory */
    }

    return rec;
}

/* parse record line to psm_record{} */
static struct psm_record *record_parse(char *buf)
{
    char *t_start, *t_end;
    char *v_start, *v_end;
    char *name, *type, *ctype;
    char *tok, *delim, *sp;

    if ((t_start = strstr(buf, "<Record")) == NULL)
        return NULL;
    t_start += strlen("<Record");

    /*
     * __NOTE__ 
     * 1. this is not strict XML parse.
     * 2. it's no need to decode XML(like &amp;...), 
     *    it will be done in uplayer, what we need is pass 
     *    original XML buffer to uplayer.
     *
     * There're 5 XML meta (' " & < >) and only '<' and '&' 
     * are strictly forbidden. Let's assume all 5 meta will 
     * not be used in PSM xml.
     */
    if ((t_end = strchr(t_start, '>')) == NULL)
        return NULL;

    *t_end = '\0';
    if (t_end[-1] == '/') { /* <.../> */
        t_end[-1] = '\0';
        v_start = v_end = NULL;
    } else {
        v_start = t_end + 1;
        if ((v_end = strstr(v_start, "</Record>")) == NULL)
            return NULL;
        *v_end = '\0';
    }

    name = type = ctype = NULL, delim = " \t\r\n";
    for (; (tok = strtok_r(t_start, delim, &sp)) != NULL; t_start = NULL) {
        if (strncmp(tok, "name=", strlen("name=")) == 0) {
            name = tok + strlen("name=");
            name = remove_quote(name);
        } else if (strncmp(tok, "type=", strlen("type=")) == 0) {
            type = tok + strlen("type=");
            type = remove_quote(type);
        } else if (strncmp(tok, "contentType=", strlen("contentType=")) == 0) {
            ctype = tok + strlen("contentType=");
            ctype = remove_quote(ctype);
        }
    }

    /* everything is Ok we can allocate a record now */
    return record_create(name, type, ctype, v_start);
}

/* return number of bytes saved or -1 on error */
static int record_save(const struct psm_record *rec, char *buf, size_t size)
{
    int n;

    if (!rec || !rec->name || !strlen(rec->name) 
            || !rec->type || !strlen(rec->type) || !buf)
        return -1;

    if (!rec->ctype)
        n = snprintf(buf, size, PSM_REC_TEMP_NOCTYPE, 
                rec->name, rec->type, (rec->value ? rec->value : ""));
    else
        n = snprintf(buf, size, PSM_REC_TEMP,
                rec->name, rec->type, rec->ctype, (rec->value ? rec->value : ""));

    return n;
}

/* load records from file to internal hash table */
static int load_records(const char *file)
{
    FILE *fp;
    char line[4096];
    struct psm_record *rec, *last;
    unsigned int idx;

    if ((fp = fopen(file, "rb")) == NULL)
    {
        CcspTraceError(("%s: failed to open %s file \n", __FUNCTION__, file));
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "<Record") == NULL)
            continue;

        if ((rec = record_parse(line)) == NULL) {
            CcspTraceError(("%s: fail to parse: %s\n", __FUNCTION__, line));
            continue;
        }

        idx = record_hash(rec->name);
        pthread_mutex_lock(&rec_hash_lock);
        if (rec_hash[idx] == NULL) {
            rec_hash[idx] = rec;
        } else {
            for (last = rec_hash[idx]; last && last->next; last = last->next)
                ;
            last->next = rec;
        }
        pthread_mutex_unlock(&rec_hash_lock);
    }

    fclose(fp);
    return 0;
}

/* free all records in hash table */
static void free_records(void)
{
    int i;
    struct psm_record *cur, *next;

   CcspTraceInfo(("-- %s : Freeing records from hash table\n", __FUNCTION__));
    pthread_mutex_lock(&rec_hash_lock);
    for (i = 0; i < PSM_REC_HASH_SIZE; i++) {
        if (!rec_hash[i])
            continue;

        for (cur = rec_hash[i], next = cur->next; cur != NULL; 
                cur = next, next = (cur ? cur->next : NULL))
            record_free(cur);

        rec_hash[i] = NULL;
    }
    pthread_mutex_unlock(&rec_hash_lock);
    return;
}

#define PSM_BUF_BASE_SIZE       (128 * 1024)
#define PSM_BUF_OFFSET_SIZE     (PSM_BUF_BASE_SIZE / 4)

/* 
 * flush all rec in Hash table to buffer.
 * @buf and @size are output.
 *
 * I combined flush and free operation to save time, 
 * (just traverse whole hash table once).
 * the bad side is that if no memory, the table may 
 * half-flushed and no change to recover the table.
 */
static int flush_records(char **buf, size_t *size)
{
    int i, off, left, n, err = -1;
    struct psm_record *cur, *next;
    void *ptr;

   CcspTraceInfo(("-- %s : Flushing all records in hash table to buffer\n", __FUNCTION__));
    /* 
     * save records from Hash to buffer and perform check 
     * we use dyn-array to save 'reallocate'.
     *
     * XXX: another option is to calculate "total buffer size" when 
     * load/insert/free records, so that we can allocate the buffer once.
     * But that idea make code not clear.
     */
    *size = PSM_BUF_BASE_SIZE;
    if ((*buf = AnscAllocateMemory(PSM_BUF_BASE_SIZE)) == NULL) {
        CcspTraceError(("%s: no memory\n", __FUNCTION__));
        return -1;
    }

    off = 0, left = PSM_BUF_BASE_SIZE;

    n = snprintf(*buf + off, left, "<?xml version=\"1.0\"  encoding=\"UTF-8\" ?>\n"
            "<Provision>\n");
    off += n;
    left -= n;

    pthread_mutex_lock(&rec_hash_lock);
    for (i = 0; i < PSM_REC_HASH_SIZE; i++) {
        if (!rec_hash[i])
            continue;

        //cfm_log_dbg(("Hash[%d] \n", i));

        for (cur = rec_hash[i], next = cur->next; cur != NULL; 
                cur = next, next = (cur ? cur->next : NULL)) {

            //cfm_log_dbg(("  name %s \n", cur->name));

            /* need to expend the buffer ? 
             * the space calculated below is little bigger than needed */
            if (left < strlen(PSM_REC_TEMP) + \
                    strlen(cur->name) + strlen(cur->type) \
                    + (cur->ctype ? strlen(cur->ctype) : 0) \
                    + (cur->value ? strlen(cur->value) : 0) \
                    + strlen("</Provision>\n") + 1) {

                *size += PSM_BUF_OFFSET_SIZE;
                if ((ptr = AnscReAllocateMemory(*buf, *size)) == NULL) {
                    CcspTraceError(("%s: no memory\n", __FUNCTION__));
                    goto out;
                }
                *buf = ptr;

                left += PSM_BUF_OFFSET_SIZE;
            }

            if ((n = record_save(cur, *buf + off, left)) < 0) {
                CcspTraceError(("%s: fail to save record\n", __FUNCTION__));
                goto out;
            }

            off += n;
            left -= n;
            record_free(cur);
        }

        rec_hash[i] = NULL;
    }

    err = 0;

out:
    pthread_mutex_unlock(&rec_hash_lock);
    if (err != 0) {
        AnscFreeMemory(*buf);
        return err;
    }

    n = snprintf(*buf + off, left, "</Provision>\n");
    off += n;
    left -= n;
    *size = off;
    return err;
}

static int insert_record(struct psm_record *new, int overwrite)
{
    int h_idx;
    struct psm_record *rec, *prev;

    h_idx = record_hash(new->name);

    pthread_mutex_lock(&rec_hash_lock);
    if (rec_hash[h_idx] == NULL) {
        rec_hash[h_idx] = new;
        cfm_log_dbg(("[INSERT-F] %s %s\n", new->name, new->value));
        pthread_mutex_unlock(&rec_hash_lock);
        return 0;
    }

    for (prev = NULL, rec = rec_hash[h_idx]; rec; prev = rec, rec = rec->next) {
        if (strcmp(rec->name, new->name) != 0)
            continue;

        if (!overwrite) {
            //cfm_log_dbg(("[IMPORTED-E] %s %s\n", new->name, new->value ? new->value : ""));
            record_free(new);
        } else {
            /* do not support change type/contentType */

            new->next = rec->next;
            /* May be I can use hlist in list.h someday.
             * things will be easier. */
            if (rec == rec_hash[h_idx]) {
                rec_hash[h_idx] = new;
            } else {
                if (prev) {
                    prev->next = new;
                } else {
                    CcspTraceError(("%s: Why got here ? it not possible.\n", __FUNCTION__));
                    pthread_mutex_unlock(&rec_hash_lock);
                    return -1;
                }
            }

            record_free(rec);
            cfm_log_dbg(("[IMPORTED-O] %s %s\n", new->name, new->value ? new->value : ""));
        }
        break;
    }
    if (!rec) {
        new->next = rec_hash[h_idx];
        rec_hash[h_idx] = new;
        CcspTraceInfo(("%s : [IMPORTED-NEW] %s %s\n",__FUNCTION__, new->name, new->value ? new->value : ""));
    }

    pthread_mutex_unlock(&rec_hash_lock);
    return 0;
}

static int import_custom_params(int overwrite)
{
    struct psm_record   *rec;
    PsmHalParam_t       *cus_params;
    int                 cus_cnt;
    int                 i, err = -1;

    if (PsmHal_GetCustomParams(&cus_params, &cus_cnt) != 0)
        return -1;

    for (i = 0; i < cus_cnt; i++) {
        if (!cus_params[i].name || !strlen(cus_params[i].name)) {
            CcspTraceError(("%s: invalid custom param\n", __FUNCTION__));
            continue;
        }

        rec = record_create(cus_params[i].name, "astr", NULL, cus_params[i].value);
        if (rec == NULL) {
            CcspTraceError(("%s: record_create fail\n", __FUNCTION__));
            goto out;
        }

        if (insert_record(rec, overwrite) != 0) {
            CcspTraceError(("%s: insert_record() fail\n", __FUNCTION__));
            record_free(rec);
            goto out;
        }
    }

    err = 0;

out:
    if (cus_params)
        free(cus_params);
    return err;
}

int Psm_GetCustomPartnersParams( PsmHalParam_t **params, int *cnt )
{
	int isNeedtoProceedfurther = 0;
	int isNeedToApplyPartnersDefault_PSM = 0;
	
	if ( ( access( PSM_CUR_CONFIG_FILE_NAME , F_OK ) != 0 ) && \
		 ( access( PSM_BAK_CONFIG_FILE_NAME , F_OK ) != 0 )
		)
	{
		isNeedtoProceedfurther = 1;
	}

	if ( access( PARTNER_DEFAULT_MIGRATE_PSM , F_OK ) == 0 )  
	{
		isNeedToApplyPartnersDefault_PSM = 1;
		isNeedtoProceedfurther = 1;
		CcspTraceInfo(("-- %s - Deleting this file :%s\n", __FUNCTION__, PARTNER_DEFAULT_MIGRATE_PSM ));
		system( "rm -rf /tmp/.apply_partner_defaults_psm" );
	}

	if ( access( PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER , F_OK ) == 0 )  
	{
		isNeedToApplyPartnersDefault_PSM = 1;
		isNeedtoProceedfurther = 1;
		CcspTraceInfo(("-- %s - Deleting this file :%s\n", __FUNCTION__, PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER ));
		system( "rm -rf /tmp/.apply_partner_defaults_new_psm_member" );
	}

	CcspTraceInfo(("-- %s - isNeedtoProceedfurther:%d \n", __FUNCTION__, isNeedtoProceedfurther ));

	if( isNeedtoProceedfurther )
	{
		int isNeedToApplyPartnersDefault = 1;
		
		if ( access( PARTNER_DEFAULT_APPLY_FILE , F_OK ) != 0 )  
		{
			isNeedToApplyPartnersDefault = 0;
		}
		else
		{
			CcspTraceInfo(("-- %s - Deleting this file :%s\n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE ));
			system( "rm -rf /nvram/.apply_partner_defaults" );
		}
		
		if( ( isNeedToApplyPartnersDefault == 1 ) || \
			( isNeedToApplyPartnersDefault_PSM == 1 )
		  )
		{
			PsmHalParam_t  localparamArray[ 128 ]	= { 0 }; //Just given max size as 128 if more than 128 then dev should change it			
			char		   value_buf[ 64 ];
			int 		   ret 						= -1,
						   localCount		 		= 0;
		
			// Init syscfg
			syscfg_init( );
						
			/* eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.X_RDKCENTRAL-COM_Syndication.WiFiRegion.Code */

			//Get the syscfg.db value of PSM Param
			memset( value_buf, 0 , sizeof( value_buf ) );
			ret = syscfg_get( NULL, "WiFiRegionCode", value_buf, sizeof( value_buf ) );

			if( ( ret == 0 ) && \
				( '\0' != value_buf[ 0 ] ) 
			   )
			{
				//Copy the PSM Paramater name
				sprintf( localparamArray[ localCount ].name , "%s", "eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.X_RDKCENTRAL-COM_Syndication.WiFiRegion.Code" );
				sprintf( localparamArray[ localCount ].value, "%s", value_buf );
				CcspTraceInfo(("-- %s - Name :%s Value:%s\n", __FUNCTION__, localparamArray[ localCount ].name, localparamArray[ localCount ].value ));
				
				// Remove DB variable. It won't use
				syscfg_unset( NULL, "WiFiRegionCode" );

				//Increment the count by 1
				localCount++;
			}
	
            /* dmsb.device.deviceinfo.X_RDKCENTRAL-COM_Syndication.TR69CertLocation */

            //Get the syscfg.db value of PSM Param
            memset( value_buf, 0 , sizeof( value_buf ) );
            ret = syscfg_get( NULL, "TR69CertLocation", value_buf, sizeof( value_buf ) );

            if( ( ret == 0 ) && \
                ( '\0' != value_buf[ 0 ] )
               )
            {
				//Copy the PSM Paramater name
				sprintf( localparamArray[ localCount ].name , "%s", "dmsb.device.deviceinfo.X_RDKCENTRAL-COM_Syndication.TR69CertLocation" );
				sprintf( localparamArray[ localCount ].value, "%s", value_buf );
				CcspTraceInfo(("-- %s - Name :%s Value:%s\n", __FUNCTION__, localparamArray[ localCount ].name, localparamArray[ localCount ].value ));
				
				// Remove DB variable. It won't be used
				syscfg_unset( NULL, "TR69CertLocation" );

				//Increment the count by 1
				localCount++;
            }

            /* Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix */
			
            //Get the syscfg.db value of PSM Param
            memset( value_buf, 0 , sizeof( value_buf ) );
            ret = syscfg_get( NULL, "XHS_SSIDprefix", value_buf, sizeof( value_buf ) );

            if( ( ret == 0 ) && \
                ( '\0' != value_buf[ 0 ] )
               )
            {
				//Copy the PSM Paramater name
				sprintf( localparamArray[ localCount ].name , "%s", "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix" );
				sprintf( localparamArray[ localCount ].value, "%s", value_buf );
				CcspTraceInfo(("-- %s - Name :%s Value:%s\n", __FUNCTION__, localparamArray[ localCount ].name, localparamArray[ localCount ].value ));
				
				// Remove DB variable. It won't be used
				syscfg_unset( NULL, "XHS_SSIDprefix" );

				//Increment the count by 1
				localCount++;
            }
            //Get the syscfg.db value of DefaultAdminIP PSM Param
            memset( value_buf, 0 , sizeof( value_buf ) );
            ret = syscfg_get( NULL, "lan_ipaddr", value_buf, sizeof( value_buf ) );

            if( ( ret == 0 ) && \
                ( '\0' != value_buf[ 0 ] )
               )
            {
				//Copy the PSM Paramater name
				sprintf( localparamArray[ localCount ].name , "%s", "dmsb.l3net.4.V4Addr" );
				sprintf( localparamArray[ localCount ].value, "%s", value_buf );
				CcspTraceInfo(("-- %s - Name :%s Value:%s\n", __FUNCTION__, localparamArray[ localCount ].name, localparamArray[ localCount ].value ));
				
        			//Increment the count by 1
				localCount++;
            }
            //Get the syscfg.db value of DefaultLocalIPv4SubnetRange PSM Param
            memset( value_buf, 0 , sizeof( value_buf ) );
            ret = syscfg_get( NULL, "lan_netmask", value_buf, sizeof( value_buf ) );

            if( ( ret == 0 ) && \
                ( '\0' != value_buf[ 0 ] )
               )
            {
				//Copy the PSM Paramater name
				sprintf( localparamArray[ localCount ].name , "%s", "dmsb.l3net.4.V4SubnetMask" );
				sprintf( localparamArray[ localCount ].value, "%s", value_buf );
				CcspTraceInfo(("-- %s - Name :%s Value:%s\n", __FUNCTION__, localparamArray[ localCount ].name, localparamArray[ localCount ].value ));
				
        			//Increment the count by 1
				localCount++;
            }
			syscfg_commit();

			//Initialize count as 0 here
			*cnt	= 0;

			CcspTraceInfo(("-- %s - Total Count:%d\n", __FUNCTION__, localCount ));

			//Check whether any Data to be copy
			if( 0 < localCount )
			{
				*cnt	= localCount;
				*params =( PsmHalParam_t * ) malloc( sizeof( PsmHalParam_t ) * ( *cnt ) );

				if( NULL == *params )
				{
					*cnt	= 0;

					CcspTraceInfo(("-- %s - Failed to allocate memory\n", __FUNCTION__ ));
					return	-1;					
				}

				//Copy all current data to passed param
				memcpy( *params, &localparamArray, ( sizeof( PsmHalParam_t ) * ( *cnt ) ) );

				return	0;
			}
		}
	}

	return	-1;
}

static int applyPsm_custom_partners_params(int overwrite)
{
    struct psm_record   *rec;
    PsmHalParam_t       *cus_params;
    int                 cus_cnt2=0;
    int                 i, err = -1;

    if (Psm_ApplyCustomPartnersParams(&cus_params, &cus_cnt2) !=0)
        return -1;

    for (i = 0; i < cus_cnt2; i++)
        {
        if (!cus_params[i].name || !strlen(cus_params[i].name)) {
            CcspTraceError(("%s: invalid custom partners param\n", __FUNCTION__));
            continue;
        }

        rec = record_create(cus_params[i].name, "astr", NULL, cus_params[i].value);
        if (rec == NULL) {
            CcspTraceError(("%s: record_create fail\n", __FUNCTION__));
            goto out;
        }

        if (insert_record(rec, overwrite) != 0) {
            CcspTraceError(("%s: insert_record() fail\n", __FUNCTION__));
            record_free(rec);
            goto out;
        }
    }

    err = 0;

out:
    if (cus_params)
        free(cus_params);
    return err;
}

static int import_custom_partners_params(int overwrite)
{
    struct psm_record   *rec;
    PsmHalParam_t       *cus_params;
    int                 cus_cnt1=0;
    int                 i, err = -1;

    if (Psm_GetCustomPartnersParams(&cus_params, &cus_cnt1) != 0)
        return -1;

    for (i = 0; i < cus_cnt1; i++) 
	{
        if (!cus_params[i].name || !strlen(cus_params[i].name)) {
            CcspTraceError(("%s: invalid custom partners param\n", __FUNCTION__));
            continue;
        }

        rec = record_create(cus_params[i].name, "astr", NULL, cus_params[i].value);
        if (rec == NULL) {
            CcspTraceError(("%s: record_create fail\n", __FUNCTION__));
            goto out;
        }

        if (insert_record(rec, overwrite) != 0) {
            CcspTraceError(("%s: insert_record() fail\n", __FUNCTION__));
            record_free(rec);
            goto out;
        }
    }

    err = 0;

out:
    if (cus_params)
        free(cus_params);
    return err;
}

static int merge_missing_param(const char *from, int overwrite)
{
    FILE *fp;
    char line[4096];
    struct psm_record *rec;

    if ((fp = fopen(from, "rb")) == NULL) {
        CcspTraceError(("%s: Fail to open config: %s\n", __FUNCTION__, from));
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if ((rec = record_parse(line)) == NULL)
            continue;

        if (insert_record(rec, overwrite) != 0) {
            CcspTraceError(("%s: insert_record() fail\n", __FUNCTION__));
            record_free(rec);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
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

    char                            path[256];
    int                             usg_bak = 0;

    /*
     * 1. try read cur config and a) import custom b) merge def config
     * 2. try read bak config and a) import custom b) merge def config
     * 3. read def config(leverage merging codes) and import custom.
     *
     * it's always not overwrite.
     */
    CcspTraceInfo(("ssp_CfmReadCurConfig begins\n"));    
    snprintf(path, sizeof(path), "%s%s", pProp->SysFilePath, pProp->CurFileName);
again:
    /* load config file to Hash for fast merging and import */
    if (load_records(path) != 0) {
        CcspTraceError(("%s: Fail to load config file: %s\n", __FUNCTION__, path));
        if (!usg_bak) {
            snprintf(path, sizeof(path), "%s%s", pProp->SysFilePath, pProp->BakFileName);
            usg_bak = 1;
            goto again;
        }
    }

    /* import customer params without overwrite */
    if (import_custom_params(0) != 0)
        CcspTraceError(("%s: Fail to import custom params\n", __FUNCTION__));

    /* merge missing param from default config */
    snprintf(path, sizeof(path), "%s%s", pProp->SysFilePath, pProp->DefFileName);
    if (merge_missing_param(path, 0) != 0) {
        CcspTraceError(("%s: Fail to merge def config\n", __FUNCTION__));
    }

    /* import customer params with overwrite */
    if (import_custom_partners_params(1) != 0)
        CcspTraceInfo(("%s: Fail to import custom partners params\n", __FUNCTION__));

    /* flush merged records to buffer */
    if (flush_records((char **)ppCfgBuffer, pulCfgSize) != 0) {
        free_records();
        CcspTraceInfo((" ssp_CfmReadCurConfig-flush_records((char **)ppCfgBuffer, pulCfgSize) != 0\n"));    
        return ANSC_STATUS_FAILURE;
    }
    //CcspTraceInfo(("ssp_CfmReadCurConfig ends\n"));    
    return ssp_CfmSaveCurConfig(hThisObject, *ppCfgBuffer, *pulCfgSize);
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

    char                            path[256];
    CcspTraceInfo(("ssp_CfmReadDefConfig begins\n"));    
    snprintf(path, sizeof(path), "%s%s", pProp->SysFilePath, pProp->DefFileName);
    if (load_records(path) != 0) {
        CcspTraceError(("%s: Fail to load config file: %s\n", __FUNCTION__, path));
        return ANSC_STATUS_FAILURE;
    }

    /* import customer params and overwrite if exist */
    if (import_custom_params(1) != 0) {
        CcspTraceError(("%s: Fail to import custom params\n", __FUNCTION__));
        free_records();
        return ANSC_STATUS_FAILURE;
    }

    /* flush merged records to buffer */
    if (flush_records((char **)ppCfgBuffer, pulCfgSize) != 0) {
        free_records();
        CcspTraceInfo((" ssp_CfmReadDefConfig-flush_records((char **)ppCfgBuffer, pulCfgSize) != 0\n"));            
        return ANSC_STATUS_FAILURE;
    }
//    CcspTraceInfo(("ssp_CfmReadDefConfig ends\n"));    
    return ANSC_STATUS_SUCCESS;
}

/*-----------------------------------------------------------------------------*/

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
        AnscFreeMemory(tmpBuf); /*10-May-2016 RDKB-5567 CID-33369, free the buffer if rootNode is null*/
        return NULL;
    }

    /*AnscFreeMemory(tmpBuf); */
    return rootNode;
}

static ANSC_STATUS
FileReadToBuffer(const char *file, char **buf, ULONG *size)
{
    ANSC_HANDLE pFile;
//    CcspTraceInfo(("FileReadToBuffer begins\n"));    
    if ((pFile = AnscOpenFile((char *)file, ANSC_FILE_MODE_READ, ANSC_FILE_TYPE_RDWR)) == NULL)
        return ANSC_STATUS_FAILURE;

    if ((*size = AnscGetFileSize(pFile)) < 0)
    {
        AnscCloseFile(pFile);
    	CcspTraceInfo(("FileReadToBuffer-> *size = AnscGetFileSize(pFile)) < 0\n"));            
        return ANSC_STATUS_FAILURE;
    }

    if ((*buf = AnscAllocateMemory(*size + 1)) == NULL)
    {
        AnscCloseFile(pFile);
    	CcspTraceInfo(("FileReadToBuffer-> *buf = AnscAllocateMemory(*size + 1)) == NULL\n"));                 
        return ANSC_STATUS_FAILURE;
    }

    /* this API need ULONG * for third param */
    if (AnscReadFile(pFile, *buf, size) != ANSC_STATUS_SUCCESS)
    {
        AnscFreeMemory(*buf);
        AnscCloseFile(pFile);
    	CcspTraceInfo(("FileReadToBuffer-> AnscReadFile(pFile, *buf, size) != ANSC_STATUS_SUCCESS\n"));                 
        return ANSC_STATUS_FAILURE;
    }

    (*buf)[*size] = '\0';

    AnscCloseFile(pFile);
//    CcspTraceInfo(("FileReadToBuffer ends\n"));    
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
        CcspTraceInfo(("NodeGetRecord -> node->GetDataString(node, NULL, rec->value, &size) != ANSC_STATUS_SUCCESS\n"));    
        return ANSC_STATUS_FAILURE;
    rec->value[size] = '\0';

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
AddRecToXml(const PsmRecord_t *rec, PANSC_XML_DOM_NODE_OBJECT xml)
{
    PANSC_XML_DOM_NODE_OBJECT node;

    if ((node = xml->AddChildByName(xml, ELEM_RECORD)) == NULL)
    	CcspTraceInfo(("AddRecToXml-> node = xml->AddChildByName(xml, ELEM_RECORD) \n"));    
        return ANSC_STATUS_FAILURE;

    if (RecordSetNode(rec, node) != ANSC_STATUS_SUCCESS)
    {
        xml->DelChild(xml, node);
        CcspTraceInfo(("AddRecToXml->RecordSetNode(rec, node) != ANSC_STATUS_SUCCESS \n"));    
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
XmlToBuffer(PANSC_XML_DOM_NODE_OBJECT xml, char **buf, ULONG *size)
{
    char *newBuf = NULL;
    ULONG newSize;
//    CcspTraceInfo(("XmlToBuffer begins\n"));    
    if ((*size = newSize = xml->GetEncodedSize(xml)) == 0
            /* any one tell me the reason magic 16 ? */
            || (newBuf = AnscAllocateMemory(newSize + 16)) == NULL 
            || xml->Encode(xml, (PVOID)newBuf, &newSize) != ANSC_STATUS_SUCCESS)
    {
        if (newBuf)
            AnscFreeMemory(newBuf);
            CcspTraceInfo(("XmlToBuffer ends->newBuf"));    
        return ANSC_STATUS_FAILURE;
    }

    *buf = newBuf;
//    CcspTraceInfo(("XmlToBuffer ends\n"));    
    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS
XmlToFile(PANSC_XML_DOM_NODE_OBJECT xml, const char *file)
{
    char *buf;
    ULONG size;
    ANSC_HANDLE pFile;
//    CcspTraceInfo(("XmlToFile begins\n"));    
    if (XmlToBuffer(xml, &buf, &size) != ANSC_STATUS_SUCCESS)
     	CcspTraceInfo(("XmlToFile -> XmlToBuffer(xml, &buf, &size) != ANSC_STATUS_SUCCESS\n"));       
        return ANSC_STATUS_FAILURE;

    if ((pFile = AnscOpenFile((char *)file, 
                    ANSC_FILE_TYPE_RDWR, ANSC_FILE_TYPE_RDWR)) == NULL)
    {
        AnscFreeMemory(buf);
        CcspTraceInfo(("XmlToFile -> (pFile=open(ANSC_FILE_TYPE_RDWR, ANSC_FILE_TYPE_RDWR))==NULL\n"));       
        return ANSC_STATUS_FAILURE;
    }

    if (AnscWriteFile(pFile, buf, &size) != ANSC_STATUS_SUCCESS)
    {
        AnscCloseFile(pFile);
        AnscFreeMemory(buf);
        CcspTraceInfo(("XmlToFile -> AnscWriteFile(pFile, buf, &size) != ANSC_STATUS_SUCCESS\n"));              
        return ANSC_STATUS_FAILURE;
    }

    AnscCloseFile(pFile);
    AnscFreeMemory(buf);
//    CcspTraceInfo(("XmlToFile ends\n"));    
    return ANSC_STATUS_SUCCESS;
}

static BOOL
IsRecChangedFromXml(const PsmRecord_t *rec, PANSC_XML_DOM_NODE_OBJECT xml)
{
    PANSC_XML_DOM_NODE_OBJECT node;
    PsmRecord_t cur;
//    CcspTraceInfo(("IsRecChangedFromXml begins\n"));    
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
//        CcspTraceInfo(("IsRecChangedFromXml ends\n"));    
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
//    CcspTraceInfo(("ReadCfgXmlWithCustom begins\n"));    
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
//    CcspTraceInfo(("ReadCfgXmlWithCustom ends\n"));    
    return root;
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

    CcspTraceInfo(("%s: called\n", __FUNCTION__));

    /* save current to backup config */
    snprintf(curPath, sizeof(curPath), "%s%s", pProp->SysFilePath, pProp->CurFileName);
    snprintf(bakPath, sizeof(bakPath), "%s%s", pProp->SysFilePath, pProp->BakFileName);
    //CcspTraceInfo(("ssp_CfmSaveCurConfig begins\n"));    
    if (AnscCopyFile(curPath, bakPath, TRUE) != ANSC_STATUS_SUCCESS)
        PsmHalDbg(("%s: fail to backup current config\n", __FUNCTION__));

    if ((pFile = AnscOpenFile(curPath, ANSC_FILE_MODE_CREATE | ANSC_FILE_MODE_WRITE | ANSC_FILE_MODE_TRUNC,
            ANSC_FILE_TYPE_RDWR)) == NULL)
    {
        PsmHalDbg(("%s: fail open current config\n", __FUNCTION__));
        CcspTraceInfo(("ssp_CfmSaveCurConfig -> fail open current config\n"));           
        return ANSC_STATUS_FAILURE;
    }

    if (AnscWriteFile(pFile, pCfgBuffer, &ulCfgSize) != ANSC_STATUS_SUCCESS)
    {
        AnscCloseFile(pFile);
     	CcspTraceInfo(("ssp_CfmSaveCurConfig -> AnscWriteFile(pFile, pCfgBuffer, &ulCfgSize) != ANSC_STATUS_SUCCESS\n"));           
        return ANSC_STATUS_FAILURE;
    }

    AnscCloseFile(pFile);
    //CcspTraceInfo(("ssp_CfmSaveCurConfig ends\n"));    
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
    CcspTraceInfo(("ssp_CfmUpdateConfigs begins\n"));    
    if (!newConfPath || AnscSizeOfString(newConfPath) <= 0)
    {
        PsmHalDbg(("%s: bad param\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    snprintf(curPath, sizeof(curPath), "%s%s", pProp->SysFilePath, pProp->CurFileName);
    snprintf(defPath, sizeof(defPath), "%s%s", pProp->SysFilePath, pProp->DefFileName);

    CcspTraceInfo(("=======  %s  =======\n", __FUNCTION__));
    CcspTraceInfo(("New Cfg: %s\n", newConfPath));
    CcspTraceInfo(("Cur Cfg: %s\n", defPath));
    CcspTraceInfo(("Dev Cfg: %s\n", curPath));

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
//    CcspTraceInfo(("ssp_CfmUpdateConfigs ends\n"));    
    return err;
}
