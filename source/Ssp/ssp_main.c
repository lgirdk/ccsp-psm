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


#ifdef __GNUC__
#if (!defined _NO_EXECINFO_H_)
#include <execinfo.h>
#endif
#endif

#ifdef _ANSC_LINUX
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

#define DEBUG_INI_NAME  "/etc/debug.ini"

#include "ssp_global.h"
#include "ssp_hash.h"
#include "ssp_utils.h"
#include "ssp_custom.h"

BOOL                                bEngaged          = FALSE;
//PPSM_SYS_REGISTRY_OBJECT            pPsmSysRegistry   = (PPSM_SYS_REGISTRY_OBJECT)NULL;
void                               *bus_handle        = NULL;
char                                g_Subsystem[32]   = {0};
BOOL                                g_bLogEnable      = TRUE;

extern char*                        pComponentName;
//#ifdef USE_PLATFORM_SPECIFIC_HAL
//PSM_CFM_INTERFACE                   cfm_ifo;
//#endif

/*
    The 2 config will be set only in default file.
    If they don't exist, we just use default values.
    For old project, they should set the two parameters with true in default file.
*/
char     bPsmSupportUpgradeFromNonDatabaseVersionging = 1;
char     bPsmSupportUpgradeFromNonDatabaseVersionging_writeonetime = 0;

//Change it to 0. TBD
char     bPsmSupportDowngradeToNonDatabaseVersionging = 0;

/*
    The two file content buffer will be used by hash table directly.
    So they will be kept until unloaded.
*/
char    *pPsmDefFileBuffer = NULL;
int      PsmDefFileLen    = 0;

char    *pPsmCurFileBuffer = NULL;
int      PsmCurFileLen    = 0;

int      PsmVersion = 2;

int      bPsmNeedSaving = 0;
int      PsmContentChangedTime = 0;

PSM_FILE_INFO     PsmFileinfo = {{0}};

/*
    Chech whether this buf include <Provision> and </Provision>.
    If yes, it means the file is not partially saved.
    return 1  ---------- It's complete.
           0  ---------- Not complete
*/
int PsmHashXMLIsComplete(char *buf, int length)
{
    if ( strstr(buf,"</Provision>") )
        return 1;
    else 
        return 0;
}


/*
    Read some preconfigured psm config
*/
int  PsmReadPsmConfig()
{
    PPSM_INFO_RECORD             pRecord   = NULL;

    pRecord = PsmHashRecordFind(PSM_SUPPORT_UPGRADE_FROM_NON_DATABASEVERSIONG);   
    if ( pRecord  ){
        if ( pRecord->pDefVal && ( \
            strncmp(pRecord->pDefVal, "TRUE", 4) == 0  || \
            strncmp(pRecord->pDefVal, "true", 4) == 0  || \
            strncmp(pRecord->pDefVal, "True", 4) == 0  || \
            strncmp(pRecord->pDefVal, "1", 1) == 0  ) ) {
            bPsmSupportUpgradeFromNonDatabaseVersionging = 1;
        }else{
            bPsmSupportUpgradeFromNonDatabaseVersionging = 0;
        }
    }

    pRecord = PsmHashRecordFind(PSM_SUPPORT_DOWNGRADE_TO_NON_DATABASEVERSIONG);   
    if ( pRecord  ){
        if ( pRecord->pDefVal && ( \
            strncmp(pRecord->pDefVal, "TRUE", 4) == 0  || \
            strncmp(pRecord->pDefVal, "true", 4) == 0  || \
            strncmp(pRecord->pDefVal, "True", 4) == 0  || \
            strncmp(pRecord->pDefVal, "1", 1) == 0  ) ){
            bPsmSupportDowngradeToNonDatabaseVersionging = 1;
        }else{
            bPsmSupportDowngradeToNonDatabaseVersionging = 0;
        }
    }
    
    return 0;
}

/*
    Read fileversion from:
        PsmFileinfo.CurFileName
        PsmFileinfo.BakFileName if CurFileName doesn't exist.
*/
int PsmGetOldFileVersion(char * filepath, int * pfileVersion)
{   
    FILE            *fp          = fopen(filepath, "r"); 
    char             line[1024]  = {0};
    char            *p           = NULL;
    char            *q           = NULL;
    
    if (NULL == fp) {
        CcspTraceError(("%s(%s) -- file open error.\n", __FUNCTION__, filepath));
        return 0;
    }

    *pfileVersion = 0;
    
    p = fgets(line, sizeof(line), fp);
    while(p){
        if ( strstr(p, "Provision") ){
            if ( p=strstr(p, "Version=\"") ){
                p = p+strlen("Version=\"");
                q = p;
                while(q[0] && q[0]!='\"' ) q++;
                if ( q[0] == '\"' ) {
                    q[0] = '\0';
                    *pfileVersion = atoi(p);
                }
            }
            break;
        }
        p = fgets(line, sizeof(line), fp);
    }

    fclose(fp);

    return 0;
}


/*
    Reload custom parameters.
    If it doesn't exist, add it. 
    If it exists, use custom values to replace cur/def value if it's not same.
*/
int PsmHashLoadCustom(enum CUSTOM_PARAM_LOADING type)
{
    PsmHalParam_t               *cusParams = NULL;
    int                          cusCnt    = 0;
    int                          i         = 0 ;
    PPSM_INFO_RECORD             pRecord   = NULL;
     
    if (type==CUSTOM_PARAM_DEFAULT && ( PsmHal_GetCustomParams_default(&cusParams, &cusCnt) != 0 || cusCnt == 0) )
    {
        CcspTraceWarning(("%s: no custom default config need import\n", __FUNCTION__));
        return 0; /* nothing to do */
    }else if (type==CUSTOM_PARAM_OVERWRITE && ( PsmHal_GetCustomParams_overwrite(&cusParams, &cusCnt) != 0 || cusCnt == 0) )
    {
        CcspTraceWarning(("%s: no custom overwriting config need import\n", __FUNCTION__));
        return 0; /* nothing to do */
    }

    for (i = 0; i < cusCnt; i++)
    {
        if (cusParams[i].name[0] == '\0')
            continue;

        pRecord = PsmHashRecordFind(cusParams[i].name);
        if ( pRecord ){
            if ( pRecord->pCurVal ){
                //CUSTOM_PARAM_OVERWRITE can go here
                if (strlen(pRecord->pCurVal) >= strlen(cusParams[i].value) ){
                    strcpy(pRecord->pCurVal, cusParams[i].value);
                }else{
                    PsmHashRecordFreeString(pRecord->pCurVal);

                    pRecord->pCurVal =  AnscAllocateMemory(strlen(cusParams[i].value)+1);
                    if ( pRecord->pCurVal == NULL ){
                        CcspTraceError(("%s -- record allocation error(%d).\n", __FUNCTION__, __LINE__));
                        continue;
                    }
                    strcpy(pRecord->pCurVal, cusParams[i].value);
                }

                if (pRecord->pDefVal && strcmp(pRecord->pDefVal, pRecord->pCurVal) == 0 ){
                    PsmHashRecordFreeString(pRecord->pCurVal);
                    pRecord->pCurVal =NULL;
                }
            }else if (pRecord->pDefVal) {
                //CUSTOM_PARAM_DEFAULT type must go here
                //CUSTOM_PARAM_OVERWRITE can go here
                if (pRecord->pDefVal && strcmp(pRecord->pDefVal, cusParams[i].value) == 0 ){
                    //Do nothing                    
                    CcspTraceWarning(("%s: same value for %s\n", __FUNCTION__, cusParams[i].name));
                }else if (strlen(pRecord->pDefVal) >= strlen(cusParams[i].value) ){
                    strcpy(pRecord->pDefVal, cusParams[i].value);
                }else{
                    PsmHashRecordFreeString(pRecord->pDefVal);

                    pRecord->pDefVal =  AnscAllocateMemory(strlen(cusParams[i].value)+1);
                    if ( pRecord->pDefVal == NULL ){
                        CcspTraceError(("%s -- record allocation error(%d).\n", __FUNCTION__, __LINE__));
                        continue;
                    }
                    strcpy(pRecord->pDefVal, cusParams[i].value);
                }
            }
        }else {

            pRecord = PsmHashRecordCachePop();
            if ( pRecord == NULL ){
                CcspTraceError(("%s -- record allocation error(%d).\n", __FUNCTION__, __LINE__));
                continue;
            }

            pRecord->pName          = AnscAllocateMemory(strlen(cusParams[i].name)+1);
            if ( pRecord->pName == NULL ) goto FAILURE;
            strcpy(pRecord->pName, cusParams[i].name);

            //CUSTOM_PARAM_DEFAULT type must go here
            //CUSTOM_PARAM_OVERWRITE can go here
            pRecord->pDefVal        =  AnscAllocateMemory(strlen(cusParams[i].value)+1);
            if ( pRecord->pDefVal == NULL ) goto FAILURE;
            strcpy(pRecord->pDefVal, cusParams[i].value);

            pRecord->pType        =  AnscAllocateMemory(strlen("astr")+1);
            if ( pRecord->pType == NULL ) goto FAILURE;
            strcpy(pRecord->pType, "astr");

            pRecord->pContentType   = NULL;
        
            PsmHashRecordAddNewRecord(pRecord);

            continue;
            
FAILURE:
            CcspTraceError(("%s -- record allocation error (%d).\n", __FUNCTION__, __LINE__));
            if(pRecord->pType == NULL )
                AnscFreeMemory(pRecord->pType);
            if(pRecord->pDefVal == NULL )
                AnscFreeMemory(pRecord->pDefVal);
            if(pRecord->pName == NULL )
                AnscFreeMemory(pRecord->pName);
            if(pRecord)
                AnscFreeMemory(pRecord);
            }    
    }

    if ( cusParams ) {
        AnscFreeMemory(cusParams);
        cusParams = NULL;
        cusCnt = 0;
    }
    CcspTraceWarning(("%s: %d \n", __FUNCTION__, __LINE__));

    return 0;
}

/*
    Read def and cur/bak file 
    return : 0 ---- succeed
             1 ---- fail
*/
int PsmLoadFile()
{
    char   curPathName[PSM_SYS_FILE_NAME_SIZE] = {0};
    char   bakPathName[PSM_SYS_FILE_NAME_SIZE] = {0};
    int    fileVersion = 1;

    CcspTraceError(("%s -- line %d \n", __FUNCTION__, __LINE__));

    /*Read Default file first */
    if ( PsmReadFile(PsmFileinfo.DefFileName, &pPsmDefFileBuffer, &PsmDefFileLen) == 0 ){
       if ((!PsmHashXMLIsComplete(pPsmDefFileBuffer, PsmDefFileLen)) || (PsmHashLoadFile(PSM_FILETYPE_DEF, pPsmDefFileBuffer, PsmDefFileLen ) !=0 )){
            CcspTraceError(("%s -- PSM load def file error.\n", __FUNCTION__));
            if ( pPsmDefFileBuffer ){
                AnscFreeMemory(pPsmDefFileBuffer);
                pPsmDefFileBuffer = NULL;
                PsmDefFileLen = 0;
            }
            return 1;
       }
    }else{
        CcspTraceError(("%s -- PSM read def file error.\n", __FUNCTION__));
        return 1;
    }

    CcspTraceError(("%s -- line %d \n", __FUNCTION__, __LINE__));

    /*Read info from  DB */
    PsmHashLoadCustom(CUSTOM_PARAM_DEFAULT);

    CcspTraceError(("%s -- line %d \n", __FUNCTION__, __LINE__));

    PsmReadPsmConfig();
    
    if ( bPsmSupportUpgradeFromNonDatabaseVersionging ){
        PsmGetOldFileVersion(PsmFileinfo.CurFileName, &fileVersion);

        CcspTraceError(("%s --%s PSM cur version is %d \n", __FUNCTION__, PsmFileinfo.CurFileName, fileVersion));
        
        if ( fileVersion <1 ){
            sprintf(curPathName, "%s", PsmFileinfo.CurFileName);
            sprintf(bakPathName, "%s", PsmFileinfo.BakFileName);
            bPsmSupportUpgradeFromNonDatabaseVersionging_writeonetime = 1;
        }else{
            sprintf(curPathName, "%s", PsmFileinfo.NewCurFileName);
            sprintf(bakPathName, "%s", PsmFileinfo.NewBakFileName);
        }
    }else{
        CcspTraceError(("%s -- line %d \n", __FUNCTION__, __LINE__));

        sprintf(curPathName, "%s", PsmFileinfo.NewCurFileName);
        sprintf(bakPathName, "%s", PsmFileinfo.NewBakFileName);
    }

    /*Read cur file */
    if ( PsmReadFile(curPathName, &pPsmCurFileBuffer, &PsmCurFileLen) == 0 ){
        CcspTraceError(("%s -- PSM load  cur file(%s) before.\n", __FUNCTION__, curPathName));
        if ( !pPsmCurFileBuffer || (PsmHashXMLIsComplete(pPsmCurFileBuffer, PsmCurFileLen)) && ( PsmHashLoadFile(PSM_FILETYPE_CUR, pPsmCurFileBuffer, PsmCurFileLen ) == 0 ) ){
            CcspTraceError(("%s -- PSM load  cur file(%s) succeed.\n", __FUNCTION__, curPathName));
            goto CURFINISH;
        }else{
            CcspTraceError(("%s -- PSM load  cur file(%s) error.\n", __FUNCTION__, curPathName));
        }
    }else{
        CcspTraceError(("%s -- PSM read cur file(%s) error.\n", __FUNCTION__, curPathName));
    }

    if ( pPsmCurFileBuffer ){
        AnscFreeMemory(pPsmCurFileBuffer);
        pPsmCurFileBuffer = NULL;
        PsmCurFileLen = 0;
    }

    /*Read bak file */
    if ( PsmReadFile(bakPathName, &pPsmCurFileBuffer, &PsmCurFileLen) == 0 ){
        if ( !pPsmCurFileBuffer || (PsmHashXMLIsComplete(pPsmCurFileBuffer, PsmCurFileLen)) && ( PsmHashLoadFile(PSM_FILETYPE_CUR, pPsmCurFileBuffer, PsmCurFileLen ) == 0) ){
            goto CURFINISH;
        }else{
            CcspTraceError(("%s -- PSM load bak file(%s) error.\n", __FUNCTION__, bakPathName));
        }
    }else{
        CcspTraceError(("%s -- PSM read bak file(%s) error.\n", __FUNCTION__, bakPathName));
    }

    if ( pPsmCurFileBuffer ){
        AnscFreeMemory(pPsmCurFileBuffer);
        pPsmCurFileBuffer = NULL;
        PsmCurFileLen = 0;
    }
    
    return 1;

CURFINISH:
    
    /*Read info from custom again */
    PsmHashLoadCustom(CUSTOM_PARAM_OVERWRITE);
    CcspTraceError(("%s -- PSM load  cur file(%s) load custom.\n", __FUNCTION__, curPathName));

    return 0;
    
}

/*
    Read def and cur/bak file 
    return : 0 ---- succeed
             1 ---- fail
*/
int PsmUnloadFile()
{
    PsmHashUnloadFile(PSM_FILETYPE_FULL);

    if ( pPsmCurFileBuffer ){
        AnscFreeMemory(pPsmCurFileBuffer);
        pPsmCurFileBuffer = NULL;
        PsmDefFileLen = 0;
    }

    if ( pPsmCurFileBuffer ){
        AnscFreeMemory(pPsmCurFileBuffer);
        pPsmCurFileBuffer = NULL;
        PsmCurFileLen = 0;
    }

    return 0;
}

/*
    This function is used to to do the periodic file saving
*/
void * ThreadForFileSaving(void * nouse)
{
    unsigned int       tm = 0;


    //This help skip the syst bootsup
    // Change this. TBD
    sleep(10);
    
    while(1){
        if (bPsmNeedSaving){
            tm = AnscGetTickInSeconds();
            
            if ( ( tm - PsmContentChangedTime) >= PSM_FLUSH_DELAY )
            {
                AnscAcquireLock(&PsmAccessAccessLock);

                if (AnscCopyFile(PsmFileinfo.NewCurFileName, PsmFileinfo.NewBakFileName, TRUE) != ANSC_STATUS_SUCCESS)
                    CcspTraceError(("%s: fail to backup new current config\n", __FUNCTION__));
            
                PsmHashPeriodicSaving(TRUE, PsmFileinfo.NewCurFileName);

                
                //Remove this. TBD
                //CcspTraceInfo(("%s: Start saving new current.\n", __FUNCTION__));

                if ( bPsmSupportDowngradeToNonDatabaseVersionging || bPsmSupportUpgradeFromNonDatabaseVersionging_writeonetime){
                    if (AnscCopyFile(PsmFileinfo.CurFileName, PsmFileinfo.BakFileName, TRUE) != ANSC_STATUS_SUCCESS)
                        CcspTraceError(("%s: fail to backup current config\n", __FUNCTION__));

                    PsmHashPeriodicSaving(FALSE, PsmFileinfo.CurFileName);
                    
                    bPsmSupportUpgradeFromNonDatabaseVersionging_writeonetime = 0;
                    
                    //Remove this. TBD
                    //CcspTraceInfo(("%s: Start saving old current.\n", __FUNCTION__));

                }

                bPsmNeedSaving = FALSE;

                AnscReleaseLock(&PsmAccessAccessLock);
            }
        }
        
        sleep(PSM_TIMER_INTERVAL);
    }
}


static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#if (!defined _NO_EXECINFO_H_)
    void* tracePtrs[100];
    char** funcNames = NULL;
    int i, count = 0;

        int fd;
        const char* path = "/nvram/psmssplite_backtrace";
        fd = open(path, O_RDWR | O_CREAT, 0777);
        if (fd < 0)
        {
            fprintf(stderr, "failed to open backtrace file: %s", path);
            return;
        }
 
    count = backtrace( tracePtrs, 100 );
    backtrace_symbols_fd( tracePtrs, count, fd );
        close(fd);

    funcNames = backtrace_symbols( tracePtrs, count );

    if ( funcNames ) {
        // Print the stack trace
        for( i = 0; i < count; i++ )
           printf("%s\n", funcNames[i] );

        // Free the string pointers
        AnscFreeMemory( funcNames );
    }
#endif
#endif
}

static void daemonize(void) {
    switch (fork()) {
    case 0:
        break;
    case -1:
        // Error
        AnscTrace("Error daemonizing (fork)! %d - %s\n", errno, strerror(
                errno));
        exit(0);
        break;
    default:
        _exit(0);
    }

    if (setsid() <     0) {
        AnscTrace("Error demonizing (setsid)! %d - %s\n", errno, strerror(errno));
        exit(0);
    }

}

void sig_handler(int sig)
{

#ifdef INCLUDE_BREAKPAD
        breakpad_ExceptionHandler();
        signal(SIGUSR1, sig_handler);    
#else
    if ( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        CcspTraceError(("SIGINT received, exiting!\n"));

#if  defined(_DEBUG) 
        _print_stack_backtrace();
#endif
        exit(0);
    }
    else if ( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGUSR1 received!\n"));
    }
    else if ( sig == SIGUSR2 ) {
        CcspTraceInfo(("SIGUSR2 received!\n"));

        if (bEngaged)
        {
            CcspTraceInfo(("PSMLITE is being unloaded ...\n"));
            
            PsmUnloadFile();
                        
            bEngaged = FALSE;
            
            CcspTraceInfo(("PSMLITE has been unloaded.\n"));
        }
    
        CcspTraceError(("Exit!\n"));

        exit(0);
    }
    else if ( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGPIPE received!\n"));
    }
    else {
        /* get stack trace first */
        _print_stack_backtrace();
        CcspTraceError(("Signal %d received, exiting!\n", sig));
        exit(0);
    }
#endif
}

#ifndef INCLUDE_BREAKPAD
static int is_core_dump_opened(void)
{
    FILE *fp;
    char path[256];
    char line[1024];
    char *start, *tok, *sp;
#define TITLE   "Max core file size"

    snprintf(path, sizeof(path), "/proc/%d/limits", getpid());
    if ((fp = fopen(path, "rb")) == NULL)
        return 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if ((start = strstr(line, TITLE)) == NULL)
            continue;

        start += strlen(TITLE);
        if ((tok = strtok_r(start, " \t\r\n", &sp)) == NULL)
            break;

        fclose(fp);

        if (strcmp(tok, "0") == 0)
            return 0;
        else
            return 1;
    }

    fclose(fp);
    return 0;
}
#endif

//Remove this. TBD
//#define _DEBUG

int main(int argc, char* argv[])
{
    int                             cmdChar            = 0;
    BOOL                            bRunAsDaemon       = TRUE;
    int                             idx                = 0;
    FILE                           *fd                 = NULL;
    char                            cmd[64]            = {0};

    pComponentName = CCSP_DBUS_PSM;
    
#ifdef FEATURE_SUPPORT_RDKLOG
    RDK_LOGGER_INIT();
#endif

#if defined(_DEBUG) || defined(_COSA_SIM_)
    AnscSetTraceLevel(CCSP_TRACE_LEVEL_INFO);
#endif

    for (idx = 1; idx < argc; idx++)
    {
        if ( (strcmp(argv[idx], "-subsys") == 0) )
        {
            AnscCopyString(g_Subsystem, argv[idx+1]);
        }
        else if ( strcmp(argv[idx], "-c" ) == 0 )
        {
            bRunAsDaemon = FALSE;
        }
    }

    if ( bRunAsDaemon )
        daemonize();

    /*This is used for ccsp recovery manager */
    fprintf(stderr, "%s -- %d", __FUNCTION__, __LINE__);
    fflush(stderr);
    
    fd = fopen("/var/tmp/PsmSspLite.pid", "w+");
    if ( !fd )
    {
        AnscTrace("Create /var/tmp/PsmSspLite.pid error. \n");
        return 1;
    }
    else
    {
        sprintf(cmd, "%d", getpid());
        fputs(cmd, fd);
        fclose(fd);
    }

    if (is_core_dump_opened())
    {
        signal(SIGUSR1, sig_handler);
        CcspTraceWarning(("Core dump is opened, do not catch signal\n"));
    }
    else
    {
        CcspTraceWarning(("Core dump is NOT opened, backtrace if possible\n"));
        signal(SIGINT, sig_handler);
        /*signal(SIGCHLD, sig_handler);*/
        signal(SIGUSR1, sig_handler);
        signal(SIGUSR2, sig_handler);

        signal(SIGSEGV, sig_handler);
        signal(SIGBUS, sig_handler);
        signal(SIGKILL, sig_handler);
        signal(SIGFPE, sig_handler);
        signal(SIGILL, sig_handler);
        signal(SIGQUIT, sig_handler);
        signal(SIGHUP, sig_handler);
        signal(SIGPIPE, sig_handler);
        signal(SIGALRM, sig_handler);
    }

    gather_info();

    cmd_dispatch('e');

    if ( bRunAsDaemon ) {
        while (1)
            sleep(30);
    }
    else {
        while ( cmdChar != 'q' )
        {
            cmdChar = getchar();
            if (cmdChar < 0) 
            {
                sleep(30);
            }
            else 
            {
                cmd_dispatch(cmdChar);
            }
        }
    }

    if ( bEngaged )
    {
        CcspTraceInfo(("PSMLITE is being unloaded ...\n"));
    
        PsmUnloadFile();
    
        bEngaged = FALSE;
        
        CcspTraceInfo(("PSMLITE has been unloaded.\n"));
    }

    return 0;
}


int  cmd_dispatch(int  command)
{
    int ret = 0;
    pthread_t          threadID;

    switch ( command )
    {
        case    'e' :

                AnscZeroMemory(&PsmFileinfo, sizeof(PsmFileinfo));
                
                sprintf(PsmFileinfo.SysFilePath, "%s", PSM_DEF_XML_CONFIG_FILE_PATH);
                sprintf(PsmFileinfo.DefFileName, "%s%s", PSM_DEF_XML_CONFIG_FILE_PATH, PSM_DEF_XML_CONFIG_FILE_NAME);
                
                sprintf(PsmFileinfo.NewCurFileName, "%s%s", "", PSM_NEW_CUR_XML_CONFIG_FILE_NAME);
                sprintf(PsmFileinfo.NewBakFileName, "%s%s", "", PSM_NEW_BAK_XML_CONFIG_FILE_NAME);
                sprintf(PsmFileinfo.NewTmpFileName, "%s%s", "", PSM_NEW_TMP_XML_CONFIG_FILE_NAME);
                
                sprintf(PsmFileinfo.CurFileName, "%s%s", "", PSM_CUR_XML_CONFIG_FILE_NAME);
                sprintf(PsmFileinfo.BakFileName, "%s%s", "", PSM_BAK_XML_CONFIG_FILE_NAME);
                sprintf(PsmFileinfo.TmpFileName, "%s%s", "", PSM_TMP_XML_CONFIG_FILE_NAME);

                if (!bEngaged)
                {
                    //This must be earlier than Dbus init.
                    PsmHashInit();
                    
                    AnscAcquireLock(&PsmAccessAccessLock);                    
                    CcspTraceWarning(("PSMLITE File parsing ...\n"));
                    ret = PsmLoadFile();

                    AnscReleaseLock(&PsmAccessAccessLock);

                    if (pthread_create(&threadID, NULL, ThreadForFileSaving, NULL)  || pthread_detach(threadID)) 
                        CcspTraceError(("%s error in start ThreadForFileSaving\n", __FUNCTION__));
                    
                    if ( !ret )
                    {
                        //PsmPlugin_Platform_Init();
                        //PsmPlugin_Customer_Init();

                        PsmDbusInit();

                        bEngaged = TRUE;
                        
                        CcspTraceInfo(("PSMLITE started ...\n"));
                    }
                    else
                    {
                        CcspTraceError(("Create PSM Failed ...\n"));
                    }
                    
                }

                break;

        case    'c' :

                if ( bEngaged )
                {
                    CcspTraceInfo(("PSMLITE is being unloaded ...\n"));

                    if ( bus_handle != NULL )
                    {
                        CCSP_Message_Bus_Exit(bus_handle);
                        bus_handle = NULL;
                    }

                    PsmUnloadFile();

                    bEngaged = FALSE;
                    
                    CcspTraceInfo(("PSMLITE has been unloaded.\n"));
                }
                
                break;

        default :

                break;
    }

    return  0;
}


int  gather_info()
{
    AnscTrace("\n\n");
    AnscTrace("        ***************************************************************\n");
    AnscTrace("        ***                                                         ***\n");
    AnscTrace("        ***            PSMLITE Testing App - Simulation                 ***\n");
    AnscTrace("        ***           Common Component Service Platform             ***\n");
    AnscTrace("        ***                                                         ***\n");
    AnscTrace("        ***        Cisco System  , Inc., All Rights Reserved        ***\n");
    AnscTrace("        ***                         2011                            ***\n");
    AnscTrace("        ***                                                         ***\n");
    AnscTrace("        ***************************************************************\n");
    AnscTrace("\n\n");

    return  0;
}


