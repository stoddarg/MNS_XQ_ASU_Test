/*
 * TwoDHisto.h
 *
 *  Created on: Feb 4, 2019
 *      Author: gstoddard
 */

#ifndef SRC_TWODHISTO_H_
#define SRC_TWODHISTO_H_

#include "xil_printf.h"
#include "ff.h"
#include <math.h>
#include "lunah_defines.h"
#include "DataAcquisition.h"
#include "RecordFiles.h"

//function prototypes
int Save2DHToSD( int pmt_ID );
unsigned int Get_2DHXIndex( void );
unsigned int Get_2DHYIndex( void );
int Tally2DH(int energy_bin, int psd_bin, int pmt_ID);

#endif /* SRC_TWODHISTO_H_ */
