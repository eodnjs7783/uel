#ifndef RPT_UTILS_H
#define RPT_UTILS_H

#include "common_types.h"
#include "rpt_mission_cfg.h"


typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    RPT_Report_t Payload;
} RPT_ReportTlm_t;

typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    RPT_Report_t Payload[];
} RPT_MultipleReportTlm_t;


int32 RPT_PriorInit(void);
int32 RPT_Report(const RPT_Report_t *Report, bool IsCritical);
int32 RPT_MultipleReport(uint8_t StartIdx, uint8_t TotNum, uint8_t IsCritical);

bool RPT_VerifyReportLength(const CFE_MSG_Message_t *MsgPtr);

int RPT_OpenOpsFile(void);
int32 RPT_WriteOpsToFile(int FD, const void *Data, size_t Size);
int32 RPT_ReadOpsFromFile(int FD, void *Data, size_t Size);
int32 RPT_CloseFile(int FD);

#endif