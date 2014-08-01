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

    module:	psm_sysro_cfmif.c

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

        *   PsmSysroCfmReadCurConfig
        *   PsmSysroCfmReadDefConfig
        *   PsmSysroCfmSaveCurConfig

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


#include "psm_sysro_global.h"


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroCfmReadCurConfig
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
PsmSysroCfmReadCurConfig
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader    = (PPSM_FILE_LOADER_OBJECT   )pMyObject->hPsmFileLoader;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository = (PSYS_INFO_REPOSITORY_OBJECT)pMyObject->hSysInfoRepository;
    PSYS_IRA_INTERFACE              pIraIf             = (PSYS_IRA_INTERFACE         )pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);
    ANSC_HANDLE                     hCurCfgFile        = (ANSC_HANDLE                )NULL;
    ANSC_HANDLE                     hBakCfgFile        = (ANSC_HANDLE                )NULL;
    ULONG                           ulFileSize         = (ULONG                      )0;
    void*                           pCfgBuffer         = (void*                      )NULL;
    ULONG                           i                  = 0;
    char                            defCfgFileName[128];
    char                            curCfgFileName[128];
    char                            bakCfgFileName[128];

    *ppCfgBuffer = 0;
    *pulCfgSize  = 0;

    /*
     * The System Configuration File may become corrupted or lost if the device just recovered from
     * either a sudden reboot or power loss. It's a good practice to have backup file in place just
     * in case:
     *
     *      - first we try to restore configuration from the current System File. If it
     *        passes the verification, we're done; otherwise,
     *      - we try to restore configuration from the backup System File. If it passes
     *        the verification, we're done; otherwise,
     *      - we try to restore the factory default configuration. This step has to
     *        succeed since we don't have any other plan to back it up.
     */
    for ( i = 0; i < 3; i++ )
    {
        switch ( i )
        {
            case    0 :

                    AnscZeroMemory(curCfgFileName, 128);
                    AnscCopyString(curCfgFileName, pProperty->SysFilePath);
                    AnscCatString (curCfgFileName, pProperty->CurFileName);

                    break;

            case    1 :

                    AnscZeroMemory(bakCfgFileName, 128);
                    AnscCopyString(bakCfgFileName, pProperty->SysFilePath);
                    AnscCatString (bakCfgFileName, pProperty->BakFileName);

                    hBakCfgFile =
                        AnscOpenFile
                            (
                                bakCfgFileName,
                                ANSC_FILE_MODE_RDWR,
                                ANSC_FILE_TYPE_RDWR
                            );

                    if ( hBakCfgFile )
                    {
                        AnscCloseFile(hBakCfgFile);

                        returnStatus =
                            AnscCopyFile
                                (
                                    bakCfgFileName,
                                    curCfgFileName,
                                    TRUE
                                );
                    }
                    else
                    {
                        continue;
                    }

                    break;

            case    2 :

                    /*returnStatus = pMyObject->ResetToFactoryDefault((ANSC_HANDLE)pMyObject);*/
                    if ( TRUE )
                    {
                        AnscZeroMemory(defCfgFileName, 128);
                        AnscCopyString(defCfgFileName, pProperty->SysFilePath);
                        AnscCatString (defCfgFileName, pProperty->DefFileName);

                        AnscZeroMemory(curCfgFileName, 128);
                        AnscCopyString(curCfgFileName, pProperty->SysFilePath);
                        AnscCatString (curCfgFileName, pProperty->CurFileName);

                        returnStatus =
                            AnscCopyFile
                                (
                                    defCfgFileName,
                                    curCfgFileName,
                                    TRUE
                                );
                    }

                    break;

            default :

                    break;
        }

#ifdef  _PSM_FILE_COMPRESSION_ENABLE

        hCurCfgFile =
            AnscOpenFile
                (
                    curCfgFileName,
                    ANSC_FILE_MODE_READ | ANSC_FILE_MODE_ZLIB_COMPRESSED,
                    ANSC_FILE_TYPE_RDWR
                );

#else

        hCurCfgFile =
            AnscOpenFile
                (
                    curCfgFileName,
                    ANSC_FILE_MODE_READ,
                    ANSC_FILE_TYPE_RDWR
                );

#endif

        if ( !hCurCfgFile )
        {
            continue;
        }
        else
        {
            ulFileSize = AnscGetFileSize(hCurCfgFile);
        }

        if ( ulFileSize == 0 )
        {
            AnscCloseFile(hCurCfgFile);

            continue;
        }
        else
        {
            pCfgBuffer = AnscAllocateMemory(ulFileSize + 1);
        }

        if ( !pCfgBuffer )
        {
            *ppCfgBuffer = 0;
            *pulCfgSize  = 0;
            returnStatus = ANSC_STATUS_RESOURCES;

            goto  EXIT2;
        }
        else
        {
            returnStatus =
                AnscReadFile
                    (
                        hCurCfgFile,
                        pCfgBuffer,
                        &ulFileSize
                    );
        }

        if ( pPsmFileLoader->TestRegFile
                (
                    (ANSC_HANDLE)pPsmFileLoader,
                    pCfgBuffer,
                    ulFileSize
                ) != PSM_FLO_ERROR_CODE_noError )
        {
            AnscCloseFile (hCurCfgFile);
            AnscFreeMemory(pCfgBuffer );

            continue;
        }
        else
        {
            *ppCfgBuffer = pCfgBuffer;
            *pulCfgSize  = ulFileSize;
            returnStatus = ANSC_STATUS_SUCCESS;

            AnscCloseFile(hCurCfgFile);

            break;
        }
    }

    if ( i >= 3 )
    {
        returnStatus = ANSC_STATUS_FAILURE;
    }

    goto  EXIT1;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( hCurCfgFile )
    {
        AnscCloseFile(hCurCfgFile);
    }

EXIT1:

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroCfmReadDefConfig
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
PsmSysroCfmReadDefConfig
    (
        ANSC_HANDLE                 hThisObject,
        void**                      ppCfgBuffer,
        PULONG                      pulCfgSize
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader    = (PPSM_FILE_LOADER_OBJECT   )pMyObject->hPsmFileLoader;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository = (PSYS_INFO_REPOSITORY_OBJECT)pMyObject->hSysInfoRepository;
    PSYS_IRA_INTERFACE              pIraIf             = (PSYS_IRA_INTERFACE         )pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);
    ANSC_HANDLE                     hDefCfgFile        = (ANSC_HANDLE                )NULL;
    ULONG                           ulFileSize         = (ULONG                      )0;
    void*                           pCfgBuffer         = (void*                      )NULL;
    char                            defCfgFileName[128];

    AnscZeroMemory(defCfgFileName, 128);
    AnscCopyString(defCfgFileName, pProperty->SysFilePath);
    AnscCatString (defCfgFileName, pProperty->DefFileName);

#ifdef  _PSM_FILE_COMPRESSION_ENABLE

    hDefCfgFile =
        AnscOpenFile
            (
                defCfgFileName,
                ANSC_FILE_MODE_READ | ANSC_FILE_MODE_ZLIB_COMPRESSED,
                ANSC_FILE_TYPE_RDWR
            );

#else

    hDefCfgFile =
        AnscOpenFile
            (
                defCfgFileName,
                ANSC_FILE_MODE_READ,
                ANSC_FILE_TYPE_RDWR
            );

#endif

    if ( !hDefCfgFile )
    {
        returnStatus = ANSC_STATUS_FAILURE;

        goto  EXIT1;
    }
    else
    {
        ulFileSize = AnscGetFileSize(hDefCfgFile);
    }

    if ( ulFileSize == 0 )
    {
        *ppCfgBuffer = 0;
        *pulCfgSize  = 0;
        returnStatus = ANSC_STATUS_SUCCESS;

        goto  EXIT2;
    }
    else
    {
        pCfgBuffer = AnscAllocateMemory(ulFileSize + 1);
    }

    if ( !pCfgBuffer )
    {
        *ppCfgBuffer = 0;
        *pulCfgSize  = 0;
        returnStatus = ANSC_STATUS_RESOURCES;

        goto  EXIT2;
    }
    else
    {
        returnStatus =
            AnscReadFile
                (
                    hDefCfgFile,
                    pCfgBuffer,
                    &ulFileSize
                );
    }

    *ppCfgBuffer = pCfgBuffer;
    *pulCfgSize  = ulFileSize;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( hDefCfgFile )
    {
        AnscCloseFile(hDefCfgFile);
    }

EXIT1:

    return  returnStatus;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_STATUS
        PsmSysroCfmSaveCurConfig
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
PsmSysroCfmSaveCurConfig
    (
        ANSC_HANDLE                 hThisObject,
        void*                       pCfgBuffer,
        ULONG                       ulCfgSize
    )
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    PPSM_SYS_REGISTRY_OBJECT       pMyObject          = (PPSM_SYS_REGISTRY_OBJECT  )hThisObject;
    PPSM_SYS_REGISTRY_PROPERTY     pProperty          = (PPSM_SYS_REGISTRY_PROPERTY)&pMyObject->Property;
    PPSM_FILE_LOADER_OBJECT        pPsmFileLoader    = (PPSM_FILE_LOADER_OBJECT   )pMyObject->hPsmFileLoader;
    PSYS_INFO_REPOSITORY_OBJECT     pSysInfoRepository = (PSYS_INFO_REPOSITORY_OBJECT)pMyObject->hSysInfoRepository;
    PSYS_IRA_INTERFACE              pIraIf             = (PSYS_IRA_INTERFACE         )pSysInfoRepository->GetIraIf((ANSC_HANDLE)pSysInfoRepository);
    ANSC_HANDLE                     hCurCfgFile        = (ANSC_HANDLE                )NULL;
    char                            curCfgFileName[128];
    char                            bakCfgFileName[128];

    /*
     * To prevent the System Configuration File from becoming corrupted or lost when the device is
     * suddenly rebooted or loses power, we practise following counter-measures:
     *
     *      - first we copy the current System File into the backup file;
     *      - we then flush the configuration into the current file.
     */
    if ( TRUE )
    {
        AnscZeroMemory(curCfgFileName, 128);
        AnscCopyString(curCfgFileName, pProperty->SysFilePath);
        AnscCatString (curCfgFileName, pProperty->CurFileName);

        AnscZeroMemory(bakCfgFileName, 128);
        AnscCopyString(bakCfgFileName, pProperty->SysFilePath);
        AnscCatString (bakCfgFileName, pProperty->BakFileName);

        returnStatus =
            AnscCopyFile
                (
                    curCfgFileName,
                    bakCfgFileName,
                    TRUE
                );
    }

#ifdef  _PSM_FILE_COMPRESSION_ENABLE

    hCurCfgFile =
        AnscOpenFile
            (
                curCfgFileName,
                ANSC_FILE_MODE_WRITE | ANSC_FILE_MODE_TRUNC | ANSC_FILE_MODE_ZLIB_COMPRESSED,
                ANSC_FILE_TYPE_RDWR
            );

#else

    hCurCfgFile =
        AnscOpenFile
            (
                curCfgFileName,
                ANSC_FILE_MODE_WRITE | ANSC_FILE_MODE_TRUNC,
                ANSC_FILE_TYPE_RDWR
            );

#endif

    if ( !hCurCfgFile )
    {
        returnStatus = ANSC_STATUS_FAILURE;

        goto  EXIT1;
    }
    else
    {
        returnStatus =
            AnscWriteFile
                (
                    hCurCfgFile,
                    pCfgBuffer,
                    &ulCfgSize
                );
    }

    goto  EXIT2;


    /******************************************************************
                GRACEFUL ROLLBACK PROCEDURES AND EXIT DOORS
    ******************************************************************/

EXIT2:

    if ( hCurCfgFile )
    {
        AnscCloseFile(hCurCfgFile);
    }

EXIT1:

    return  returnStatus;
}
