/*
 * ReadCommandType.h
 *
 *  Created on: Apr 12, 2018
 *      Author: gstoddard
 */

#ifndef SRC_READCOMMANDTYPE_H_
#define SRC_READCOMMANDTYPE_H_

#include <stdio.h>		//needed for unsigned types
#include <string.h>		//needed for mem functions
#include <sleep.h>		//needed for usleep
#include "xil_cache.h"	//TEST
#include "xuartps.h"	//needed for uart functions
#include "lunah_defines.h"
#include "DataAcquisition.h"

void bufferShift(char * buff, int bytes_to_del, int buff_strlen);
char * GetLastCommand( void );
unsigned int GetLastCommandSize( void );
int ReadCommandType(char * RecvBuffer, XUartPs *Uart_PS);
int GetIntParam( int param_num );
float GetFloatParam( int param_num );
unsigned int GetRealTimeParam( void );
char * GetFileToAccess( void );

#endif /* SRC_READCOMMANDTYPE_H_ */
