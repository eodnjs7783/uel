/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *   This file contains the source code for the UEL Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "uel_app.h"
#include "uel_app_cmds.h"
#include "uel_app_msgids.h"
#include "uel_app_eventids.h"
#include "uel_app_version.h"
#include "uel_app_tbl.h"
#include "uel_app_utils.h"
#include "uel_app_msg.h"

/* The uel_lib module provides the UEL_Function() prototype */
//#include "uel_lib.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t UEL_APP_SendHkCmd(const UEL_APP_SendHkCmd_t *Msg)
{
    int i;

    /*
    ** Get command execution counters...
    */
    UEL_APP_Data.HkTlm.Payload.CommandErrorCounter = UEL_APP_Data.ErrCounter;
    UEL_APP_Data.HkTlm.Payload.CommandCounter      = UEL_APP_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(UEL_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(UEL_APP_Data.HkTlm.TelemetryHeader), true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < UEL_APP_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(UEL_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* UEL NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t UEL_APP_NoopCmd(const UEL_APP_NoopCmd_t *Msg)
{
    UEL_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(UEL_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "UEL: NOOP command %s",
                      UEL_APP_VERSION);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t UEL_APP_ResetCountersCmd(const UEL_APP_ResetCountersCmd_t *Msg)
{
    UEL_APP_Data.CmdCounter = 0;
    UEL_APP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(UEL_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "UEL: RESET command");

    return CFE_SUCCESS;
}

/*
CFE_Status_t UEL_APP_CamBootCmd(const UEL_APP_CamBootCmd_t *Msg)
{
    UEL_APP_Data.CmdCounter++;

    uint8_t TxData[2] = {0x40, 0x00};

    int32 status = CFE_SRL_ApiTransactionCSP(
        UEL_APP_UEL_OBC_NODE, 
        UEL_APP_CAM_BOOT_PORT, 
        TxData, 
        sizeof(TxData), 
        NULL, 
        0
    );

    if (status == 1){

        CFE_EVS_SendEvent(UEL_APP_CAM_BOOT_INF_EID, CFE_EVS_EventType_INFORMATION, "UEL: CAMERA booting command sent successfully");
        return CFE_SUCCESS;
    }
    else{
        UEL_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(UEL_APP_CAM_BOOT_ERR_EID, CFE_EVS_EventType_ERROR, "UEL: CAMERA booting command transmission failed");
        return status;
    }
}
*/

