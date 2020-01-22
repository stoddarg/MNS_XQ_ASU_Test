/*
 * lunah_utils.h
 *
 *  Created on: Jun 22, 2018
 *      Author: IRDLAB
 */

#ifndef SRC_LUNAH_UTILS_H_
#define SRC_LUNAH_UTILS_H_

#include <stdlib.h>
#include <xtime_l.h>
#include <xuartps.h>
#include "ff.h"
#include "ReadCommandType.h"	//gives access to last command strings
#include "lunah_defines.h"
#include "LI2C_Interface.h"		//talk to I2C devices (temperature sensors)

#define IIC_SLAVE_ADDR2		0x4B	//Temp sensor on digital board
#define IIC_SLAVE_ADDR3		0x48	//Temp sensor on the analog board
#define IIC_SLAVE_ADDR5		0x4A	//Extra Temp Sensor Board, mounted near the modules

#define TAB_CHAR_CODE		9
#define NEWLINE_CHAR_CODE	10
#define SOH_PACKET_LENGTH	56
#define TEMP_PACKET_LENGTH	19

// prototypes
void InitStartTime( void );
XTime GetLocalTime( void );
XTime GetTempTime(void);
void ResetNeutronCounts( void );
int IncNeutronTotal(int pmt_id, int increment);
int GetDigiTemp( void );
int GetAnlgTemp( void );
int GetModuTemp( void );
int InitTempSensors( XIicPs *Iic );
void SetModeByte( unsigned char mode );
void SetIDNumber( int id_number );
void SetRunNumber( int run_number );
int GetIDNumber( void );
int GetRunNumber( void );
void CheckForSOH(XIicPs * Iic, XUartPs Uart_PS);
int report_SOH(XIicPs * Iic, XTime local_time, XUartPs Uart_PS, int packet_type);
void PutCCSDSHeader(unsigned char * SOH_buff, int packet_type, int group_flags, int sequence_count, int length);
int reportSuccess(XUartPs Uart_PS, int report_filename);
int reportFailure(XUartPs Uart_PS);
void CalculateChecksums(unsigned char * packet_array);
int CalculateDataFileChecksum(XUartPs Uart_PS, char * RecvBuffer, int file_type, int id_num, int run_num, int set_num);
int DeleteFile( XUartPs Uart_PS, char * RecvBuffer, int sd_card_number, int file_type, int id_num, int run_num, int set_num );
int TransferSDFile( XUartPs Uart_PS, char * RecvBuffer, int file_type, int id_num, int run_num,  int set_num );
int SendPacket( XUartPs Uart_PS, unsigned char *packet_buffer, int bytes_to_send );

#endif /* SRC_LUNAH_UTILS_H_ */
