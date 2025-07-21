#ifndef RPT_H
#define RPT_H

#include "cfe.h"
#include "cfe_core_api_base_msgids.h"

/**
 * RPT Topic IDs
 */
#define RPT_TOPICID         0xFE
#define RPT_ALL_TOPICID     0xFF

/**
 * RPT Msg IDs
 */
#define RPT_MSGID           CFE_PLATFORM_TLM_TOPICID_TO_MIDV(RPT_TOPICID) /* 0x08FE */
#define RPT_ALL_MSGID       CFE_PLATFORM_TLM_TOPICID_TO_MIDV(RPT_ALL_TOPICID) /* 0x08FF */

/**
 * RPT Interface definition
 */
#define RPT_ENTRY_LENGTH        10
#define RPT_RET_VALUE_BUF_SIZE  60

typedef struct {
    uint16 MsgID;
    uint8 CommandCode;

    uint8 ReturnType;
    int32 ReturnCode;

    uint16 ReturnDataSize;
    uint8 ReturnValue[RPT_RET_VALUE_BUF_SIZE];
} RPT_Report_t;

typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    RPT_Report_t Payload;
} RPT_ReportTlm_t;

typedef struct {
    CFE_MSG_TelemetryHeader_t TelemetryHeader;
    RPT_Report_t Payload[10];
} RPT_ReportAllTlm_t;

typedef struct {
    RPT_Report_t Entry[RPT_ENTRY_LENGTH];
    uint8_t Head;
    uint8_t Count;
} RPT_ReportQueue_t;



int32 RPT_Init(void);
int32 RPT_Report(RPT_Report_t *Report);
int32 RPT_ReportAll(void);

#endif /* RPT_H */