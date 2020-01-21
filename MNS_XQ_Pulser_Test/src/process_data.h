/*
 * process_data.h
 *
 *  Created on: May 9, 2018
 *      Author: gstoddard
 */

#ifndef SRC_PROCESS_DATA_H_
#define SRC_PROCESS_DATA_H_

#include <string.h>
#include <stdbool.h>
#include "ff.h"
#include "lunah_defines.h"
#include "lunah_utils.h"
#include "SetInstrumentParam.h"
#include "CPSDataProduct.h"
#include "TwoDHisto.h"

//the data from the FPGA are in the following format
//event id = data_raw[iter]
//time = data_raw[iter+1]
//total counts = data_raw[iter+2]
//event number/PMT Hit ID = data_raw[iter+3] //I think this is just a 28/4 bit split across the 32-bit unsigned int
//baseline int = data_raw[iter+4]
//short int = data_raw[iter+5]
//long int = data_raw[iter+6]
//full int = data_raw[iter+7]

typedef struct {
	unsigned char field0;
	unsigned char field1;
	unsigned char field2;
	unsigned char field3;
	unsigned char field4;
	unsigned char field5;
	unsigned char field6;
	unsigned char field7;
}GENERAL_EVENT_TYPE;

//function prototypes
GENERAL_EVENT_TYPE * GetEVTsBufferAddress( void );
void ResetEVTsBuffer( void );
void ResetEVTsIterator( void );
unsigned int GetFirstEventTime( void );
int ProcessData( unsigned int * data_raw );

#endif /* SRC_PROCESS_DATA_H_ */
