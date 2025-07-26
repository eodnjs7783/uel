/**
 * @file
 *   Report (RPT) library
 */
#include "rpt_task.h"
#include "common_types.h"
#include "rpt_mission_cfg.h"
#include "rpt_msgids.h"
#include "rpt_utils.h"
#include "rpt_mission_cfg.h"
#include "../inc/rpt_eventids.h"

#include <unistd.h>
#include <fcntl.h>

void RPT_FIFO_Init(void) {

    RPT_Data.RptQueue.Head = 0;
    RPT_Data.RptQueue.Count = 0;

    RPT_Data.CritQueue.Head = 0;
    RPT_Data.CritQueue.Count = 0;
}

int32 RPT_PriorInit(void) {

    RPT_FIFO_Init();

    return CFE_SUCCESS;
}

void RPT_Enqueue(const RPT_Report_t *Report, bool IsCritical) {

    if (IsCritical) {
        OS_MutSemTake(RPT_Data.CritMutexID);

        RPT_Data.CritQueue.Entry[RPT_Data.CritQueue.Head] = *Report;
        RPT_Data.CritQueue.Head = (RPT_Data.CritQueue.Head + 1) % RPT_CRITICAL_QUEUE_LEN;

        if (RPT_Data.CritQueue.Count < RPT_CRITICAL_QUEUE_LEN) RPT_Data.CritQueue.Count ++;

        OS_MutSemGive(RPT_Data.CritMutexID);

        /**
         * Append Time info - Only for critical
         */
        OS_MutSemTake(RPT_Data.OpsMutexID);
        RPT_Data.CritQueue.Time[RPT_Data.CritQueue.Head].Seconds = RPT_Data.OpsData.TimeSec;
        RPT_Data.CritQueue.Time[RPT_Data.CritQueue.Head].Subseconds = RPT_Data.OpsData.TimeSubsec;
        OS_MutSemGive(RPT_Data.OpsMutexID);
    }
    else {
        OS_MutSemTake(RPT_Data.ReportMutexID);

        RPT_Data.RptQueue.Entry[RPT_Data.RptQueue.Head] = *Report;
        RPT_Data.RptQueue.Head = (RPT_Data.RptQueue.Head +1) % RPT_REPORT_QUEUE_LEN;

        if (RPT_Data.RptQueue.Count < RPT_REPORT_QUEUE_LEN) RPT_Data.RptQueue.Count ++;
        
        OS_MutSemGive(RPT_Data.ReportMutexID);
    }

}

int32 RPT_Report(const RPT_Report_t *Report, bool IsCritical) {
    /**
     * Invoked by SB Msg
     * 1. Get report.
     * 2. Msg init with `RPT_REPORT_TLM_MID` or `RPT_CRITICAL_TLM_MID`
     * 3. Publish Msg
     * 4. TO will ingest this.
     */
    int32 Status;

    RPT_ReportTlm_t RPT_Tlm = {0,};

    if (IsCritical) {
        Status = CFE_MSG_Init(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader), CFE_SB_ValueToMsgId(RPT_CRITICAL_TLM_MID), sizeof(RPT_Tlm));    
    }
    else Status = CFE_MSG_Init(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader), CFE_SB_ValueToMsgId(RPT_REPORT_TLM_MID), sizeof(RPT_Tlm));
    
    if (Status == CFE_SUCCESS) {
        RPT_Tlm.Payload = *Report;
        CFE_SB_TimeStampMsg(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader));
        Status = CFE_SB_TransmitMsg(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader), false);
    }

    RPT_Enqueue(Report, IsCritical);
    OS_printf("Enqueue Done.\n");

    return Status;
}

int32 RPT_MultipleReport(uint8_t StartIdx, uint8_t TotNum, uint8_t IsCritical) {
    /**
     * Invoked by GS command
     */
    int32 Status;

    uint8_t QueueLen;
    uint8_t QueueCnt;
    uint8_t Start;
    RPT_Report_t *Entry;
    CFE_SB_MsgId_t MID;
    osal_id_t MutexID;

    if (IsCritical) {
        Entry = RPT_Data.CritQueue.Entry;
        QueueLen = RPT_CRITICAL_QUEUE_LEN;
        QueueCnt = RPT_Data.CritQueue.Count;
        MID = CFE_SB_ValueToMsgId(RPT_CRITICAL_TLM_MID);
        MutexID = RPT_Data.CritMutexID;
    }
    else {
        Entry = RPT_Data.RptQueue.Entry;
        QueueLen = RPT_REPORT_QUEUE_LEN;
        QueueCnt = RPT_Data.RptQueue.Count;
        MID = CFE_SB_ValueToMsgId(RPT_REPORT_TLM_MID);
        MutexID = RPT_Data.ReportMutexID;
    }

    OS_MutSemTake(MutexID);

    /**
     * Parameter validation
     */
    if (StartIdx >= QueueLen || TotNum >= QueueLen) return -1;
    if (StartIdx + TotNum > QueueCnt) TotNum = QueueCnt;

    size_t TlmSize = sizeof(RPT_MultipleReportTlm_t) + TotNum * sizeof(RPT_Report_t);
    
    RPT_MultipleReportTlm_t *RPT_MultipleTlm = (RPT_MultipleReportTlm_t *)calloc(1, TlmSize);
    if (RPT_MultipleTlm == NULL) return -1;

    Status = CFE_MSG_Init(CFE_MSG_PTR(RPT_MultipleTlm->TelemetryHeader), MID, TlmSize);
    if (Status != CFE_SUCCESS) goto cleanup;

    uint8_t OldIdx = (IsCritical? RPT_Data.CritQueue.Head : RPT_Data.RptQueue.Head) + QueueLen - QueueCnt;

    Start = (OldIdx + StartIdx) % QueueLen;
    for (uint8_t i = 0; i < TotNum; i++) {
        RPT_MultipleTlm->Payload[i] = Entry[(Start + i) % QueueLen];
    }

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(RPT_MultipleTlm->TelemetryHeader));
    Status = CFE_SB_TransmitMsg(CFE_MSG_PTR(RPT_MultipleTlm->TelemetryHeader), false);

cleanup:
    OS_MutSemGive(MutexID);

    free(RPT_MultipleTlm);
    RPT_MultipleTlm = NULL;

    return Status;
}

bool RPT_VerifyReportLength(const CFE_MSG_Message_t *MsgPtr) {
    int32 Status;
    bool Result = true;
    size_t ActualSize;

    Status = CFE_MSG_GetSize(MsgPtr, &ActualSize);
    if (Status != CFE_SUCCESS) return false;

    if (ActualSize != sizeof(RPT_ReportTlm_t)) Result = false;

    return Result;
}



/**********************************
 * 
 * Operation Data function
 * 
 **********************************/
int RPT_OpenOpsFile(void) {
    int FD;

    FD = open(RPT_OPS_DATA_PATH, O_CREAT | O_RDWR, 0666);
    OS_printf("RPT Ops FD: %d\n", FD);

    // If, error occur, return -1
    return FD;
}

int32 RPT_WriteOpsToFile(int FD, const void *Data, size_t Size) {
    int32 Status;

    lseek(FD, 0, SEEK_SET);

    Status = write(FD, Data, Size);
    if (Status != Size) return -1;
    Status = fsync(FD);
    if (Status != 0) return -2;

    return CFE_SUCCESS;
}

int32 RPT_ReadOpsFromFile(int FD, void *Data, size_t Size) {
    return read(FD, Data, Size);
}

int32 RPT_CloseFile(int FD) {
    return close(FD);
}