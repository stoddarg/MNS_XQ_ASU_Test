/*
 * lunah_utils.c
 *
 *  Created on: Jun 22, 2018
 *      Author: IRDLAB
 */

#include "lunah_utils.h"

static XTime t_elapsed;			//was LocalTime
static XTime t_next_interval;	//added to take the place of t_elapsed
static XTime TempTime;			//unchanged
static XTime t_start;			//was LocalTimeStart
static XTime t_current;			//was LocalTimeCurrent
static XTime wait_start;		//timer for sending packets
static XTime wait_timer;		//timer for sending packets

static int analog_board_temp;
static int digital_board_temp;
static int modu_board_temp;
static int iNeutronTotal;		//total neutron counts across all PMTs
static int iNeutronTotal_pmt0;
static int iNeutronTotal_pmt1;
static int iNeutronTotal_pmt2;
static int iNeutronTotal_pmt3;
static int check_temp_sensor;
static unsigned char mode_byte;
static int soh_id_number;
static int soh_run_number;

/*
 * Initialize t_start and t_elapsed at startup
 */
void InitStartTime(void)
{
	XTime_GetTime(&t_start);	//get the time
	t_next_interval = 0;
}
/*
 * Report the elapsed time for the run
 */
XTime GetLocalTime(void)
{
	XTime_GetTime(&t_current);
	t_elapsed = (t_current - t_start)/COUNTS_PER_SECOND;
	return t_elapsed;
}

XTime GetTempTime(void)
{
	XTime_GetTime(&t_current);
	TempTime = (t_current - t_start)/COUNTS_PER_SECOND;
	return TempTime;
}

/*
 *  Stub file to return neutron total.
 */
int GetNeutronTotal(void)
{
	return iNeutronTotal;
}

int PutNeutronTotal(int total)
{
	iNeutronTotal = total;
	return iNeutronTotal;
}

/*
 * Take in both the number of neutrons to increment by, as well as which PMT to assign the
 *  counts to.
 *
 *  @param	(int)PMT to assign the counts to (1, 2, 4, 8)
 *  @param	(int)number of counts to increment
 */
int IncNeutronTotal(int pmt_id, int increment)
{
	switch(pmt_id)
	{
	case PMT_ID_0:
		iNeutronTotal_pmt0 += increment;
		break;
	case PMT_ID_1:
		iNeutronTotal_pmt1 += increment;
		break;
	case PMT_ID_2:
		iNeutronTotal_pmt2 += increment;
		break;
	case PMT_ID_3:
		iNeutronTotal_pmt3 += increment;
		break;
	}

	//TODO: handle a bad PMT ID, they will certainly get passed in,
	// but we don't want to include them in the individual module totals.
	//I do not think that we need anything special to include it. Just increment the total, but not the individual hits.
	iNeutronTotal += increment;

	return iNeutronTotal;
}
/*
 * Getter functions to grab the temperature which was most recently read by the system
 *
 * @param:	None
 *
 * Return:	(INT) gives the temperature from the chosen module
 */
int GetDigiTemp( void )
{
	return digital_board_temp;
}

int GetAnlgTemp( void )
{
	return analog_board_temp;
}

int GetModuTemp( void )
{
	return modu_board_temp;
}

/*
 * Setter function for the mode byte
 *
 * @param	the mode that the Mini-NS is in
 */
void SetModeByte( unsigned char mode )
{
	mode_byte = mode;
}

/*
 * Setter function for the SOH ID number
 *
 * @param	The ID number for the pre-DAQ, DAQ, or WF run
 */
void SetIDNumber( int id_number )
{
	soh_id_number = id_number;
}

/*
 * Setter function for the SOH Run number
 *
 * @param	The Run number for the pre-DAQ or DAQ run
 */
void SetRunNumber( int run_number )
{
	soh_run_number = run_number;
}

/*
 * Setter functions for the SOH ID number and Run number
 */
int GetIDNumber( void )
{
	return soh_id_number;
}

int GetRunNumber( void )
{
	return soh_run_number;
}

/*
 *  CheckForSOH
 *      Check if time to send SOH and if it is send it.
 *
 *	Using a new algorithm for this starting 6-21-19
 *		Trying to not have the time intervals lag as the run progresses.
 *		This method keeps the intervals at 1 second past the start time consistently.
 *		This method also handles not checking this function for more than 1 second.
 */
void CheckForSOH(XIicPs * Iic, XUartPs Uart_PS)
{
	XTime_GetTime(&t_current);
	t_elapsed = (t_current - t_start)/COUNTS_PER_SECOND;
	if(t_elapsed >= t_next_interval)
	{
		while(t_elapsed >= t_next_interval)
		{
			t_next_interval++;
		}
		report_SOH(Iic, t_elapsed, Uart_PS, GETSTAT_CMD);	//use GETSTAT_CMD for heartbeat
	}
	return;
}



//////////////////////////// Report SOH Function ////////////////////////////////
//This function takes in the number of neutrons currently counted and the local time
// and pushes the SOH data product to the bus over the UART
int report_SOH(XIicPs * Iic, XTime local_time, XUartPs Uart_PS, int packet_type)
{
	//Variables
	unsigned char report_buff[100] = "";
	unsigned char i2c_Send_Buffer[2] = {};
	unsigned char i2c_Recv_Buffer[2] = {};
	int a = 0;
	int b = 0;
	int status = 0;
	int bytes_sent = 0;
	unsigned int local_time_holder = 0;

	i2c_Send_Buffer[0] = 0x0;
	i2c_Send_Buffer[1] = 0x0;
	int IIC_SLAVE_ADDR2 = 0x4B;	//Temp sensor on digital board
	int IIC_SLAVE_ADDR3 = 0x48;	//Temp sensor on the analog board
	int IIC_SLAVE_ADDR5 = 0x4A;	//Extra Temp Sensor Board, mounted near the modules

	switch(check_temp_sensor){
	case 0:	//analog board
		XTime_GetTime(&t_current);
		if(((t_current - t_start)/COUNTS_PER_SECOND) >= (TempTime + 60))
		{
			TempTime = (t_current - t_start)/COUNTS_PER_SECOND; //temp time is reset
			check_temp_sensor++;

			IicPsMasterSend(Iic, IIC_DEVICE_ID_0, i2c_Send_Buffer, i2c_Recv_Buffer, &IIC_SLAVE_ADDR3);
			IicPsMasterRecieve(Iic, i2c_Recv_Buffer, &IIC_SLAVE_ADDR3);
			a = i2c_Recv_Buffer[0]<< 5;
			b = a | i2c_Recv_Buffer[1] >> 3;
			if(i2c_Recv_Buffer[0] >= 128)
			{
				b = (b - 8192) / 16;
			}
			else
			{
				b = b / 16;
			}
			analog_board_temp = b;
		}
		break;
	case 1:	//digital board
		XTime_GetTime(&t_current);
		if(((t_current - t_start)/COUNTS_PER_SECOND) >= (TempTime + 60))
		{
			TempTime = (t_current - t_start)/COUNTS_PER_SECOND; //temp time is reset
			check_temp_sensor++;

			IicPsMasterSend(Iic, IIC_DEVICE_ID_1, i2c_Send_Buffer, i2c_Recv_Buffer, &IIC_SLAVE_ADDR2);
			IicPsMasterRecieve(Iic, i2c_Recv_Buffer, &IIC_SLAVE_ADDR2);
			a = i2c_Recv_Buffer[0]<< 5;
			b = a | i2c_Recv_Buffer[1] >> 3;
			if(i2c_Recv_Buffer[0] >= 128)
			{
				b = (b - 8192) / 16;
			}
			else
			{
				b = b / 16;
			}
			digital_board_temp = b;
		}
		break;
	case 2:	//module sensor
		XTime_GetTime(&t_current);
		if(((t_current - t_start)/COUNTS_PER_SECOND) >= (TempTime + 60))
		{
			TempTime = (t_current - t_start)/COUNTS_PER_SECOND; //temp time is reset
			check_temp_sensor = 0;

			status = IicPsMasterSend(Iic, IIC_DEVICE_ID_0, i2c_Send_Buffer, i2c_Recv_Buffer, &IIC_SLAVE_ADDR5);
			status = IicPsMasterRecieve(Iic, i2c_Recv_Buffer, &IIC_SLAVE_ADDR5);
			a = i2c_Recv_Buffer[0]<< 5;
			b = a | i2c_Recv_Buffer[1] >> 3;
			if(i2c_Recv_Buffer[0] >= 128)
			{
				b = (b - 8192) / 16;
			}
			else
			{
				b = b / 16;
			}
			modu_board_temp = b;
		}
		break;
	default:
		status = CMD_FAILURE;
		break;
	}

	report_buff[11] = (unsigned char)(analog_board_temp >> 24);
	report_buff[12] = (unsigned char)(analog_board_temp >> 16);
	report_buff[13] = (unsigned char)(analog_board_temp >> 8);
	report_buff[14] = (unsigned char)(analog_board_temp);
	report_buff[15] = TAB_CHAR_CODE;
	report_buff[16] = (unsigned char)(digital_board_temp >> 24);
	report_buff[17] = (unsigned char)(digital_board_temp >> 16);
	report_buff[18] = (unsigned char)(digital_board_temp >> 8);
	report_buff[19] = (unsigned char)(digital_board_temp);
	report_buff[20] = TAB_CHAR_CODE;
	report_buff[21] = (unsigned char)(modu_board_temp >> 24);
	report_buff[22] = (unsigned char)(modu_board_temp >> 16);
	report_buff[23] = (unsigned char)(modu_board_temp >> 8);
	report_buff[24] = (unsigned char)(modu_board_temp);
	report_buff[25] = NEWLINE_CHAR_CODE;

	switch(packet_type)
	{
	case READ_TMP_CMD:
		PutCCSDSHeader(report_buff, APID_TEMP, GF_UNSEG_PACKET, 1, TEMP_PACKET_LENGTH);
		CalculateChecksums(report_buff);

		bytes_sent = XUartPs_Send(&Uart_PS, (u8 *)report_buff, (TEMP_PACKET_LENGTH + CCSDS_HEADER_FULL));
		if(bytes_sent == (TEMP_PACKET_LENGTH + CCSDS_HEADER_FULL))
			status = CMD_SUCCESS;
		else
			status = CMD_FAILURE;
		break;
	case GETSTAT_CMD:
		report_buff[26] = (unsigned char)(iNeutronTotal_pmt0 >> 24);
		report_buff[27] = (unsigned char)(iNeutronTotal_pmt0 >> 16);
		report_buff[28] = (unsigned char)(iNeutronTotal_pmt0 >> 8);
		report_buff[29] = (unsigned char)(iNeutronTotal_pmt0);
		report_buff[30] = TAB_CHAR_CODE;
		report_buff[31] = (unsigned char)(iNeutronTotal_pmt1 >> 24);
		report_buff[32] = (unsigned char)(iNeutronTotal_pmt1 >> 16);
		report_buff[33] = (unsigned char)(iNeutronTotal_pmt1 >> 8);
		report_buff[34] = (unsigned char)(iNeutronTotal_pmt1);
		report_buff[35] = TAB_CHAR_CODE;
		report_buff[36] = (unsigned char)(iNeutronTotal_pmt2 >> 24);
		report_buff[37] = (unsigned char)(iNeutronTotal_pmt2 >> 16);
		report_buff[38] = (unsigned char)(iNeutronTotal_pmt2 >> 8);
		report_buff[39] = (unsigned char)(iNeutronTotal_pmt2);
		report_buff[40] = TAB_CHAR_CODE;
		report_buff[41] = (unsigned char)(iNeutronTotal_pmt3 >> 24);
		report_buff[42] = (unsigned char)(iNeutronTotal_pmt3 >> 16);
		report_buff[43] = (unsigned char)(iNeutronTotal_pmt3 >> 8);
		report_buff[44] = (unsigned char)(iNeutronTotal_pmt3);
		report_buff[45] = TAB_CHAR_CODE;
		local_time_holder = (unsigned int)local_time;
		report_buff[46] = (unsigned char)(local_time_holder >> 24);
		report_buff[47] = (unsigned char)(local_time_holder >> 16);
		report_buff[48] = (unsigned char)(local_time_holder >> 8);
		report_buff[49] = (unsigned char)(local_time_holder);
		report_buff[50] = TAB_CHAR_CODE;
		report_buff[51] = mode_byte;
		report_buff[52] = TAB_CHAR_CODE;
		report_buff[53] = (unsigned char)(soh_id_number >> 24);
		report_buff[54] = (unsigned char)(soh_id_number >> 16);
		report_buff[55] = (unsigned char)(soh_id_number >> 8);
		report_buff[56] = (unsigned char)(soh_id_number);
		report_buff[57] = TAB_CHAR_CODE;
		report_buff[58] = (unsigned char)(soh_run_number >> 24);
		report_buff[59] = (unsigned char)(soh_run_number >> 16);
		report_buff[60] = (unsigned char)(soh_run_number >> 8);
		report_buff[61] = (unsigned char)(soh_run_number);
		report_buff[62] = NEWLINE_CHAR_CODE;

		PutCCSDSHeader(report_buff, APID_SOH, GF_UNSEG_PACKET, 1, SOH_PACKET_LENGTH);
		CalculateChecksums(report_buff);

		bytes_sent = XUartPs_Send(&Uart_PS, (u8 *)report_buff, (SOH_PACKET_LENGTH + CCSDS_HEADER_FULL));
		if(bytes_sent == (SOH_PACKET_LENGTH + CCSDS_HEADER_FULL))
			status = CMD_SUCCESS;
		else
			status = CMD_FAILURE;
		break;
	default:
		status = CMD_FAILURE;
		break;
	}

	return status;
}

/*
 * Put the appropriate CCSDS header values into the output packet.
 *
 * @param SOH_buff			Pointer to the packet buffer
 * @param packet_type		The APID for the packet being constructed
 * @param group_flags		Indicates whether the packet is unsegmented, first, intermediate, or last
 * @param sequence_count	What the sequence count number is for the packet being constructed
 * @param length			The length of the packet is equal to the number of bytes in the secondary CCSDS header
 * 							plus the payload data bytes plus the checksums minus one.
 * 							Len = 1 + N + 4 - 1
 *
 */
void PutCCSDSHeader(unsigned char * SOH_buff, int packet_type, int group_flags, int sequence_count, int length)
{
	//get the values for the CCSDS header
	SOH_buff[0] = 0x35;
	SOH_buff[1] = 0x2E;
	SOH_buff[2] = 0xF8;
	SOH_buff[3] = 0x53;
	if(MNS_DETECTOR_NUM == 0)
		SOH_buff[4] = 0x0A; //identify detector 0 or 1
	else
		SOH_buff[4] = 0x0B;
	//use the input to determine what APID to fill here
	switch(packet_type)
	{
	case APID_CMD_SUCC:
		SOH_buff[5] = 0x00;	//APID for Command Success
		break;
	case APID_CMD_FAIL:
		SOH_buff[5] = 0x11;	//APID for Command Failure
		break;
	case APID_SOH:
		SOH_buff[5] = 0x22;	//APID for SOH
		break;
	case APID_DIR:
		SOH_buff[5] = 0x33;	//APID for LS Files
		break;
	case APID_TEMP:
		SOH_buff[5] = 0x44;	//APID for Temperature
		break;
	case APID_MNS_CPS:
		SOH_buff[5] = 0x55;	//APID for Counts per second
		break;
	case APID_MNS_WAV:
		SOH_buff[5] = 0x66;	//APID for Waveforms
		break;
	case APID_MNS_EVT:
		SOH_buff[5] = 0x77;	//APID for Event-by-Event
		break;
	case APID_MNS_2DH:
		SOH_buff[5] = 0x88;	//APID for 2D Histogram
		break;
	case APID_LOG_FILE:
		SOH_buff[5] = 0x99;	//APID for Log
		break;
	case APID_CONFIG:
		SOH_buff[5] = 0xAA;	//APID for Configuration
		break;
	case DATA_TYPE_2DH_1:
		SOH_buff[5] = 0x88;	//APID for 2D Histogram
		break;
	case DATA_TYPE_2DH_2:
		SOH_buff[5] = 0x88;	//APID for 2D Histogram
		break;
	case DATA_TYPE_2DH_3:
		SOH_buff[5] = 0x88;	//APID for 2D Histogram
		break;
	default:
		SOH_buff[5] = 0x22; //default to SOH just in case?
		break;
	}

	SOH_buff[6] = (unsigned char)(sequence_count >> 8);
	SOH_buff[6] &= 0x3F;
	SOH_buff[6] |= (unsigned char)(group_flags << 6);
	SOH_buff[7] = (unsigned char)(sequence_count);
	SOH_buff[8] = (length & 0xFF00) >> 8;
	SOH_buff[9] = length & 0xFF;

	SOH_buff[10] = 0x00;	//TODO: actually set the conditions to report a reset request

	return;
}

/**
 * Report the SUCCESS packet for a function which was received and passed
 *
 * @param Uart_PS	Pointer to the instance of the UART which will
 * 					transmit the packet to the spacecraft.
 * @param daq_filename	A switch to turn on if we want to report the
 * 						filename that a DAQ run will use.
 * 						0: no filename
 * 						1: report filename
 * 						else: no filename
 *
 * @return	CMD_SUCCESS or CMD_FAILURE depending on if we sent out
 * 			the correct number of bytes with the packet.
 *
 * NB: Only DAQ or WF should enable the daq_filename switch, otherwise a
 * 		junk filename will be received which will at least not be relevant,
 * 		but at worst could cause a problem.
 *
 */
int reportSuccess(XUartPs Uart_PS, int report_filename)
{
	int status = 0;
	int bytes_sent = 0;
	int packet_size = 0;	//Don't record the size of the CCSDS header with this variable
	int i_sprintf_ret = 0;
	unsigned char cmdSuccess[100] = "";

	//fill the data bytes
	switch(report_filename)
	{
	case 1:
		//Enabled the switch to report the filename
		//should we check to see if the last command was DAQ/WF? No for now
		i_sprintf_ret = snprintf((char *)(&cmdSuccess[11]), 100, "%s\n", GetLastCommand());
		if(i_sprintf_ret == GetLastCommandSize())
		{
			packet_size += i_sprintf_ret;
			//now we want to add in the new filename that is going to be used
			//TODO: report the folder instead of the filename (the run number is in the folder name now)
			i_sprintf_ret = snprintf((char *)(&cmdSuccess[11 + i_sprintf_ret]), 100, "%s", GetFolderName());
			if(i_sprintf_ret == GetFolderNameSize())
			{
				packet_size += i_sprintf_ret;
				status = CMD_SUCCESS;
			}
			else
				status = CMD_FAILURE;
		}
		else
			status = CMD_FAILURE;
		break;
	default:
		//Case 0 is the default so that the normal success happens
		// even if we get some weird value coming through
		//no switch to report the filename, this is a normal SUCCESS PACKET
		i_sprintf_ret = snprintf((char *)(&cmdSuccess[11]), 100, "%s\n", GetLastCommand());
		if(i_sprintf_ret == GetLastCommandSize())
		{
			packet_size += i_sprintf_ret;
			status = CMD_SUCCESS;
		}
		else
			status = CMD_FAILURE;
		break;
	}

	//I should look at using a regular char buffer rather than using calloc() and free() //changed 3/14/19
	//get last command gets the size of the string minus the newline
	//we want to add 1 (secondary header) and add 4 (checksums) minus 1
	PutCCSDSHeader(cmdSuccess, APID_CMD_SUCC, GF_UNSEG_PACKET, 1, packet_size + CHECKSUM_SIZE);
	CalculateChecksums(cmdSuccess);

	bytes_sent = XUartPs_Send(&Uart_PS, (u8 *)cmdSuccess, (CCSDS_HEADER_FULL + packet_size + CHECKSUM_SIZE));
	if(bytes_sent == (CCSDS_HEADER_FULL + packet_size + CHECKSUM_SIZE))
		status = CMD_SUCCESS;
	else
		status = CMD_FAILURE;

	return status;
}

/**
 * Report the FAILURE packet for a function which was received, but failed
 *
 * @param Uart_PS	Pointer to the instance of the UART which will
 * 					transmit the packet to the spacecraft.
 *
 * @return	CMD_SUCCESS or CMD_FAILURE depending on if we sent out
 * 			the correct number of bytes with the packet.
 *
 * TODO: Model this function after the report success function which has different modes:
 * 			For example, the report success function can either send back a filename or not
 * 			depending on what kind of a success we are acknowledging. This will be useful
 * 			for the user to know what kind of failure is happening.
 * 		 Use a second parameter to return a specific error code to the user as the payload
 * 		 	for this packet. Ie. if the payload bytes for this packet are a 13, then that
 * 		 	could mean that an f_write function failed. This will be a good way to communicate
 * 		 	when something specific goes wrong in a function, too as we can provide sub-codes
 * 		 	like 13.1 which could say this was the first write or 13.5 which is the fifth write
 * 		 	within the function.
 */
int reportFailure(XUartPs Uart_PS)
{
	int status = 0;
	int bytes_sent = 0;
	int i_sprintf_ret = 0;
	unsigned char cmdFailure[100] = "";

	i_sprintf_ret = snprintf((char *)(&cmdFailure[11]), 100, "%s\n", GetLastCommand());
	if(i_sprintf_ret == GetLastCommandSize())
		status = CMD_SUCCESS;
	else
		status = CMD_FAILURE;

	PutCCSDSHeader(cmdFailure, APID_CMD_FAIL, GF_UNSEG_PACKET, 1, GetLastCommandSize() + CHECKSUM_SIZE);
	CalculateChecksums(cmdFailure);

	bytes_sent = XUartPs_Send(&Uart_PS, (u8 *)cmdFailure, (CCSDS_HEADER_FULL + i_sprintf_ret + CHECKSUM_SIZE));
	if(bytes_sent == (CCSDS_HEADER_FULL + i_sprintf_ret + CHECKSUM_SIZE))
		status = CMD_SUCCESS;
	else
		status = CMD_FAILURE;

	return status;
}

/* Function to calculate all four checksums for CCSDS packets
 * This function calculates the Simple, Fletcher, and BCT checksums
 *  by looping over the bytes within the packet after the sync marker.
 *
 *  @param	packet_array	This is a pointer to the CCSDS packet which
 *    							needs to have its checksums calculated.
 *	@param	length			The packet length
 *
 *	@return	(int) returns the value assigned when the command was scanned
 */
void CalculateChecksums(unsigned char * packet_array)
{
	//this function will calculate the simple, Fletcher, and CCSDS checksums for any packet going out
	int packet_size = 0;
	int total_packet_size = 0;
	int iterator = 0;
	int rmd_checksum_simple = 0;
	int rmd_checksum_Fletch = 0;
	unsigned short bct_checksum = 0;

	packet_size = (packet_array[8] << 8) + packet_array[9];	//from the packet, includes payload data plus checksums
	total_packet_size = packet_size + CCSDS_HEADER_FULL;	//includes both primary and secondary CCSDS headers

	//create the RMD checksums
	while(iterator <= (packet_size - CHECKSUM_SIZE))
	{
		rmd_checksum_simple = (rmd_checksum_simple + packet_array[CCSDS_HEADER_PRIM + iterator]) % 255;
		rmd_checksum_Fletch = (rmd_checksum_Fletch + rmd_checksum_simple) % 255;
		iterator++;
	}

	packet_array[total_packet_size - CHECKSUM_SIZE] = rmd_checksum_simple;
	packet_array[total_packet_size - CHECKSUM_SIZE + 1] = rmd_checksum_Fletch;

	//calculate the BCT checksum
	iterator = 0;
	while(iterator < (packet_size - RMD_CHECKSUM_SIZE + CCSDS_HEADER_DATA))
	{
		bct_checksum += packet_array[SYNC_MARKER_SIZE + iterator];
		iterator++;
	}

	packet_array[total_packet_size - CHECKSUM_SIZE + 2] = bct_checksum >> 8;
	packet_array[total_packet_size - CHECKSUM_SIZE + 3] = bct_checksum;

    return;
}

/*
 *  Function to calculate a checksum for the data product files we generate on the Mini-NS.
 *  This function calculates a Fletcher checksum for the file requested by looping over all the bytes that will be
 *  	sent in a file transfer. All data product files (WF, EVT, CPS, 2DH) have some header bytes and footer bytes
 *  	within the data product files which will not be transferred, so those bytes should not be used when calculating
 *  	the checksum.
 *  Alternatively, a cumulative checksum could be provided? This could be something like we keep calculate a checksum
 *  	for the data bytes we send out in a packet, then when we read more data bytes to send, we re-calculate
 *
 *
 *  The file is found via the file type, id, run, and set numbers. Using this information, we create the file string
 *  	and then attempt to open the file.
 *  Alternatively, this function could be called by passing in an open FIL pointer.
 *
 *  @param	packet_array	This is a pointer to the CCSDS packet which
 *    							needs to have its checksums calculated.
 *	@param	length			The packet length
 *
 *	@return	(int) returns the value assigned when the command was scanned
 */
int CalculateDataFileChecksum(XUartPs Uart_PS, char * RecvBuffer, int file_type, int id_num, int run_num, int set_num)
{
	int status = 0;

	return status;
}

/*
 * Delete a file on either SD card.
 *
 *  @param	(XUartPs) instance of the UART
 *  @param	(char *) pointer to the receive buffer
 *  @param	(int) the SD card number (0/1)
 *  @param	(int) the file type that is going to be deleted
 *  @param	(int) the ID number of the file to be deleted
 *  @param	(int) the Run number of the file to be deleted
 *  @param	(int) the Set number of the file to be deleted
 *
 *  @return	(int) status of the delete function
 *  			0 - success
 *  			1 - trouble constructing the file name
 *  			2 - file type is not recognized
 *  			3 - folder name doesn't exist
 *  			4 - denied access when trying to delete file
 *  			5 - f_stat failed
 *  			6 - f_unlink failed
 */
int DeleteFile( XUartPs Uart_PS, char * RecvBuffer, int sd_card_number, int file_type, int id_num, int run_num, int set_num )
{
	int status = 0;
	unsigned int bytes_written = 0;
	char *ptr_file_TX_filename = NULL;
	char file_TX_folder[100] = "";
	char file_TX_filename[100] = "";
	char file_TX_path[100] = "";
	char WF_FILENAME[] = "wf01.bin";
	char log_file[] = "MNSCMDLOG.txt";
	char config_file[] = "MNSCONF.bin";
	FILINFO fno;			//file info structure
	//Initialize the FILINFO struct with something //If using LFN, then we need to init these values, otherwise we don't
	TCHAR LFName[256];
	fno.lfname = LFName;
	fno.lfsize = sizeof(LFName);
	FRESULT f_res = FR_OK;	//SD card status variable type

	//put the file name back together
	//find the folder/file that was requested
	if(file_type == DATA_TYPE_LOG)
	{
		//just on the root directory
		bytes_written = snprintf(file_TX_folder, 100, "0:");
		if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE)
			status = 1;
		ptr_file_TX_filename = log_file;
	}
	else if(file_type == DATA_TYPE_CFG)
	{
		//just on the root directory
		bytes_written = snprintf(file_TX_folder, 100, "0:");
		if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE)
			status = 1;
		ptr_file_TX_filename = config_file;
	}
	else if(file_type == DATA_TYPE_WAV)
	{
		//construct the folder
		bytes_written = snprintf(file_TX_folder, 100,  "0:/WF_I%d", id_num);
		if(bytes_written == 0)
			status = 1;
		//construct the file name
		ptr_file_TX_filename = WF_FILENAME;
	}
	else
	{
		//construct the folder
		bytes_written = snprintf(file_TX_folder, 100, "0:/I%04d_R%04d", id_num, run_num);
		if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE + DAQ_FOLDER_SIZE)
			status = 1;
		//construct the file name
		if(file_type == DATA_TYPE_EVT)
		{
			bytes_written = snprintf(file_TX_filename, 100, "evt_S%04d.bin", set_num);
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_CPS)
		{
			bytes_written = snprintf(file_TX_filename, 100, "cps.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_0)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d0.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_1)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d1.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_2)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d2.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_3)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d3.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else
		{
			//the file type was not recognized
			//we should break out and not go further
			status = 2;	//indicate that the file type is not recognized
		}

		ptr_file_TX_filename = file_TX_filename;
	}

	//write the total file path
	bytes_written = snprintf(file_TX_path, 100, "%s/%s", file_TX_folder, ptr_file_TX_filename);
	if(bytes_written == 0)
		status = 1;

	//check to see if the file exists
	//check that the folder/file we just wrote exists in the file system
	//check first so that we don't just open a blank new file; there are no protections for that
	f_res = f_stat(file_TX_path, &fno);
	if(f_res == FR_NO_FILE)
	{
		//couldn't find the folder
		status = 3;	//folder DNE
	}
	else if(f_res == FR_DENIED)
	{
		//the file/sub-directory must not be read-only
		//the sub-directory must be empty and must not be the current directory
		status = 4;
	}
	else if(f_res == FR_OK)
	{
		//if it does, call f_unlink to delete the file
		f_res = f_unlink(file_TX_path);
		if(f_res == FR_OK)
			status = 0;	//all things are good
		else
			status = 6;//TODO: error check the delete function
	}
	else
		status = 5;

	return status;
}

/*
 * Transfers any one file that is on the SD card. Will return command FAILURE if the file does not exist.
 *
 * @param	(XUartPS)The instance of the UART so we can push packets to the bus
 * @param	(char *)pointer to the receive buffer to check for a BREAK
 * @param	(int)file_type	The macro for the type of file to TX back, see lunah_defines.h for the codes
 * 							 There are 9 file types:
 * 							 DATA_TYPE_EVT, DATA_TYPE_CPS, DATA_TYPE_WAV,
 * 							 DATA_TYPE_2DH_0, DATA_TYPE_2DH_1, DATA_TYPE_2DH_2, DATA_TYPE_2DH_3,
 * 							 DATA_TYPE_LOG, DATA_TYPE_CFG
 * @param 	(int)id_num 	The ID number for the folder the user wants to access
 * @param	(int)run_num	The Run number for the folder the user wants to access *
 * @param	(int)set_num_low	The set number to TX, if multiple files are requested by the user, the
 * 								 calling function will call this function multiple times with a different
 * 								 set number each time.
 *
 * @return	(int) returns the status of the transfer, 0 = good, 1 = file DNE, 2+ = other problems
 * 					0 = all good
 * 					1 = file does not exist or other problem
 * 					2 = other problem
 * 					3 = file type not recognized
 *
 * NOTES: For this function, the file type is the important parameter because it tells the function how to
 * 			interpret the parameters which are given.
 * 		: For the Log and configuration files, parameters other than file_type should be written as 0's.
 * 		: For CPS, WAV, and 2DH files, the set numbers should be 0's.
 * 		: For EVT, the set numbers give a way to selectively transfer one or more set files at a time. If the
 * 			set_num_high value is 0, then just one set file will be TX'd. Otherwise, each set file from set low
 * 			to set high will be sent. There will be a checks on the user input, but no SOH in between the files.
 */
int TransferSDFile( XUartPs Uart_PS, char * RecvBuffer, int file_type, int id_num, int run_num, int set_num )
{
	int status = 0;			//0=good, 1=file DNE, 2+=other problem
	int poll_val = 0;		//local polling status variable
	int sent = 0;			//bytes sent by UART
	int bytes_sent = 0;
	short s_holder = 0;
	unsigned short us_holder = 0;
	float f_holder = 0;
	int file_TX_size = 0;				//tracks number of bytes left to TX in the file TOTAL
	int file_TX_packet_size = 0;		//number of bytes to send //number of bytes in a packet total
	int file_TX_data_bytes_size = 0;	//size of the data bytes for that type of data product packet
	int file_TX_packet_header_size = 0;	//size of the packet header bytes minus the CCSDS primary header (10 bytes)
	int file_TX_add_padding = 0;		//flag to add padding bytes to an outgoing packet
	int file_TX_group_flags = 0;
	int file_TX_sequence_count = 0;
	int file_TX_apid = 0;
	int file_TX_file_pointer_location = 0;
	int m_loop_var = 1;					//0 = false; 1 = true
	int bytes_to_read = 0;				//number of bytes to read from data file to put into packet data bytes
	unsigned int bytes_written = 0;
	unsigned int bytes_read = 0;
	unsigned int file_TX_2DH_oor_values[5] = {};
	char *ptr_file_TX_filename = NULL;
	char WF_FILENAME[] = "wf01.bin";
	char log_file[] = "MNSCMDLOG.txt";
	char config_file[] = "MNSCONF.bin";
	char file_TX_folder[100] = "";
	char file_TX_filename[100] = "";
	char file_TX_path[100] = "";
	unsigned char packet_array[2040] = "";	//TODO: check if I can drop the 2040 -> TELEMETRY_MAX_SIZE (2038)
	DATA_FILE_HEADER_TYPE data_file_header = {};
	DATA_FILE_SECONDARY_HEADER_TYPE data_file_2ndy_header = {};
	CONFIG_STRUCT_TYPE config_file_header = {};
	DATA_FILE_FOOTER_TYPE data_file_footer = {};
	FIL TXFile;				//file object
	FILINFO fno;			//file info structure
	//Initialize the FILINFO struct with something //If using LFN, then we need to init these values, otherwise we don't
	TCHAR LFName[256];
	fno.lfname = LFName;
	fno.lfsize = sizeof(LFName);
	FRESULT f_res = FR_OK;	//SD card status variable type

	//find the folder/file that was requested
	if(file_type == DATA_TYPE_LOG)
	{
		//just on the root directory
		bytes_written = snprintf(file_TX_folder, 100, "0:");
		if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE)
			status = 1;
		ptr_file_TX_filename = log_file;
	}
	else if(file_type == DATA_TYPE_CFG)
	{
		//just on the root directory
		bytes_written = snprintf(file_TX_folder, 100, "0:");
		if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE)
			status = 1;
		ptr_file_TX_filename = config_file;
	}
	else if(file_type == DATA_TYPE_WAV)
	{
		//construct the folder
		bytes_written = snprintf(file_TX_folder, 100,  "0:/WF_I%d", id_num);
		if(bytes_written == 0)
			status = 1;
		//construct the file name
		ptr_file_TX_filename = WF_FILENAME;
	}
	else
	{
		//construct the folder
		bytes_written = snprintf(file_TX_folder, 100, "0:/I%04d_R%04d", id_num, run_num);
		if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE + DAQ_FOLDER_SIZE)
			status = 1;
		//construct the file name
		if(file_type == DATA_TYPE_EVT)
		{
			bytes_written = snprintf(file_TX_filename, 100, "evt_S%04d.bin", set_num);
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_WAV)
		{
			bytes_written = snprintf(file_TX_filename, 100, "wav_S%04d.bin", set_num);
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_CPS)
		{
			bytes_written = snprintf(file_TX_filename, 100, "cps.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_0)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d0.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_1)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d1.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_2)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d2.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else if(file_type == DATA_TYPE_2DH_3)
		{
			bytes_written = snprintf(file_TX_filename, 100, "2d3.bin");
			if(bytes_written == 0)
				status = 1;
		}
		else
		{
			//the file type was not recognized
			//we should break out and not go further
			status = 3;	//indicate that the file type is bad
		}

		ptr_file_TX_filename = file_TX_filename;
	}

	//write the total file path
	bytes_written = snprintf(file_TX_path, 100, "%s/%s", file_TX_folder, ptr_file_TX_filename);
	if(bytes_written == 0)
		status = 1;

	//check that the folder/file we just wrote exists in the file system
	//check first so that we don't just open a blank new file; there are no protections for that
	f_res = f_stat(file_TX_path, &fno);
	if(f_res == FR_NO_FILE)
	{
		//couldn't find the folder
		status = 1;	//folder DNE
	}

	if(status == 0)
	{
		//can just do an open on the dir:/folder/file.bin if we want, that way we don't have to use chdir or anything
		f_res = f_open(&TXFile, file_TX_path, FA_READ);	//the files exists, so just open it //only do fa-read so that we don't open a new file
		if(f_res != FR_OK)
		{
			if(f_res == FR_NO_PATH)
				status = 1;
			else
				status = 2;
		}
	}
	//read in important information (file size, header, first event, real time, etc.)
	if(status == 0)
	{
		file_TX_size = file_size(&TXFile);
		if(file_type != DATA_TYPE_LOG && file_type != DATA_TYPE_CFG)	//EVT, CPS, 2DH, WAV files
		{
			f_res = f_read(&TXFile, &data_file_header, sizeof(data_file_header), &bytes_read);	//read in the data file header
			if(f_res != FR_OK || bytes_read != sizeof(data_file_header))
				status = 2;
			else
				file_TX_size -= bytes_read;

			if(file_type == DATA_TYPE_EVT || file_type == DATA_TYPE_CPS) //EVT, CPS files
			{
				f_res = f_read(&TXFile, &data_file_2ndy_header, sizeof(data_file_2ndy_header), &bytes_read);	//read in the secondary data file header
				if(f_res != FR_OK)
				{
					//TODO: can do a check that the eventID bytes are correct here so we know that it's a good read?
					// could use this as a verification, but maybe it's too much
					status = 2;
				}
				else
					file_TX_size -= bytes_read;
			}
			if(file_type == DATA_TYPE_EVT)
			{
				f_res = f_lseek(&TXFile, DP_HEADER_SIZE);	//move past the
				if(f_res != FR_OK)
					status = 2;
				else
					file_TX_size -= (DP_HEADER_SIZE - sizeof(data_file_header) - sizeof(data_file_2ndy_header));	//this is correct 10-02-2019
			}
		}
		else if(file_type == DATA_TYPE_CFG)	//the config file is just one config header
		{
			f_res = f_read(&TXFile, &config_file_header, sizeof(config_file_header), &bytes_read);
			if(f_res != FR_OK || bytes_read != sizeof(config_file_header))
				status = 2;
			else
				file_TX_size -= bytes_read;
		}
		//no header information in the log file //need to assign the
	}
	//deal with footers on the files, if they exist
	if(status == 0)
	{
		//get the location of the file pointer so we can reset it there when we are done
		file_TX_file_pointer_location = TXFile.fptr;
		if(file_type == DATA_TYPE_EVT)
		{
			f_res = f_lseek(&TXFile, file_size(&TXFile) - sizeof(data_file_footer));	//goes to the end minus the footer size
			if(f_res != FR_OK)
				status = 2;
			else
			{
				f_res = f_read(&TXFile, &data_file_footer, sizeof(data_file_footer), &bytes_read);
				if(f_res != FR_OK || bytes_read != sizeof(data_file_footer))
					status = 2;
			}

			file_TX_size -= sizeof(data_file_footer);
		}
		else if(file_type == DATA_TYPE_CPS)
		{
			f_res = f_lseek(&TXFile, file_size(&TXFile) - sizeof(data_file_footer));
			if(f_res != FR_OK)
				status = 2;
			else
			{
				f_res = f_read(&TXFile, &data_file_footer, sizeof(data_file_footer), &bytes_read);
				if(f_res != FR_OK || bytes_read != sizeof(data_file_footer))
					status = 2;
			}

			file_TX_size -= sizeof(data_file_footer);
		}
		else if(file_type == DATA_TYPE_2DH_0 || file_type == DATA_TYPE_2DH_1 || file_type == DATA_TYPE_2DH_2 || file_type == DATA_TYPE_2DH_3 )
		{
			f_res = f_lseek(&TXFile, file_size(&TXFile) - FILE_FOOT_2DH);
			if(f_res != FR_OK)
				status = 2;
			else
			{
				f_res = f_read(&TXFile, &file_TX_2DH_oor_values, sizeof(unsigned int) * 5, &bytes_read);
				if(f_res != FR_OK || bytes_read != (sizeof(unsigned int) * 5))
					status = 2;
			}

			file_TX_size -= sizeof(unsigned int) * 5;
		}
		//reset the file pointer to where it was when we began
		f_res = f_lseek(&TXFile, file_TX_file_pointer_location);
	}

	//compile the Mini-NS data header (different based on file type)
	if(status == 0)
	{
		//for EVT, CPS, 2DH, WAV file types
		//fill in the Mini-NS Data Header	//these are shared header values for CPS, 2DH, EVT, WAV, CFG //only LOG doesn't have this
		//will probably have to keep the floats like this to ensure that they get copied correctly
		//when we are keeping the other parameters consistent with the packet values (ie. unsigned short on file -> unsigned short in packet) then
		// we'll be able to just use memcpy rather than using the holder variables
		f_holder = data_file_header.configBuff.ECalSlope;								memcpy(&(packet_array[11]), &f_holder, sizeof(float));
		f_holder = data_file_header.configBuff.ECalIntercept;							memcpy(&(packet_array[15]), &f_holder, sizeof(float));
		us_holder = (unsigned short)data_file_header.configBuff.TriggerThreshold;		memcpy(&(packet_array[19]), &us_holder, sizeof(us_holder));
		s_holder = (short)data_file_header.configBuff.IntegrationBaseline;		memcpy(&(packet_array[21]), &s_holder, sizeof(s_holder));
		s_holder = (short)data_file_header.configBuff.IntegrationShort;		memcpy(&(packet_array[23]), &s_holder, sizeof(s_holder));
		s_holder = (short)data_file_header.configBuff.IntegrationLong;			memcpy(&(packet_array[25]), &s_holder, sizeof(s_holder));
		s_holder = (short)data_file_header.configBuff.IntegrationFull;			memcpy(&(packet_array[27]), &s_holder, sizeof(s_holder));
		us_holder = (unsigned short)data_file_header.configBuff.HighVoltageValue[0];	memcpy(&(packet_array[29]), &us_holder, sizeof(us_holder));
		us_holder = (unsigned short)data_file_header.configBuff.HighVoltageValue[1];	memcpy(&(packet_array[31]), &us_holder, sizeof(us_holder));
		us_holder = (unsigned short)data_file_header.configBuff.HighVoltageValue[2];	memcpy(&(packet_array[33]), &us_holder, sizeof(us_holder));
		us_holder = (unsigned short)data_file_header.configBuff.HighVoltageValue[3];	memcpy(&(packet_array[35]), &us_holder, sizeof(us_holder));
		us_holder = (unsigned short)data_file_header.IDNum;												memcpy(&(packet_array[37]), &us_holder, sizeof(us_holder));
		us_holder = (unsigned short)data_file_header.RunNum;											memcpy(&(packet_array[39]), &us_holder, sizeof(us_holder));

		if(file_type != DATA_TYPE_LOG && file_type != DATA_TYPE_CFG && file_type != DATA_TYPE_WAV)
		{
			memcpy(&(packet_array[41]), &data_file_2ndy_header.RealTime, sizeof(data_file_2ndy_header.RealTime));
			memcpy(&(packet_array[45]), &data_file_2ndy_header.FirstEventTime, sizeof(data_file_2ndy_header.FirstEventTime));
		}
		else
		{
			//the log file, config file, and WAV files don't have spacecraft or FPGA times
			memset(&(packet_array[41]), 0, sizeof(unsigned int));	//just set the variables to 0
			memset(&(packet_array[45]), 0, sizeof(unsigned int));
		}
	}
	if(status != 0)
		m_loop_var = 0;	//don't loop, just exit
	//All above information is necessary to acquire once as it stays the same in each packet
	//This loop compiles the remaining parts of the packet and sends it, then repeats until there is no more data
	while(m_loop_var == 1)
	{
		switch(file_type)
		{
		case DATA_TYPE_CPS:
			file_TX_data_bytes_size = DATA_BYTES_CPS;
			file_TX_packet_size = PKT_SIZE_CPS;
			file_TX_packet_header_size = PKT_HEADER_CPS;
			file_TX_apid = 0x55;
			break;
		case DATA_TYPE_WAV:
			file_TX_data_bytes_size = DATA_BYTES_WAV;
			file_TX_packet_size = PKT_SIZE_WAV;
			file_TX_packet_header_size = PKT_HEADER_WAV;
			file_TX_apid = 0x66;
			break;
		case DATA_TYPE_EVT:
			//what do we need to specify to make things correct for one data product or another?
			file_TX_data_bytes_size = DATA_BYTES_EVT;
			file_TX_packet_size = PKT_SIZE_EVT;
			file_TX_packet_header_size = PKT_HEADER_EVT;
			file_TX_apid = 0x77;
			break;
		case DATA_TYPE_2DH_0:
			/* Falls through to case 2DH_2 */
		case DATA_TYPE_2DH_1:
			/* Falls through to case 2DH_3 */
		case DATA_TYPE_2DH_2:
			/* Falls through to case 2DH_4 */
		case DATA_TYPE_2DH_3:
			file_TX_data_bytes_size = DATA_BYTES_2DH - 1;	//Subtract one byte, there is an additional field (PMT ID) added to the packet
			file_TX_packet_size = PKT_SIZE_2DH;
			file_TX_packet_header_size = PKT_HEADER_2DH;
			file_TX_apid = 0x88;
			break;
		case DATA_TYPE_LOG:
			//can't do these yet
			file_TX_data_bytes_size = DATA_BYTES_LOG;
			file_TX_packet_size = PKT_SIZE_LOG;
			file_TX_packet_header_size = PKT_HEADER_LOG;
			file_TX_apid = 0x99;
			break;
		case DATA_TYPE_CFG:
			file_TX_data_bytes_size = DATA_BYTES_CFG;
			file_TX_packet_size = PKT_SIZE_CFG;
			file_TX_packet_header_size = PKT_HEADER_CFG;
			file_TX_apid = 0xAA;
			break;
		default:
			status = 3;		//problem with the file type
			m_loop_var = 0;	//stop looping
			break;
		}

		//check to see if we should add padding, and what the group flags should be
		if(file_TX_size > file_TX_data_bytes_size)
		{
			bytes_to_read = file_TX_data_bytes_size;
			file_TX_add_padding = 0;
			if(file_TX_sequence_count == 0)
				file_TX_group_flags = GF_FIRST_PACKET;	//first packet // 1
			else
				file_TX_group_flags = GF_INTER_PACKET;	//intermediate packet // 0
		}
		else
		{
			bytes_to_read = file_TX_size;
			file_TX_add_padding = 1;
			if(file_TX_sequence_count == 0)
				file_TX_group_flags = GF_UNSEG_PACKET;	//unsegmented packet // 3
			else
				file_TX_group_flags = GF_LAST_PACKET;	//last packet // 2
		}

		//need to generalize this for the CFG, LOG transfers, they don't have a data file header to read the APID from
		PutCCSDSHeader(packet_array, data_file_header.FileTypeAPID, file_TX_group_flags, file_TX_sequence_count, file_TX_packet_size);

		f_res = f_read(&TXFile, &(packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size]), bytes_to_read, &bytes_read);
		if(f_res != FR_OK)
			status = 2;
		else
			file_TX_size -= bytes_read;	//set to bytes_read 5-21

		//add padding bytes, if necessary
		if(file_TX_add_padding == 1)
		{
			memset(&(packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size + bytes_read]), file_TX_apid, file_TX_data_bytes_size - bytes_read);
			file_TX_add_padding = 0;	//reset
		}

		//for 2DH packets, add the PMT ID to the end of the data bytes //currently at 10+39+1984 = 2033 //10-4-2019
		if(file_type == DATA_TYPE_2DH_0)
			packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size + file_TX_data_bytes_size] = 0x01;
		else if(file_type == DATA_TYPE_2DH_1)
			packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size + file_TX_data_bytes_size] = 0x02;
		else if(file_type == DATA_TYPE_2DH_2)
			packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size + file_TX_data_bytes_size] = 0x04;
		else if(file_type == DATA_TYPE_2DH_3)
			packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size + file_TX_data_bytes_size] = 0x08;

		//calculate the checksums for the packet
		CalculateChecksums(packet_array);

		//send the packet
		sent = 0;
		bytes_sent = 0;
		file_TX_packet_size += CCSDS_HEADER_FULL;	//the full packet size in bytes
		while(sent < file_TX_packet_size)
		{
			bytes_sent = XUartPs_Send(&Uart_PS, &(packet_array[sent]), file_TX_packet_size - sent);
			sent += bytes_sent;
		}

		while(XUartPs_IsSending(&Uart_PS))
		{
			//wait here
		}

		//add in a "wait time" where we allow the XB-1 flight computer enough time to have completely emptied
		// its receive buffer so that we don't overwrite anything
		XTime_GetTime(&wait_start);
		while(1)
		{
			XTime_GetTime(&wait_timer);
			if((float)(wait_timer - wait_start)/COUNTS_PER_SECOND >= 0.015)
				break;
		}

		//check if there are multiple packets to send
		switch(file_TX_group_flags)
		{
		case 0:	//intermediate packet
			/* Falls through to case 1 */
		case 1:	//first packet
			m_loop_var = 1;
			file_TX_sequence_count++;
			//erase the parts of the packet which are unique so they don't get put into the next packet
			//this includes everything past the data header
			memset(&(packet_array[6]), '\0', 2);	//reset group flags, sequence count
			memset(&(packet_array[10]), '\0', 1);	//reset secondary header (reset request bits)
			if(file_type == DATA_TYPE_EVT || file_type == DATA_TYPE_WAV || file_type == DATA_TYPE_CPS)
				memset(&(packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size]), '\0', file_TX_packet_size - CCSDS_HEADER_PRIM - file_TX_packet_header_size);
			else if(file_type == DATA_TYPE_2DH_0 || file_type == DATA_TYPE_2DH_1 || file_type == DATA_TYPE_2DH_2 || file_type == DATA_TYPE_2DH_3 )
				memset(&(packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size]), '\0', file_TX_packet_size - CCSDS_HEADER_PRIM - file_TX_packet_header_size);
			else if(file_type == DATA_TYPE_LOG)
				memset(&(packet_array[CCSDS_HEADER_PRIM + file_TX_packet_header_size]), '\0', file_TX_packet_size - CCSDS_HEADER_PRIM);	//modify this for CFG, LOG
			break;
		case 2:	//current packet was last packet
			/* Falls through to case 3 */
		case 3:	//current packet was unsegmented
			m_loop_var = 0;
			break;
		default:
			//TODO: error check bad group flags, for now just be done, don't loop back
			m_loop_var = 0;
			status = 2;
			break;
		}

		//check for user interaction (break, too many bytes)
		poll_val = ReadCommandType(RecvBuffer, &Uart_PS);
		switch(poll_val)
		{
		case -1:
			//this is bad input or an error in input
			//TODO: handle this separately from default
			break;
		case BREAK_CMD:
			m_loop_var = 0;	//done looping
			status = DAQ_BREAK;
			break;
		default:
			break;
		}
	}//END OF WHILE(m_loop_var == 1)

	f_close(&TXFile);

	return status;
}

/*
 * Function to send a packet of data out across the RS-422
 * Pass in the variables that we need, the UART handle, the packet to send, the number of bytes
 * Pass in the total number of bytes to send. This function does no math on the bytes_to_send variable
 *  that is passed in, so as to remain transparent about what is being done.
 *
 * @param	(XUartPS)	The instance of the UART so we can push packets to the bus
 * @param	(unsigned char *)	pointer to the packet buffer
 * @param	(int)		total bytes in the packet (should be the same for packets of the same type)
 *
 * @return	(int)
 */
int SendPacket( XUartPs Uart_PS, unsigned char *packet_buffer, int bytes_to_send )
{
	int status = 0;
	int sent = 0;
	int bytes_sent = 0;

	//send the packet
	while(sent < bytes_to_send)
	{
		bytes_sent = XUartPs_Send(&Uart_PS, &(packet_buffer[sent]), bytes_to_send - sent);
		sent += bytes_sent;
	}

	while(XUartPs_IsSending(&Uart_PS))
	{
		//wait here
	}

	//add in a "wait time" where we allow the XB-1 flight computer enough time to have completely emptied
	// its receive buffer so that we don't overwrite anything
	XTime_GetTime(&wait_start);
	while(1)
	{
		XTime_GetTime(&wait_timer);
		if((float)(wait_timer - wait_start)/COUNTS_PER_SECOND >= 0.015)
			break;
	}

	return status;
}
