/*
 * LogFileControl.h
 *
 *  Created on: Nov 6, 2018
 *      Author: gstoddard
 */

#ifndef SRC_LOGFILECONTROL_H_
#define SRC_LOGFILECONTROL_H_

#include <stdio.h>
#include "ff.h"
#include "xil_printf.h"
#include "lunah_defines.h"

int InitLogFile0( void );
int InitLogFile1( void );
int MountSDCards( FATFS * fs );
int MountSD0( FATFS * fs );
int MountSD1( FATFS * fs );
int LogFileWrite( char * write_buff, unsigned int bytes_to_write );

#endif /* SRC_LOGFILECONTROL_H_ */
