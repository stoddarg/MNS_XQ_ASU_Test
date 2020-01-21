/*
 * main.h
 *
 *  Created on: Apr 24, 2018
 *      Author: gstoddard
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <stdio.h>
#include <xil_io.h>
#include "platform.h"
#include "ps7_init.h"
#include "xscugic.h"		// Global interrupt controller
#include "xaxidma.h"		// AXI DMA handler
#include "xparameters.h"	// SDK generated parameters
#include "platform_config.h"
#include "xgpiops.h"		// GPIO handlers
#include "xuartps.h"		// UART handler
#include "xil_printf.h"		// will remove when we move entirely to packets
#include "sleep.h"			// needed for one usleep() call
#include "xtime_l.h"		// timing
#include "xsdps.h"			// SD device driver
#include "ff.h"				// SD libraries
#include "xil_cache.h"		// for cache invalidation
#include "LI2C_Interface.h"	// IIC handler // Includes "xiicps.h"

//GJS CODE LIBRARIES
#include "ReadCommandType.h"
#include "SetInstrumentParam.h"
#include "process_data.h"
#include "lunah_defines.h"
#include "lunah_utils.h"
#include "LogFileControl.h"
#include "DataAcquisition.h"
#include "RecordFiles.h"

//Global Interrupt Control Variables
//These need to be global for interrupts to be handled appropriately within the system
static XScuGic_Config *GicConfig; 	// Configuration parameters of the controller
XScuGic InterruptController;		// Interrupt controller

// Methods
int InitializeAXIDma( void ); 		// Initialize AXI DMA Transfer
int InitializeInterruptSystem(u16 deviceID);
void InterruptHandler ( void );
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr);

#endif /* SRC_MAIN_H_ */
