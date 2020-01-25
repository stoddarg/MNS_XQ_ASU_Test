/*
 * CPSDataProduct.h
 *
 *  Created on: Jan 18, 2019
 *      Author: gstoddard
 */

#ifndef SRC_CPSDATAPRODUCT_H_
#define SRC_CPSDATAPRODUCT_H_

#include <stdbool.h>
#include "lunah_utils.h"	//access to module temp
#include "SetInstrumentParam.h"	//access to the neutron cuts

//#define CPS_EVENT_SIZE	14
#define CPS_EVENT_SIZE	40

/*
 * This is the CPS event structure and has the follow data fields:
 * 	event ID 		= 0x55
 * 	---------- Per-module numbers
 * 	n_ellipse1 		= events which are within the first ellipse
 * 	n_ellipse2 		= events which are within the second ellipse
 * 	non_n_events 	= events which are outside both ellipses are classified as non-neutron events
 * 	high_energy_events = events with an energy above our dynamic range
 *  ---------- Per-module numbers
 *  event_counts 	= total number of event windows opened by the FPGA in the current 1-second interval
 *  time_MSB 		= FPGA time from the beginning of the current 1s interval (extremely important!!!)
 *  time_LSB1
 *  time_LSB2
 *  time_LSB3
 *
 * There is one set of per-module numbers reported for each PMT, they are numbered 0-3
 *
 */
typedef struct {
	unsigned char event_id;
	char modu_temp;
	unsigned short n_ellipse1_0;
	unsigned short n_ellipse2_0;
	unsigned short non_n_events_0;
	unsigned short high_energy_events_0;
	unsigned short n_ellipse1_1;
	unsigned short n_ellipse2_1;
	unsigned short non_n_events_1;
	unsigned short high_energy_events_1;
	unsigned short n_ellipse1_2;
	unsigned short n_ellipse2_2;
	unsigned short non_n_events_2;
	unsigned short high_energy_events_2;
	unsigned short n_ellipse1_3;
	unsigned short n_ellipse2_3;
	unsigned short non_n_events_3;
	unsigned short high_energy_events_3;
	unsigned short event_counts;
	unsigned char time_MSB;
	unsigned char time_LSB1;
	unsigned char time_LSB2;
	unsigned char time_LSB3;
}CPS_EVENT_STRUCT_TYPE;

//Function Prototypes
void CPSSetCuts( void );
void CPSInit( void );
void CPSResetCounts( void );
void cpsSetFirstEventTime( unsigned int time );
unsigned int cpsGetFirstEventTime( void );
unsigned int cpsGetCurrentTime( void );
void cpsInitStartTime(void);
float convertToSeconds( unsigned int time );
unsigned int convertToCycles( float time );
bool cpsCheckTime( unsigned int time );
CPS_EVENT_STRUCT_TYPE * cpsGetEvent( void );
bool CPSIsWithinEllipse( int energy, int psd, int pmt_id, int module_num, int ellipse_num );
int CPSUpdateTallies(int energy_bin, int psd_bin, int pmt_id);

#endif /* SRC_CPSDATAPRODUCT_H_ */
