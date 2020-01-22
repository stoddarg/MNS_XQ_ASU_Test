/*
 * LI2C_Interface.h
 *
 *  Created on: Mar 16, 2017
 *      Author: GStoddard
 */

#ifndef LI2C_INTERFACE_H_
#define LI2C_INTERFACE_H_

#include "xiicps.h"
#include <xtime_l.h>

#define IIC_SCLK_RATE		90000
#define TEST_BUFFER_SIZE	2

/* Declare Variables */
//XIicPs Iic;					//Instance of the IIC device

/* Function Declarations */
void I2C_InitStartTime(void);
int IicPsInit(XIicPs * Iic, u16 DeviceId);
int IicPsMasterSend(XIicPs * Iic, u16 DeviceId, u8 * ptr_Send_Buffer, u8 * ptr_Recv_Buffer, int iI2C_slave_addr);
int IicPsMasterRecieve(XIicPs * Iic, u8 * ptr_Recv_Buffer, int iI2C_slave_addr);

#endif /* LI2C_INTERFACE_H_ */
