/**
 * @file
 *   Report (RPT) library
 */

#include "rpt.h"

RPT_ReportTlm_t RPT_Tlm = {0,};
RPT_ReportAllTlm_t RPT_AllTlm = {0,};

RPT_ReportQueue_t FIFO = {0,};

void RPT_FIFO_Init(void) {
    FIFO.Head = 0;
    FIFO.Count = 0;
}

int32 RPT_Init(void) {
    int32 Status;

    RPT_FIFO_Init();

    Status = CFE_MSG_Init(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader),
                        CFE_SB_ValueToMsgId(RPT_MSGID), sizeof(RPT_ReportTlm_t));
    if (Status != CFE_SUCCESS) {
        return -1;
    }
    Status = CFE_MSG_Init(CFE_MSG_PTR(RPT_AllTlm.TelemetryHeader),
                        CFE_SB_ValueToMsgId(RPT_ALL_MSGID), sizeof(RPT_ReportAllTlm_t));
    if (Status != CFE_SUCCESS) {
        return -1;
    }

    return CFE_SUCCESS;
}

void RPT_Enqueue(RPT_Report_t *Report) {
    FIFO.Entry[FIFO.Head] = *Report;
    FIFO.Head = (FIFO.Head + 1) % RPT_ENTRY_LENGTH;

    if (FIFO.Count < RPT_ENTRY_LENGTH) FIFO.Count ++;
}

int32 RPT_Report(RPT_Report_t *Report) {
    int32 Status;

    memcpy(&RPT_Tlm.Payload, Report, sizeof(RPT_Report_t));

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader));
    Status = CFE_SB_TransmitMsg(CFE_MSG_PTR(RPT_Tlm.TelemetryHeader), false);

    RPT_Enqueue(Report);

    return Status;
}

int32 RPT_ReportAll(void) {
    int32 Status;

    uint8_t Start;
    Start = (FIFO.Head + RPT_ENTRY_LENGTH - FIFO.Count) % RPT_ENTRY_LENGTH;

    for (uint8_t i = 0; i < FIFO.Count; i++) {
        RPT_AllTlm.Payload[i] = FIFO.Entry[(Start + i) % RPT_ENTRY_LENGTH];
    }

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(RPT_AllTlm.TelemetryHeader));
    Status = CFE_SB_TransmitMsg(CFE_MSG_PTR(RPT_AllTlm.TelemetryHeader), true);

    return Status;
}