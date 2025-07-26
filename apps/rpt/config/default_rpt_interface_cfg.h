#ifndef RPT_INTERFACE_CFG_H
#define RPT_INTERFACE_CFG_H

#include "common_types.h"

/**
 * Queue Length
 */
#define RPT_REPORT_QUEUE_LEN    10
#define RPT_CRITICAL_QUEUE_LEN  40

/**
 * Report Buffer Size
 */
#define RPT_RET_VALUE_BUF_SIZE  60


/**
 * RPT Subscribe Table Entry size
 */
#define RPT_MAX_TBL_ENTRY       30

typedef struct {
    uint16 MsgID;
    uint8 CommandCode;

    uint8 ReturnType;
    int32 ReturnCode;

    uint16 ReturnDataSize;
    uint8 ReturnValue[RPT_RET_VALUE_BUF_SIZE];
} RPT_Report_t;

typedef RPT_Report_t RPT_Critical_t;


#endif
