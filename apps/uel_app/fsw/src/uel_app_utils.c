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
 *   This file contains the source code for the UEL utility functions
 */

/*
** Include Files:
*/
#include "osapi.h"
#include "uel_app.h"
#include "uel_app_eventids.h"
#include "uel_app_tbl.h"
#include "uel_app_utils.h"
#include "uel_app_mission_cfg.h"
#include "osapi.h"
#include <csp/csp.h>
#include <csp/csp_cmp.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Verify contents of First Example Table buffer contents                  */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t UEL_APP_TblValidationFunc(void *TblData)
{
    CFE_Status_t               ReturnCode = CFE_SUCCESS;
    UEL_APP_ExampleTable_t *TblDataPtr = (UEL_APP_ExampleTable_t *)TblData;

    /*
    ** Sample Example Table Validation
    */
    if (TblDataPtr->Int1 > UEL_APP_TBL_ELEMENT_1_MAX)
    {
        /* First element is out of range, return an appropriate error code */
        ReturnCode = UEL_APP_TABLE_OUT_OF_RANGE_ERR_CODE;
    }

    return ReturnCode;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Output CRC                                                      */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void UEL_APP_GetCrc(const char *TableName)
{
    CFE_Status_t   status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("UEL: Error Getting Example Table Info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("UEL: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }
}



static void UEL_APP_CANTask(void)                          
{
    
    csp_socket_t *sock = csp_socket(CSP_SO_NONE);     
    if (sock == NULL)                                         
    {
        CFE_EVS_SendEvent(UEL_APP_CAM_EVT_TASK_ERR_EID,      
                          CFE_EVS_EventType_ERROR,
                          "CAM CAN: socket create failed");
        CFE_ES_ExitChildTask();                             
    }

   
    if (csp_bind(sock, CSP_ANY) != CSP_ERR_NONE)              // 소켓을 모든 포트(CSP_ANY)로 바인딩
    {
        CFE_EVS_SendEvent(UEL_APP_CAM_EVT_TASK_ERR_EID,      
                          CFE_EVS_EventType_ERROR,
                          "CAM CAN: bind ANY failed");
        CFE_ES_ExitChildTask();                         
    }

    /* 연결 대기 큐 설정 */
    if (csp_listen(sock, 5) != CSP_ERR_NONE)                  
    {
        CFE_EVS_SendEvent(UEL_APP_CAM_EVT_TASK_ERR_EID,       
                          CFE_EVS_EventType_ERROR,
                          "CAM CAN: listen failed");
        CFE_ES_ExitChildTask();                              
    }

   
    while (1)                                                 
    {
        /* 1000ms 타임아웃으로 연결 수락 */
        csp_conn_t *conn = csp_accept(sock, 1000);        
        if (conn == NULL)                                    
        {
            /* 타임아웃: 계속 대기 */
            continue;                                        
        }

        /* 패킷 수신 (연결 성립 후 즉시 수신 기대) */
        csp_packet_t *pkt = csp_read(conn, 0);                // 연결에서 패킷 1개 수신(블록 없음: 타임아웃 0)
        if (pkt != NULL)                                      
        {
            if (pkt->length >= 2)                             // 최소 헤더(CMD 1바이트 + LEN 1바이트) 이상인지 확인
            {
                const uint8_t *buf = pkt->data;              
                uint8_t cmd   = buf[0];                       
                uint8_t plen  = buf[1];                       // payload length

                /* 길이 검증: [CMD(1) + LEN(1) + PAYLOAD(plen)] <= pkt->length */
                if ((uint16)2 + (uint16)plen <= pkt->length)  // 패킷 버퍼에 선언된 페이로드가 실제로 들어있나 점검(오버런 방지)
                {
                    if (cmd == UEL_APP_SENSOR_VALUES)         
                    {
                        /* CMD 0x20: 센싱 통합 데이터 수신 */
                        const uint8 *p = &buf[2];             // 페이로드 시작 위치(헤더 2바이트 건너뜀)
                        int idx = 0;                          // 페이로드 파싱용 오프셋

                        for (int i = 0; i < 3; i++)          
                        {
                            UEL_APP_Data.SenseCurrent[i] =    
                                (uint16)((p[idx] << 8) | p[idx+1]);
                            idx += 2;                         
                        }
                        
                        for (int i = 0; i < 3; i++)           
                        {
                            UEL_APP_Data.SenseVoltage[i] =
                                (uint16)((p[idx] << 8) | p[idx+1]);
                            idx += 2;
                        }
                        
                        UEL_APP_Data.SenseTemp = p[idx++];    // 온도는 1바이트(예: °C 단위 원시값)

                        /* Yaw, Pitch, Roll (각 2바이트) */
                        UEL_APP_Data.SenseYaw   = (int16)((p[idx] << 8) | p[idx+1]);  idx += 2;  // 0.1° 단위 값 가정
                        UEL_APP_Data.SensePitch = (int16)((p[idx] << 8) | p[idx+1]);  idx += 2;
                        UEL_APP_Data.SenseRoll  = (int16)((p[idx] << 8) | p[idx+1]);  idx += 2;
                        
                        /*
                        CFE_EVS_SendEvent(UEL_APP_CAM_SENSE_RCV_EID,                   // 정보성 이벤트로 수신값 로깅
                            CFE_EVS_EventType_INFORMATION,
                            "COSMIC: SENSE Rcvd I=[%u,%u,%u]mA V=[%u,%u,%u]mV T=%u°C Y=%.1f° P=%.1f° R=%.1f°",
                            UEL_APP_Data.SenseCurrent[0],                              // 전류 3채널(mA)
                            UEL_APP_Data.SenseCurrent[1],
                            UEL_APP_Data.SenseCurrent[2],
                            UEL_APP_Data.SenseVoltage[0],                              // 전압 3채널(mV)
                            UEL_APP_Data.SenseVoltage[1],
                            UEL_APP_Data.SenseVoltage[2],
                            UEL_APP_Data.SenseTemp,                                    
                            UEL_APP_Data.SenseYaw,                              
                            UEL_APP_Data.SensePitch, 
                            UEL_APP_Data.SenseRoll  
                        
                        );
                        */
                    }
                    else                                      
                    {
                        /* 다른 CMD: 필요 시  */   
                    }
                }
                else                                           // LEN이 선언한 길이가 실제 패킷 길이를 초과 → 손상/오류
                {
                    CFE_EVS_SendEvent(UEL_APP_CAM_EVT_RECV_ERR_EID,
                                      CFE_EVS_EventType_ERROR,
                                      "CAM CAN: length mismatch (LEN=%u, PKT=%u)",
                                      (unsigned)plen, (unsigned)pkt->length);
                }
            }
            /* pkt가 너무 짧으면 무시 */
            csp_buffer_free(pkt);                              // 패킷 버퍼 반환(메모리 누수 방지)
        }

        csp_close(conn);                                       // 연결 종료(다음 연결 수락 준비)
    }
}

CFE_Status_t UEL_APP_StartCANListener(void)
{
    CFE_ES_TaskId_t s_CamCanTaskId = 0;
    /* 중복 시작 방지(필요 시 단순 체크) */
    if (s_CamCanTaskId != 0)
    {
        return CFE_SUCCESS;
    }

    CFE_Status_t status = CFE_ES_CreateChildTask(&s_CamCanTaskId,
                                                 "listener",
                                                 UEL_APP_CANTask,
                                                 NULL,
                                                 (2 * 1024),
                                                 95,
                                                 0);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(UEL_APP_CAM_EVT_TASK_ERR_EID, CFE_EVS_EventType_ERROR,
                          "CAM CAN: listener task start failed, RC=0x%08lX",
                          (unsigned long)status);
    }

    return status;
}
