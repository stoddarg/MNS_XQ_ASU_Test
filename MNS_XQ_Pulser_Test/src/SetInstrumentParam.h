/*
 * SetInstrumentParam.h
 *
 *  Created on: Jun 20, 2018
 *      Author: IRDLAB
 */

/*
 * This file handles all interaction with the system parameters and the configuration file.
 */

#ifndef SRC_SETINSTRUMENTPARAM_H_
#define SRC_SETINSTRUMENTPARAM_H_

#include <stdio.h>
#include <xparameters.h>
#include "ff.h"
#include "lunah_defines.h"
#include "lunah_utils.h"
#include "LI2C_Interface.h"
#include "RecordFiles.h"

/*
 * Mini-NS Configuration Parameter Structure
 * This is the collection of all of the Mini-NS system parameters.
 * Unless the user explicitly changes these, then the default will be filled in
 *  when the system boots. The configuration file is where the defaults are stored.
 * Each time that a parameter is changed by the user, that value is written to the
 *  configuration file, as well as to the current struct holding the parameters.
 * In this fashion, we are able to hold onto any changes that are made. This should
 *  reduce the amount of interaction necessary.
 * Added the total files/folders values to the config file so that we can
 *  better keep track of the state of the SD card.
 *
 * See the Mini-NS ICD for a breakdown of these parameters and how to change them.
 * Current ICD version: 10.4.0
 *
 * Size = 300 bytes (10/23/19)
 * No padding bytes
 * Outline:
 * 	4 x 2
 * 	4 x 5
 * 	4 x 4
 * 	8 x 8 x 4
 * 	Doubles are 8 bytes.
 */
typedef struct {
	float ECalSlope;
	float ECalIntercept;
	int TriggerThreshold;
	int IntegrationBaseline;
	int IntegrationShort;
	int IntegrationLong;
	int IntegrationFull;
	int HighVoltageValue[4];
	double SF_E[8];
	double SF_PSD[8];
	double Off_E[8];
	double Off_PSD[8];
	int TotalFiles;
	int TotalFolders;
	unsigned int MostRecentRealTime;
} CONFIG_STRUCT_TYPE;

/*
* This is a struct which includes the information from the config buffer above
* plus a few extra pieces that need to go into headers.
*
* NOTE: The file type APID is the INTERNAL number for the file type
* 		Thus, for file type CPS, we put a 5 as that char, which corresponds
* 		 to APID_MNS_CPS and DATA_TYPE_CPS
*
* Size = 320 bytes (10/23/19)
* 4 padding bytes (10/23/19)
* Outline:
* 	config buff = 300 bytes
* 	padding bytes = 4 bytes
* 	4 x 3 = 12 bytes
* 	1 x 4 = 4 bytes
*
*/
typedef struct{
	CONFIG_STRUCT_TYPE configBuff;	//43 4-byte values
	unsigned int IDNum;
	unsigned int RunNum;
	unsigned int SetNum;
	unsigned char FileTypeAPID;
	unsigned char TempCorrectionSetNum;
	unsigned char EventID1;
	unsigned char EventID2;
}DATA_FILE_HEADER_TYPE;

/*
 * Secondary file header for EVT, CPS data products
 *
 * Size = 16 bytes (10/23/19)
 */
typedef struct{
	unsigned int RealTime;
	unsigned char EventID1;
	unsigned char EventID2;
	unsigned char EventID3;
	unsigned char EventID4;
	unsigned int FirstEventTime;
	unsigned char EventID5;
	unsigned char EventID6;
	unsigned char EventID7;
	unsigned char EventID8;
}DATA_FILE_SECONDARY_HEADER_TYPE;	//currently 24 bytes, see p47

/*
 * Footer for EVT, CPS data products
 *
 * Size = 20 bytes (10/23/19)
 */
typedef struct{
	unsigned char eventID1;
	unsigned char eventID2;
	unsigned char eventID3;
	unsigned char eventID4;
	unsigned int RealTime;
	unsigned char eventID5;
	unsigned char eventID6;
	unsigned char eventID7;
	unsigned char eventID8;
	int digiTemp;
	unsigned char eventID9;
	unsigned char eventID10;
	unsigned char eventID11;
	unsigned char eventID12;
}DATA_FILE_FOOTER_TYPE;

// prototypes
void CreateDefaultConfig( void );
CONFIG_STRUCT_TYPE * GetConfigBuffer( void );
int GetBaselineInt( void );
int GetShortInt( void );
int GetLongInt( void );
int GetFullInt( void );
int InitConfig( void );
int SaveConfig( void );
int SetTriggerThreshold(int iTrigThreshold);
int SetNeutronCutGates(int moduleID, int ellipseNum, float ECut1, float ECut2, float PCut1, float PCut2);
int SetHighVoltage(XIicPs * Iic, unsigned char PmtId, int value);
int SetIntegrationTime(int Baseline, int Short, int Long, int Full);
int SetEnergyCalParam(float Slope, float Intercept);
void SetSDTotalFiles( int total_files );
int GetSDTotalFiles( void );
void SetSDTotalFolders( int total_folders );
int GetSDTotalFolders( void );
int RecordSDState( void );
int SetRealTime( unsigned int real_time );
unsigned int GetRealTime( void );
int ApplyDAQConfig( XIicPs * Iic );

#endif /* SRC_SETINSTRUMENTPARAM_H_ */
