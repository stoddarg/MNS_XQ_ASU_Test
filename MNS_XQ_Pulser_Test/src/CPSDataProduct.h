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

#define CPS_EVENT_SIZE	14
//#define CPS_EVENT_SIZE	40

/*
 * This is the CPS event structure and has the follow data fields:
 * 	event ID = 0x55
 * 	n_ellipse1_MSB = events which are within the first ellipse
 * 	n_ellipse1_LSB
 * 	n_ellipse2_MSB = events which are within the second ellipse
 * 	n_ellipse2_LSB
 * 	non_n_events_MSB = events which are outside both ellipses are classified as non-neutron events
 * 	non_n_events_LSB
 * 	high_energy_events_MSB = events with an energy greater than 10 MeV (threshold for energy may change)
 *  high_energy_events_LSB
 *  time_MSB = FPGA time from the beginning of the current 1s interval (extremely important!!!)
 *  time_LSB1
 *  time_LSB2
 *  time_LSB3
 *  modu_temp = the module temperature
 */
typedef struct {
	unsigned char event_id;
	unsigned char n_ellipse1_MSB;
	unsigned char n_ellipse1_LSB;
	unsigned char n_ellipse2_MSB;
	unsigned char n_ellipse2_LSB;
	unsigned char non_n_events_MSB;
	unsigned char non_n_events_LSB;
	unsigned char high_energy_events_MSB;
	unsigned char high_energy_events_LSB;
//	unsigned char n_ellipse1_MSB_1;
//	unsigned char n_ellipse1_LSB_1;
//	unsigned char n_ellipse2_MSB_1;
//	unsigned char n_ellipse2_LSB_1;
//	unsigned char non_n_events_MSB_1;
//	unsigned char non_n_events_LSB_1;
//	unsigned char high_energy_events_MSB_1;
//	unsigned char high_energy_events_LSB_1;
//	unsigned char n_ellipse1_MSB_2;
//	unsigned char n_ellipse1_LSB_2;
//	unsigned char n_ellipse2_MSB_2;
//	unsigned char n_ellipse2_LSB_2;
//	unsigned char non_n_events_MSB_2;
//	unsigned char non_n_events_LSB_2;
//	unsigned char high_energy_events_MSB_2;
//	unsigned char high_energy_events_LSB_2;
//	unsigned char n_ellipse1_MSB_3;
//	unsigned char n_ellipse1_LSB_3;
//	unsigned char n_ellipse2_MSB_3;
//	unsigned char n_ellipse2_LSB_3;
//	unsigned char non_n_events_MSB_3;
//	unsigned char non_n_events_LSB_3;
//	unsigned char high_energy_events_MSB_3;
//	unsigned char high_energy_events_LSB_3;
//	unsigned char dead_time_MSB;
//	unsigned char dead_time_LSB;
	unsigned char time_MSB;
	unsigned char time_LSB1;
	unsigned char time_LSB2;
	unsigned char time_LSB3;
	unsigned char modu_temp;
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
bool CPSIsWithinEllipse( double energy, double psd, int module_num, int ellipse_num );
int CPSUpdateTallies(double energy, double psd, int pmt_id);

#endif /* SRC_CPSDATAPRODUCT_H_ */
