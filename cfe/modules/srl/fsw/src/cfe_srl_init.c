/* Auto-Generated file. Never change this code! */
/**
 * Required header files
*/
#include "cfe_srl_module_all.h"

/**
 * Global data
*/
CFE_SRL_IO_Handle_t *Handles[CFE_SRL_GNRL_DEVICE_NUM];
/**************************************************
 * Index of Each device
 * 0 : Not initializaed padding Handle
 **************************************************/

CFE_SRL_GPIO_Handle_t GPIO[CFE_SRL_TOT_GPIO_NUM];


/************************************************************************
 * Early Initialization function executed at cFE ES
 * Append object to `cfe_es_objtab.c`
 * Declaration is located at
 * `cfe/modules/core_private/fsw/inc/cfe_srl_core_internal.h`
 ************************************************************************/
int32 CFE_SRL_EarlyInit(void) {
	int32 Status;

	Status = CFE_SRL_PriorInit();
	if(Status != CFE_SUCCESS) return Status;
	CFE_ES_WriteToSysLog("%s: Prior Initialized.", __func__);
	/**************************************************
	 * Serial Comm. Init
 	 * Only `ready == true` interface is initialized
	 **************************************************/
	/* GPIO ADCS_EN Init */
	Status = CFE_SRL_GpioInit(&GPIO[CFE_SRL_ADCS_EN_GPIO_INDEXER], "/dev/gpiochip2", 3, "ADCS_EN", 0, false);
	if (Status != CFE_SUCCESS) {
		CFE_ES_WriteToSysLog("%s: GPIO ADCS_EN Initialization failed! RC=%d\n", __func__, Status);
		return CFE_SRL_ADCS_EN_INIT_ERR;
	}

	/* GPIO STX_EN Init */
	Status = CFE_SRL_GpioInit(&GPIO[CFE_SRL_STX_EN_GPIO_INDEXER], "/dev/gpiochip2", 3, "STX_EN", 0, true);
	if (Status != CFE_SUCCESS) {
		CFE_ES_WriteToSysLog("%s: GPIO STX_EN Initialization failed! RC=%d\n", __func__, Status);
		return CFE_SRL_STX_EN_INIT_ERR;
	}

	Status = CFE_SRL_InitCSP();
	if (Status != CFE_SUCCESS) {
		CFE_ES_WriteToSysLog("%s: CSP Initialization failed! RC=%d\n", __func__, Status);
		return CFE_SRL_CSP_INIT_ERR;
	}
	CFE_ES_WriteToSysLog("%s: CSP Successfully Initialized.\n", __func__);

return CFE_SUCCESS;
}
