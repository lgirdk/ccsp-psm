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

    module:	psm_sysro_storage.c

        For Persistent Storage Manager Implementation (PSM),
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    description:

        This module implements the advanced storage functions
        of the Psm Sys Registry Object.

        *   PsmSysroResetToFactoryDefault
        *   PsmSysroImportConfig
        *   PsmSysroExportConfig
        *   PsmSysroGetConfigSize
        *   PsmSysroSaveConfigToFlash

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

#include <unistd.h>

#include "psm_sysro_global.h"
#include "safec_lib_common.h"

ULONG
AnscCryptoDesDecrypt
    (
        PVOID                       cipher,
        ULONG                       size,
        PVOID                       plain,
        PANSC_CRYPTO_KEY            key,
        PANSC_CRYPTO_IV             iv
    );

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroResetToFactoryDefault
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to restore the factory default Psm
        configuration.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroResetToFactoryDefault
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY  )&pMyObject->Property;
    PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj       = (PANSC_TIMER_DESCRIPTOR_OBJECT)pMyObject->hRegTimerObj;
    char curCfgFileName[PSM_SYS_FILE_PATH_SIZE + PSM_SYS_FILE_NAME_SIZE + 1];

    /*
     * Once the configuration is reversed to the Factory Default, the device will be rebooted auto-
     * matically. Until then, we need to prevent the current configuration file from being over-
     * written by the pending changes.
     */
    pMyObject->bNoSave = TRUE;

    pRegTimerObj->Stop((ANSC_HANDLE)pRegTimerObj);

    snprintf(curCfgFileName, sizeof(curCfgFileName), "%s%s", pProperty->SysFilePath, pProperty->CurFileName);

    returnStatus = (unlink(curCfgFileName) == 0) ? ANSC_STATUS_SUCCESS : ANSC_STATUS_FAILURE;

    snprintf(curCfgFileName, sizeof(curCfgFileName), "%s%s", pProperty->SysFilePath, pProperty->BakFileName);

    returnStatus = (unlink(curCfgFileName) == 0) ? ANSC_STATUS_SUCCESS : ANSC_STATUS_FAILURE;

    return returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroImportConfig
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pCfgBuffer,
                ULONG                       ulCfgSize,
                void*                       pDecryptKey,
                ULONG                       ulKeySize
            );

    description:

        This function is called to import the configuration.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pCfgBuffer
                Specifies the configuration to be imported.

                ULONG                       ulCfgSize
                Specifies the size of the configuration.

                void*                       pDecryptKey
                Specifies the decryption key to be used.

                ULONG                       ulKeySize
                Specifies the size of the decryption key.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroImportConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize,
        void*                       pDecryptKey,
        ULONG                       ulKeySize
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY  )&pMyObject->Property;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader    = (PPSM_FILE_LOADER_OBJECT     )pMyObject->hPsmFileLoader;
    ULONG                           ulFileSize         = (ULONG                        )0;
    void*                           pTmpCfgBuffer      = (void*                        )NULL;
    ANSC_HANDLE                     hTmpCfgFile        = (ANSC_HANDLE                  )NULL;
    char*                           pCfgDataString     = (char*                        )pCfgBuffer;
    ANSC_CRYPTO_KEY                 desDecryptKey;
    ANSC_CRYPTO_IV                  desDecryptIv;
    char                            tmpCfgFileName[128] = {0};
    errno_t                         rc                 = -1;
//    CcspTraceInfo(("\n##PsmSysroImportConfig() begins##\n"));
    if ( pDecryptKey && (ulKeySize >= (ANSC_DES_KEY_SIZE + ANSC_DES_IV_SIZE)) )
    {
        if ( (ulCfgSize % ANSC_DES_BLOCK_SIZE) != 0 )
        {
            returnStatus = ANSC_STATUS_BAD_SIZE;

            goto  EXIT1;
        }
        else
        {
            desDecryptKey.KeyNumber   = 1;
            desDecryptKey.RoundNumber = 1;
            desDecryptKey.Length      = ANSC_DES_KEY_SIZE;

            AnscCopyMemory
                (
                    desDecryptKey.Value[0],
                    pDecryptKey,
                    AnscGetMin2(ulKeySize, ANSC_DES_KEY_SIZE)
                );

            desDecryptIv.Length = ANSC_DES_IV_SIZE;

            AnscCopyMemory
                (
                    desDecryptIv.Value,
                    (void*)((ULONG)pDecryptKey + ANSC_DES_KEY_SIZE),
                    ANSC_DES_IV_SIZE
                );
        }

        ulCfgSize =
        	AnscCryptoDesDecrypt
                (
                    pCfgBuffer,
                    ulCfgSize,
                    pCfgBuffer,
                    &desDecryptKey,
                    &desDecryptIv
                );

        while ( (ulCfgSize > 0) && (pCfgDataString[ulCfgSize - 1] == 0) )
        {
            ulCfgSize--;
        }

        /*
         * We now save the compressed buffer into a temporary file, then load this into a buffer to
         * verify the file contents and also to save this to the current configuration.
         */

        rc = strcpy_s(tmpCfgFileName, sizeof(tmpCfgFileName), pProperty->SysFilePath);
	if(rc != EOK)
	{
            ERR_CHK(rc);
	    return ANSC_STATUS_FAILURE;
	}

        rc = strcat_s(tmpCfgFileName, sizeof(tmpCfgFileName), pProperty->TmpFileName);
        if(rc != EOK)
        {
	    ERR_CHK(rc);
	    return ANSC_STATUS_FAILURE;
	}
        hTmpCfgFile =
            AnscOpenFile
                (
                    tmpCfgFileName,
                    ANSC_FILE_MODE_RDWR | ANSC_FILE_MODE_CREATE,
                    ANSC_FILE_TYPE_RDWR
                );

        if ( !hTmpCfgFile )
        {
            returnStatus = ANSC_STATUS_FAILURE;

            goto EXIT1;
        }

        if ( AnscWriteFile
                (
                    hTmpCfgFile,
                    pCfgBuffer,
                    &ulCfgSize
                ) != ANSC_STATUS_SUCCESS)
        {
            AnscCloseFile(hTmpCfgFile);

            returnStatus = ANSC_STATUS_FAILURE;

            goto EXIT1;
        }

        AnscCloseFile(hTmpCfgFile);

        hTmpCfgFile =
            AnscOpenFile
                (
                    tmpCfgFileName,
                    ANSC_FILE_MODE_READ | ANSC_FILE_MODE_ZLIB_COMPRESSED,
                    ANSC_FILE_TYPE_RDWR
				);

        if ( !hTmpCfgFile )
        {
            returnStatus = ANSC_STATUS_FAILURE;

            goto EXIT1;
        }
        else
        {
            ulFileSize = AnscGetFileSize(hTmpCfgFile);
        }
	/* Coverity Issue Fix - CID: 64024 : Negative Returns */
        if ( ulFileSize <= 0 )
        {
            returnStatus = ANSC_STATUS_FAILURE;

            goto EXIT2;
        }
        else
        {
            pTmpCfgBuffer = AnscAllocateMemory(ulFileSize + 1);
        }

        if ( !pTmpCfgBuffer )
        {
            returnStatus = ANSC_STATUS_RESOURCES;

            goto  EXIT2;
        }
        else
        {
            ULONG                   ulReadSize = ulFileSize;

            if ( AnscReadFile
                    (
                        hTmpCfgFile,
                        pTmpCfgBuffer,
                        &ulReadSize
                    ) != ANSC_STATUS_SUCCESS)
            {
                returnStatus = ANSC_STATUS_FAILURE;

                goto EXIT3;
            }
        }

        if ( pPsmFileLoader->TestRegFile
                (
                    (ANSC_HANDLE)pPsmFileLoader,
                    pTmpCfgBuffer,
                    ulFileSize
                ) != PSM_FLO_ERROR_CODE_noError )
        {
            returnStatus = ANSC_STATUS_BAD_MEDIA;

            goto  EXIT3;
        }
    }

    returnStatus =
        pMyObject->CfmSaveCurConfig
            (
                (ANSC_HANDLE)pMyObject,
                pTmpCfgBuffer,
                ulFileSize
            );


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT3:

    AnscFreeMemory(pTmpCfgBuffer);

EXIT2:

    AnscCloseFile(hTmpCfgFile);

    unlink(tmpCfgFileName);

EXIT1:
//    CcspTraceInfo(("\n##PsmSysroImportConfig() ends##\n"));
    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroExportConfig
            (
                ANSC_HANDLE                 hThisObject,
                void*                       pCfgBuffer,
                PULONG                      pulCfgSize,
                void*                       pEncryptKey,
                ULONG                       ulKeySize
            );

    description:

        This function is called to export the configuration.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

                void*                       pCfgBuffer
                Specifies where the configuration to be exported.

                PULONG                      pulCfgSize
                Specifies the size of the configuration to be returned.

                void*                       pEncryptKey
                Specifies the encryption key to be used.

                ULONG                       ulKeySize
                Specifies the size of the encryption key.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroExportConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        PULONG                      pulCfgSize,
        void*                       pEncryptKey,
        ULONG                       ulKeySize
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
    ANSC_HANDLE                     hCurCfgFile        = (ANSC_HANDLE                )NULL;
    ULONG                           ulOrgBufferSize    = (ULONG                      )*pulCfgSize;
    ANSC_CRYPTO_KEY                 desEncryptKey;
    ANSC_CRYPTO_IV                  desEncryptIv;
    char                            curCfgFileName[128] = {0};
    errno_t                         rc                  = -1;

    rc = strcpy_s(curCfgFileName, sizeof(curCfgFileName), pProperty->SysFilePath);
    if(rc != EOK)
    {
	ERR_CHK(rc);
	return ANSC_STATUS_FAILURE;
    }
    rc = strcat_s(curCfgFileName, sizeof(curCfgFileName), pProperty->CurFileName);
    if(rc != EOK)
    {
	ERR_CHK(rc);
	return ANSC_STATUS_FAILURE;
    }

    hCurCfgFile =
        AnscOpenFile
            (
                curCfgFileName,
                ANSC_FILE_MODE_RDWR,
                ANSC_FILE_TYPE_RDWR
            );
//    CcspTraceInfo(("\n##PsmSysroExportConfig() begins##\n"));
    if ( !hCurCfgFile )
    {
        returnStatus = ANSC_STATUS_FAILURE;

        goto  EXIT1;
    }
    else
    {
        returnStatus =
            AnscReadFile
                (
                    hCurCfgFile,
                    pCfgBuffer,
                    pulCfgSize
                );
    }

    AnscCloseFile(hCurCfgFile);

    if ( pEncryptKey && (ulKeySize >= (ANSC_DES_KEY_SIZE + ANSC_DES_IV_SIZE)) )
    {
        if ( (ulOrgBufferSize % ANSC_DES_BLOCK_SIZE) != 0 )
        {
            returnStatus = ANSC_STATUS_BAD_SIZE;

            goto  EXIT1;
        }
        else
        {
            desEncryptKey.KeyNumber   = 1;
            desEncryptKey.RoundNumber = 1;
            desEncryptKey.Length      = ANSC_DES_KEY_SIZE;

            AnscCopyMemory
                (
                    desEncryptKey.Value[0],
                    pEncryptKey,
                    AnscGetMin2(ulKeySize, ANSC_DES_KEY_SIZE)
                );

            desEncryptIv.Length = ANSC_DES_IV_SIZE;

            AnscCopyMemory
                (
                    desEncryptIv.Value,
                    (void*)((ULONG)pEncryptKey + ANSC_DES_KEY_SIZE),
                    ANSC_DES_IV_SIZE
                );
        }

        *pulCfgSize =
        	AnscCryptoDesDecrypt
                (
                    pCfgBuffer,
                    ulOrgBufferSize,
                    pCfgBuffer,
                    &desEncryptKey,
                    &desEncryptIv
                );
    }


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT1:
//    CcspTraceInfo(("\n##PsmSysroExportConfig() ends##\n"));
    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        PsmSysroGetConfigSize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to retrieve the size of the current
        configuration.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     configuration size.

**********************************************************************/

ULONG
PsmSysroGetConfigSize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
    ANSC_HANDLE                     hCurCfgFile        = (ANSC_HANDLE                )NULL;
    ULONG                           ulFileSize         = (ULONG                      )0;
    char                            curCfgFileName[128] = {0};
    errno_t                         rc                  = -1;

    rc =strcpy_s(curCfgFileName, sizeof(curCfgFileName), pProperty->SysFilePath);
    if(rc != EOK)
    {
	ERR_CHK(rc);
	return ANSC_STATUS_FAILURE;
    }
    rc = strcat_s(curCfgFileName, sizeof(curCfgFileName), pProperty->CurFileName);
    if(rc != EOK)
    {
	ERR_CHK(rc);
	return ANSC_STATUS_FAILURE;
    }

    hCurCfgFile =
        AnscOpenFile
            (
                curCfgFileName,
                ANSC_FILE_MODE_RDWR,
                ANSC_FILE_TYPE_RDWR
            );
//    CcspTraceInfo(("\n##PsmSysroGetConfigSize() begins##\n"));
    if ( !hCurCfgFile )
    {
        return  0;
    }
    else
    {
        ulFileSize = AnscGetFileSize(hCurCfgFile);
    }

    AnscCloseFile(hCurCfgFile);
//    CcspTraceInfo(("\n##PsmSysroGetConfigSize() ends##\n"));
    return  ulFileSize;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroSaveConfigToFlash
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function is called to flush the configuration.

    argument:   ANSC_HANDLE                 hThisObject
                This handle is actually the pointer of this object
                itself.

    return:     status of operation.

**********************************************************************/

ANSC_STATUS
PsmSysroSaveConfigToFlash
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus    = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject       = (PPSM_SYS_REGISTRY_OBJECT    )hThisObject;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader = (PPSM_FILE_LOADER_OBJECT     )pMyObject->hPsmFileLoader;
    PANSC_TIMER_DESCRIPTOR_OBJECT   pRegTimerObj    = (PANSC_TIMER_DESCRIPTOR_OBJECT)pMyObject->hRegTimerObj;

//    CcspTraceInfo(("\n##PsmSysRegistry.SaveConfigToFlash() begins##\n"));

    /*
     * Once the configuration is flushed to permanent storage, the device will be rebooted auto-
     * matically. Until then, we need to prevent the current configuration file from being over-
     * written by the pending changes.
     */
    pMyObject->bNoSave = TRUE;
    AnscAcquireLock(&pMyObject->AccessLock);

    if ( TRUE )
    {
        pRegTimerObj->Stop((ANSC_HANDLE)pRegTimerObj);
    }

    pMyObject->bSaveInProgress = TRUE;

    returnStatus = pPsmFileLoader->SaveRegFile((ANSC_HANDLE)pPsmFileLoader);

    pMyObject->bNeedFlush      = FALSE;
    pMyObject->bSaveInProgress = FALSE;
    pMyObject->LastRegFlushAt  = AnscGetTickInSeconds();
    AnscReleaseLock(&pMyObject->AccessLock);
//    CcspTraceInfo(("\n##PsmSysRegistry.SaveConfigToFlash() ends##\n"));

    return  returnStatus;
}
