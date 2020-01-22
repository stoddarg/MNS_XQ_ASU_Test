/*
 * lunah_defines.h
 *
 *  Created on: Jun 20, 2018
 *      Author: IRDLAB
 */

#ifndef SRC_LUNAH_DEFINES_H_
#define SRC_LUNAH_DEFINES_H_

#include "xparameters.h"

#define MNS_DETECTOR_NUM	0

#define PRODUCE_RAW_DATA	0

#define NS_TO_SAMPLES		4		//conversion factor number of nanoseconds per sample
#define INTEG_TIME_START	200
#define LOG_FILE_BUFF_SIZE	120
#define UART_DEVICEID		XPAR_XUARTPS_0_DEVICE_ID
#define SW_BREAK_GPIO		51
#define IIC_DEVICE_ID_0		XPAR_XIICPS_0_DEVICE_ID	//sensor head
#define IIC_DEVICE_ID_1		XPAR_XIICPS_1_DEVICE_ID	//thermometer/pot on digital board
#define FILENAME_SIZE		50
#define	TEC_PIN				18
#define EVT_EVENT_SIZE		8
#define ROOT_DIR_NAME_SIZE	3
#define DAQ_FOLDER_SIZE		11		//"I1234_R1234"
#define WF_FOLDER_SIZE		8		//"WF_I1234"
#define SIZEOF_FILENAME		13		//filename example: "cps_S0001.bin"
#define TELEMETRY_MAX_SIZE	2038
#define VALID_BUFFER_SIZE	512
#define DATA_BUFFER_SIZE	4096
#define EVENT_BUFFER_SIZE	2048
#define EVT_DATA_BUFF_SIZE	16384
#define SIZEOF_HEADER_TIMES	14
#define TWODH_X_BINS		512		//260
#define	TWODH_Y_BINS		64		//30
#define TWODH_ENERGY_MAX	1200000	//previously used 1,000,000 but recalculated that this was correct using temp. calib. data
#define TWODH_PSD_MAX		2.0
#define RMD_CHECKSUM_SIZE	2
#define SYNC_MARKER			892270675	//0x35 2E F8 53
#define SYNC_MARKER_SIZE	4
#define EVENT_ID_VALUE		111111
#define EVENT_ID_SIZE		4
#define CHECKSUM_SIZE		4
#define CCSDS_HEADER_DATA	7		//without the sync marker, with the reset request byte
#define CCSDS_HEADER_PRIM	10		//with the sync marker, without the reset request byte
#define CCSDS_HEADER_FULL	11		//with the sync marker, with the reset request byte
#define SIZE_1_MIB			1048576	//1 MiB, rather than 1 MB (1e6 bytes)
#define SIZE_10_MIB			10485760	//10 MiB
#define DP_HEADER_SIZE		16384	//we put blank space past the header so we always write on a cluster boundary
#define XB1_SEND_WAIT		0.015	//15ms wait time; this accounts for the latency on the XB-1 side of communications

//PMT ID Values
//These values are the decimal interpretations of a binary, active high signal (ie. 4=0100 -> PMT_ID_3)
#define NO_HIT_PATTERN	0
#define	PMT_ID_0		1
#define	PMT_ID_1		2
#define	PMT_ID_2		4
#define	PMT_ID_3		8

// Command definitions
#define DAQ_CMD			0
#define WF_CMD			1
#define READ_TMP_CMD	2
#define GETSTAT_CMD		3
#define DISABLE_ACT_CMD	4
#define ENABLE_ACT_CMD	5
#define TX_CMD			6
#define DEL_CMD			7
#define DIR_CMD			8
#define TXLOG_CMD		9
#define	CONF_CMD		10
#define TRG_CMD			11
#define ECAL_CMD		12
#define NGATES_CMD		13
#define HV_CMD			14
#define INT_CMD			15
#define BREAK_CMD		16
#define START_CMD		17
#define END_CMD			18
#define INPUT_OVERFLOW	100

//Command SUCCESS/FAILURE values
#define CMD_FAILURE		0	// 0 == FALSE
#define CMD_SUCCESS		1	// non-zero == TRUE
#define CMD_ERROR		2	//

//Mode Byte Values
#define MODE_STANDBY	17	//0x11
#define MODE_LOAD		34	//0x22
#define MODE_TRANSFER	51	//0x33
#define MODE_PRE_DAQ	68	//0x44
#define MODE_DAQ		85	//0x55
#define MODE_DAQ_WF		102	//0x66

//APID Packet Types
#define APID_CMD_SUCC	0
#define APID_CMD_FAIL	1
#define APID_SOH		2
#define APID_DIR		3
#define APID_TEMP		4
#define APID_MNS_CPS	5
#define APID_MNS_WAV	6
#define APID_MNS_EVT	7
#define APID_MNS_2DH	8
#define APID_LOG_FILE	9
#define APID_CONFIG		10

//MNS GROUP FLAGS
#define GF_INTER_PACKET	0
#define GF_FIRST_PACKET	1
#define GF_LAST_PACKET	2
#define GF_UNSEG_PACKET	3

//MNS DATA PRODUCT TYPES //UPDATED 4/10/19
#define DATA_TYPE_CPS	5
#define DATA_TYPE_WAV	6
#define DATA_TYPE_EVT 	7
#define DATA_TYPE_2DH_0	8
#define DATA_TYPE_LOG	9
#define DATA_TYPE_CFG	10
//the above are made to match the APID packet types
//the below are to make it easier to sort 2dh types
#define DATA_TYPE_2DH_1	11
#define DATA_TYPE_2DH_2	12
#define DATA_TYPE_2DH_3 13

//MNS DATA PACKET HEADER SIZES //includes secondary header + data header
#define PKT_HEADER_DIR	18
#define PKT_HEADER_EVT	39
#define PKT_HEADER_WAV	39
#define PKT_HEADER_CPS	39
#define PKT_HEADER_2DH	39
#define PKT_HEADER_LOG	1
#define PKT_HEADER_CFG	1

//MNS DATA PRODUCT DATA BYTE SIZES
#define DATA_BYTES_DIR	2006
#define DATA_BYTES_EVT	1984
#define DATA_BYTES_WAV	1984
#define DATA_BYTES_CPS	1974
#define DATA_BYTES_2DH	1985
#define DATA_BYTES_LOG	1963
#define DATA_BYTES_CFG	187

//MNS DATA PACKET SIZES //Full size - 10 - 1
#define PKT_SIZE_DIR	2027
#define PKT_SIZE_EVT 	2026
#define PKT_SIZE_WAV	2026
#define PKT_SIZE_CPS	2016
#define PKT_SIZE_2DH	2027
#define PKT_SIZE_LOG	2029	//TODO: define this
#define PKT_SIZE_CFG	191

//MNS DATA FILE FOOTER SIZES //The main data products have a footer in the file
#define FILE_FOOT_2DH	20

//DAQ FINAL STATE
#define DAQ_BREAK		0
#define DAQ_TIME_OUT	1
#define DAQ_END			2

//Mini-NS DMA Addresses to read from
#define DRAM_BASE		0xA000000u
#define DRAM_CEILING	0xA004000u

#endif /* SRC_LUNAH_DEFINES_H_ */
