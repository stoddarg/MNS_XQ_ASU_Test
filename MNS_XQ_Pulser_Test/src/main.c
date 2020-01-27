/******************************************************************************
*
* Copyright (C) 2014 - 2016 Xilinx, Inc. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * Mini-NS Flight Software, Version 5.53
 * Graham Stoddard, 5/31/2019
 *
 * 02-25-2019
 * Added a compiler option "m" to allow us to include math.h to be linked in so we
 *  may use the floor() function. If this can be worked around, I think we should. - GJS
 *
 * 1-7-2020
 * We can use the compiler optimization, the issue turned out to be something else, see notes.
 * Added "xilffs" library to be compiled in. This solves so many of the issues I had when
 *  trying to build this project over the past couple of years.
 * ASU - 921600 baud
 * HSFL - 115200 baud
 * Currently using 1 MiB stack & heap
 * 15 ms "protection" period should be applied to all UART send commands so the XB-1 doesn't
 *  get swamped when we are sending packets to it.
 *
 * 1-22-2020
 * The boot files created at 12:30pm are Version 7.1
 */

#include "main.h"

int main()
{
	int status = 0;			//local status variable for reporting SUCCESS/FAILURE

	init_platform();		//Maybe we dropped out important init functions?
	ps7_post_config();
	Xil_DCacheDisable();	// Disable the L1/L2 data caches
	InitializeAXIDma();		// Initialize the AXI DMA Transfer Interface

	status = InitializeInterruptSystem(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	if(status != XST_SUCCESS)
	{
		xil_printf("Interrupt system initialization error\n");

	}
	// *********** Setup the Hardware Reset GPIO ****************//
	// GPIO/TEC Test Variables
	XGpioPs Gpio;
	int gpio_status = 0;
	XGpioPs_Config *GPIOConfigPtr;

	GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
	gpio_status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr->BaseAddr);
	if(gpio_status != XST_SUCCESS)
		xil_printf("GPIO PS init failed\r\n");

	XGpioPs_SetDirectionPin(&Gpio, TEC_PIN, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, TEC_PIN, 1);
	XGpioPs_WritePin(&Gpio, TEC_PIN, 0);	//disable TEC startup

	XGpioPs_SetDirectionPin(&Gpio, SW_BREAK_GPIO, 1);
	//******************Setup and Initialize IIC*********************//
	int iic_status = 0;
	//Make the IIC Instance come from here and we pass it in to the functions
	XIicPs Iic;

	iic_status = IicPsInit(&Iic, IIC_DEVICE_ID_0);
	if(iic_status != XST_SUCCESS)
	{
		//handle the issue
		xil_printf("fix the Iic device 0\r\n");
	}
	iic_status = IicPsInit(&Iic, IIC_DEVICE_ID_1);
	if(iic_status != XST_SUCCESS)
	{
		//handle the issue
		xil_printf("fix the Iic device 1\r\n");
	}

	//******************* Set parameters in the FPGA **********************//
	Xil_Out32 (XPAR_AXI_GPIO_0_BASEADDR, 38);	//baseline integration time	//subtract 38 from each int
	Xil_Out32 (XPAR_AXI_GPIO_1_BASEADDR, 73);	//short
	Xil_Out32 (XPAR_AXI_GPIO_2_BASEADDR, 169);	//long
	Xil_Out32 (XPAR_AXI_GPIO_3_BASEADDR, 1551);	//full
	Xil_Out32 (XPAR_AXI_GPIO_6_BASEADDR, 0);	//disable the system, allows data
	Xil_Out32 (XPAR_AXI_GPIO_7_BASEADDR, 0);	//disable 5V to sensor head
	Xil_Out32 (XPAR_AXI_GPIO_10_BASEADDR, 8500);	//threshold, max of 2^14 (16384)
	Xil_Out32 (XPAR_AXI_GPIO_16_BASEADDR, 16384);	//master-slave frame size
	Xil_Out32 (XPAR_AXI_GPIO_17_BASEADDR, 1);	//master-slave enable
	Xil_Out32 (XPAR_AXI_GPIO_18_BASEADDR, 0);	//capture module disable

	//*******************Setup the UART **********************//
	status = 0;
	int LoopCount = 0;
	XUartPs Uart_PS;	// instance of UART

	XUartPs_Config *Config = XUartPs_LookupConfig(UART_DEVICEID);
	if (Config == NULL) { return 1;}
	status = XUartPs_CfgInitialize(&Uart_PS, Config, Config->BaseAddress);
	if (status != 0){ xil_printf("XUartPS did not CfgInit properly.\n");	}

	/* Conduct a Selftest for the UART */
	status = XUartPs_SelfTest(&Uart_PS);
	if (status != 0) { xil_printf("XUartPS failed self test.\n"); }			//handle error checks here better

	/* Set to normal mode. */
	XUartPs_SetOperMode(&Uart_PS, XUARTPS_OPER_MODE_NORMAL);
	while (XUartPs_IsSending(&Uart_PS)) {
		LoopCount++;
	}
	// *********** Mount SD Card ****************//
	/* FAT File System Variables */
	FRESULT f_res = FR_OK;
	FATFS fatfs[2];
	int sd_status = 0;

	sd_status = MountSDCards( fatfs );
	if(sd_status == CMD_SUCCESS)	//correct mounting
	{
		sd_status = InitLogFile0();	//create log file on SD0
		if(sd_status == CMD_FAILURE)
		{
			//handle a bad log file?
			xil_printf("SD0 failed to init\r\n");
		}
		sd_status = InitLogFile1();	//create log file on SD1
		if(sd_status == CMD_FAILURE)
		{
			//handle a bad log file?
			xil_printf("SD1 failed to init\r\n");
		}
	}
	else
	{
		xil_printf("SD0/1 failed to mount\r\n");
		//need to handle the SD card not reading
		//do we try each one separately then set a flag?
		sd_status = MountSD0(fatfs);
		if(sd_status == CMD_SUCCESS)
		{
			//SD0 is not the problem
		}
		else
		{
			//SD0 is the problem
			//set a flag to indicate to only use SD1?
			xil_printf("SD0 failed to mount\r\n");
		}
		sd_status = MountSD1(fatfs);
		if(sd_status == CMD_SUCCESS)
		{
			//SD1 is not the problem
		}
		else
		{
			//SD1 is the problem
			//set a flag to indicate to only use SD0?
			xil_printf("SD1 failed to mount\r\n");
		}
	}
	// *********** Initialize Mini-NS System Parameters ****************//
	InitConfig();

	// *********** Initialize Local Variables ****************//
	InitTempSensors(&Iic);
	InitStartTime();			//start timing
	SetModeByte(MODE_STANDBY);	//set the mode byte to standby

	// Initialize buffers
	char RecvBuffer[100] = "";	//user input buffer
	int done = 0;				//local status variable for keeping track of progress within loops
	int DAQ_run_number = 0;		//run number value for file names, tracks the number of runs per POR
	int	menusel = 99999;		//case select variable for polling
	FIL *cpsDataFile;			//create FIL pointers to check and make sure that the DAQ files were closed out
	FIL *evtDataFile;
	// ******************* WF Data Product Variables *******************//
	int valid_data = 0;		//local test variable for WF
	int numWFs = 0;
	int dram_addr = 0;
	int array_index = 0;
	int wf_id_number = 0;
	unsigned int numBytesWritten = 0;
	char wf_run_folder[100] = "";
	DATA_FILE_HEADER_TYPE wf_file_header = {};
	unsigned int wf_data[DATA_BUFFER_SIZE] = {};
	char WF_FILENAME[] = "wf01.bin";
	FIL WFData;
	FILINFO fno;		//file info structure
	TCHAR LFName[256];
	fno.lfname = LFName;
	fno.lfsize = sizeof(LFName);
	//packet buffer for DIR function
	char dir_sd_card_buffer[10] = "";
	unsigned char dir_packet_buffer[TELEMETRY_MAX_SIZE] = "";
	int bytes_written = 0;

	// ******************* APPLICATION LOOP *******************//

	//This loop will continue forever and the program won't leave it
	//This loop checks for input from the user, then checks to see if it's time to report SOH
	//If input is received, then it reads the input for correctness
	// if input is a valid MNS command, the system processes the command and reacts
	// if not, then the system rejects the command and issues a failure packet
	//After checking for input, the clock is checked to see if it is time to report SOH
	//When it is time, reports a full CCSDS SOH packet

	while(1){	//OUTER LEVEL 3 TESTING LOOP
		while(1){
			//resetting this value every time is (potentially) critical
			//resetting this ensures we don't re-use a command a second time (erroneously)
			menusel = 99999;
			menusel = ReadCommandType(RecvBuffer, &Uart_PS);	//Check for user input

			if ( menusel >= -1 && menusel <= 15 )	//let all input in, including errors, so we can report them //we are not handling break, end, start, overflow by keeping this to 15...
			{
				//we found a valid LUNAH command or input was bad (-1)
				//log the command issued, unless it is an error
				if(menusel != -1)
					LogFileWrite( GetLastCommand(), GetLastCommandSize() );
				break;	//leave the inner loop and execute the commanded function
			}
			//check to see if it is time to report SOH information, 1 Hz
			CheckForSOH(&Iic, Uart_PS);
		}//END TEMP ASU TESTING LOOP

		//MAIN MENU OF FUNCTIONS
		switch (menusel) { // Switch-Case Menu Select
		case -1:
			//we found an invalid command
			//Report CMD_FAILURE
			reportFailure(Uart_PS);
			break;
		case DAQ_CMD:
			Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, 4);	//set processed data mode
			Xil_Out32 (XPAR_AXI_GPIO_7_BASEADDR, 1);	//enable 5V to analog board
			//set all the configuration parameters
			status = ApplyDAQConfig(&Iic);
			if(status != CMD_SUCCESS)
			{
				//TODO: more error checking
			}
			//prepare the status variables
			done = 0;	//not done yet
			ResetSOHNeutronCounts();	//set the SOH counts to 0
			CPSInit();	//reset neutron counts for the run
			status = CMD_SUCCESS;	//reset the variable so that we jump into the loop
			SetModeByte(MODE_PRE_DAQ);
			SetIDNumber(GetIntParam(1));
			/* Create the file names we will use for this run:
			 * Check if the filename given is unique
			 * if the filename is unique, then we will go through these functions once
			 * if not, then we will loop until a unique name is found */
			while(status == CMD_SUCCESS)
			{
				//only report a packet when the file has been successfully changed and did not exist already?
				++DAQ_run_number;	//initialized to 0, the first run will increment to 1
				SetRunNumber(DAQ_run_number);
				SetFileName(GetIntParam(1), DAQ_run_number, 0);	//creates a file name of IDNum_runNum_type.bin
				//check that the file name(s) do not already exist on the SD card...we do not want to append existing files

				status = DoesFileExist();
//				if(status == CMD_ERROR)	// commented
//				{
//					//handle a non-success/failure return
//					//this means we don't have access to something
//					//TODO: handle return
//				}
				//returns FALSE if file does NOT exist
				//returns TRUE if file does exist
				//we need the file to be unique, so FALSE is a positive result,
				// if we get TRUE(CMD_SUCCESS), we need to keep looping
				//when status is FALSE(CMD_FAILURE), we send a packet to report the file name
				if(status == CMD_FAILURE)
				{
					reportSuccess(Uart_PS, 1);
					//create the files before polling for user input, leave them open
					//This also fills in the data header, as much as we know
					status = CreateDAQFiles();
					if(status == CMD_SUCCESS)
						break;
					else
					{
						done = 1;	//we can't make the files correctly, report failure and return?
						reportFailure(Uart_PS);
						//TODO: may want check that we don't have any other files open
					}
				}
				CheckForSOH(&Iic, Uart_PS);
			}
			while(done != 1)
			{
				status = ReadCommandType(RecvBuffer, &Uart_PS);	//Check for user input
				if ( status >= -1 && status <= 23 )
				{
					if(status != -1)
						LogFileWrite( GetLastCommand(), GetLastCommandSize() );

					switch(status)
					{
					case -1:
						done = 0;
						reportFailure(Uart_PS);
						break;
					case READ_TMP_CMD:
						done = 0;
						status = report_SOH(&Iic, GetLocalTime(), Uart_PS, READ_TMP_CMD);
						if(status == CMD_FAILURE)
							reportFailure(Uart_PS);
						break;
					case BREAK_CMD:
						done = 1;
						reportSuccess(Uart_PS, 0);
						break;
					case START_CMD:
						SetRealTime(GetRealTimeParam());
						status = DataAcquisition(&Iic, Uart_PS, RecvBuffer, GetIntParam(1));
						//we will return in three ways:
						// BREAK (0)	= failure
						// time out (1) = success
						// END (2)		= success
						switch(status)
						{
						case 0:
							LogFileWrite( GetLastCommand(), GetLastCommandSize() );
							reportFailure(Uart_PS);
							break;
						case 1:
							reportSuccess(Uart_PS, 0);
							break;
						case 2:
							LogFileWrite( GetLastCommand(), GetLastCommandSize() );
							reportSuccess(Uart_PS, 0);
							break;
						default:
							reportFailure(Uart_PS);
							break;
						}
						done = 1;
						break;
					default:
						//got something outside of these commands
						done = 0;	//continue looping //not done
						reportFailure(Uart_PS);
						break;
					}
				}
				//check to see if it is time to report SOH information, 1 Hz
				CheckForSOH(&Iic, Uart_PS);
			}//END OF WHILE DONE != 0

			cpsDataFile = GetCPSFilePointer();	//check the FIL pointers created by DAQ are closed safely
			if (cpsDataFile->fs != NULL)
				f_close(cpsDataFile);
			evtDataFile = GetEVTFilePointer();
			if (evtDataFile->fs != NULL)
				f_close(evtDataFile);

			//change directories back to the root directory
			f_res = f_chdir("0:/");
			if(f_res != FR_OK)
			{
				//TODO: handle change directory fail
			}
			//data acquisition has been completed, wrap up anything not handled by the DAQ function
			//turn off the active components //TODO: add this to the Flow Diagram so people know that it's off
			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable capture module
			Xil_Out32(XPAR_AXI_GPIO_6_BASEADDR, 0);		//disable ADC
			Xil_Out32 (XPAR_AXI_GPIO_7_BASEADDR, 0);	//disable 5V to analog board

			ClearBRAMBuffers();							//tell FPGA there is a buffer it can write to //TEST

			SetModeByte(MODE_STANDBY);
			SetIDNumber(0);
			SetRunNumber(0);
			break;
		case WF_CMD:
			//set processed data mode
			if(GetIntParam(1) == 0)
				Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, GetIntParam(1));	//get AA wfs = 0 //TRG wfs = 3
			else
			{
				reportFailure(Uart_PS);	//report failure
				break;
			}

			Xil_Out32 (XPAR_AXI_GPIO_7_BASEADDR, 1);	//enable 5V to analog board
			status = ApplyDAQConfig(&Iic);

			wf_id_number = GetIntParam(3);
			while(1)
			{
				numBytesWritten = snprintf(wf_run_folder, 100, "0:/WF_I%04d", wf_id_number);
				if(numBytesWritten == 0)
					status = CMD_FAILURE;
				f_res = f_stat(wf_run_folder, &fno);
				if(f_res == FR_NO_FILE)
					break;
				else if(f_res == FR_NO_PATH)
					break;
				else
					wf_id_number++;
			}

			f_res = f_mkdir(wf_run_folder);
			if(f_res != FR_OK)
			{
				done = 1;
			}
			else
			{
				f_res = f_chdir(wf_run_folder);
				if(f_res != FR_OK)
				{
					done = 1;
				}
				//don't open the file unless we have changed directory first
				f_res = f_open(&WFData, WF_FILENAME, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
				if(f_res != FR_OK)
					xil_printf("1 open file fail WF\n");
				else
				{
					//if we open the file, create and write in the header file
					wf_file_header.configBuff = *GetConfigBuffer();	//dereference to copy the struct into our local struct
					wf_file_header.IDNum = wf_id_number;
					wf_file_header.RunNum = 0;
					wf_file_header.SetNum = 0;
					wf_file_header.FileTypeAPID = DATA_TYPE_WAV;
					wf_file_header.TempCorrectionSetNum = 1;
					wf_file_header.EventID1 = 0xFF;
					wf_file_header.EventID2 = 0xFF;

					//write the header into the file
					f_res = f_write(&WFData, &wf_file_header, sizeof(wf_file_header), &numBytesWritten);
					if(f_res == FR_OK && numBytesWritten == sizeof(wf_file_header))
					{
						f_res = f_sync(&WFData);
						if(f_res == FR_OK)
							done = 0;
						else
							done = 1;
					}
				}
			}

			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 1);	//enable capture module
			Xil_Out32(XPAR_AXI_GPIO_6_BASEADDR, 1);		//enable ADC
			numWFs = 0;
			memset(wf_data, '\0', sizeof(unsigned int)*DATA_BUFFER_SIZE);
			SetModeByte(MODE_DAQ_WF);
			SetIDNumber(wf_id_number);
			SetRunNumber(0);
			ClearBRAMBuffers();
			while(numWFs < GetIntParam(2))
			{
				valid_data = Xil_In32 (XPAR_AXI_GPIO_11_BASEADDR);
				if(valid_data == 1)
				{
					//init/start MUX to transfer data between integrator modules and the DMA
					Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 1);
					Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x48, DRAM_BASE);
					Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x58, 65536);
					usleep(54);
					//TODO: need to check a shared variable within the interrupt handler and this function
					// to see if the transfer is completed
					//This check would replace the sleep statement.

					Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 0);
					ClearBRAMBuffers();
					Xil_DCacheInvalidateRange(DRAM_BASE, 65536);

					array_index = 0;
					dram_addr = DRAM_BASE;
					while(dram_addr < DRAM_CEILING)
					{
						//TODO: cast these 32-bit values as unsigned 16-bit values so we save space and packets, etc.
						wf_data[array_index] = Xil_In32(dram_addr);
						dram_addr += 4;
						array_index++;
					}

					numWFs++;
					//have the WF, save to file //WFData
					//NOTE: the values from the DRAM are 16 bit numbers written into 32 bit fields - the LSBs are where the values are stored
					f_res = f_write(&WFData, wf_data, DATA_BUFFER_SIZE * sizeof(unsigned int), &numBytesWritten);
					if(f_res != FR_OK)
						xil_printf("3 write fail WF\n");
				}

				//check for SOH
				CheckForSOH(&Iic, Uart_PS);
				//check for user input (check for BREAK)
				status = ReadCommandType(RecvBuffer, &Uart_PS);
				switch(status)
				{
				case -1:
					//We received a bad command
					done = 0;
					reportFailure(Uart_PS);
					break;
				case BREAK_CMD:
					//received the command to BREAK out of this loop and stop taking WF data
					done = 1;
					break;
				default:
					//other commands will not generate a response
					break;
				}
				if(done == 1)
					break;
			} //end of While(numWFs < #)

			f_close(&WFData);

			//change directories back to the root directory
			f_res = f_chdir("0:/");
			if(f_res != FR_OK)
			{
				//TODO: handle change directory fail
			}

			Xil_Out32(XPAR_AXI_GPIO_6_BASEADDR, 0);		//disable ADC
			Xil_Out32 (XPAR_AXI_GPIO_7_BASEADDR, 0);	//disable 5V to analog board

			if(done == 1)
				reportFailure(Uart_PS);
			else
				reportSuccess(Uart_PS, 0);

			SetModeByte(MODE_STANDBY);
			SetIDNumber(0);
			SetRunNumber(0);
			break;
		case READ_TMP_CMD:
			//tell the report_SOH function that we want a temp packet
			status = report_SOH(&Iic, GetLocalTime(), Uart_PS, READ_TMP_CMD);
			if(status == CMD_FAILURE)
				reportFailure(Uart_PS);
			break;
		case GETSTAT_CMD: //Push an SOH packet to the bus
			//instead of checking for SOH, just push one SOH packet out because it was requested
			status = report_SOH(&Iic, GetLocalTime(), Uart_PS, GETSTAT_CMD);
			if(status == CMD_FAILURE)
				reportFailure(Uart_PS);
			break;
		case DISABLE_ACT_CMD:
			//disable the components
			Xil_Out32(XPAR_AXI_GPIO_7_BASEADDR, 0);		//disable 5v to Analog board
			//No SW check on success/failure
			reportSuccess(Uart_PS, 0);
			break;
		case ENABLE_ACT_CMD:
			//enable the active components
			Xil_Out32(XPAR_AXI_GPIO_7_BASEADDR, 1);		//enable 5V to analog board
			//No SW check on success/failure
			reportSuccess(Uart_PS, 0);
			break;
		case TX_CMD:
			//transfer any file on the SD card
//			status = TransferSDFile( Uart_PS, TX_CMD );
//			TransferSDFile( XUartPs Uart_PS, char * RecvBuffer, int file_type, int id_num, int run_num,  int set_num );
			//switch on the file type
			switch(GetIntParam(1))
			{
			case DATA_TYPE_EVT:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_EVT, GetIntParam(2), GetIntParam(3), GetIntParam(4));
				break;
			case DATA_TYPE_WAV:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_WAV, GetIntParam(2), 0, 0);	//Run, set numbers always 0 for WAV
				break;
			case DATA_TYPE_CPS:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_CPS, GetIntParam(2), GetIntParam(3), 0);	//set number always 0 for CPS
				break;
			case DATA_TYPE_2DH_0:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_2DH_0, GetIntParam(2), GetIntParam(3), 0);	//set number always 0 for 2DH
				break;
			case DATA_TYPE_2DH_1:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_2DH_1, GetIntParam(2), GetIntParam(3), 0);	//set number always 0 for 2DH
				break;
			case DATA_TYPE_2DH_2:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_2DH_2, GetIntParam(2), GetIntParam(3), 0);	//set number always 0 for 2DH
				break;
			case DATA_TYPE_2DH_3:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_2DH_3, GetIntParam(2), GetIntParam(3), 0);	//set number always 0 for 2DH
				break;
			case DATA_TYPE_LOG:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_LOG, 0, 0, 0);
				break;
			case DATA_TYPE_CFG:
				status = TransferSDFile(Uart_PS, RecvBuffer, DATA_TYPE_CFG, 0, 0, 0);
				break;
			default:
				status = 1;	//failed to get data type
				break;
			}

			//need to put some space here to make sure that the UART is done sending before we try and send anything else
			//how much space do I need? Can I just check to see if the UART is done sending and then leave?
			while(XUartPs_IsSending(&Uart_PS))
			{
				//wait here
			}

			if(status != 0)
				reportFailure(Uart_PS);
			break;
		case DEL_CMD:
			//delete a file from the SD card
			SetModeByte(MODE_TRANSFER);
			status = DeleteFile(Uart_PS, RecvBuffer, GetIntParam(1), GetIntParam(2), GetIntParam(3), GetIntParam(4), GetIntParam(5));
			if(status == 0)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			SetModeByte(MODE_STANDBY);
			break;
		case DIR_CMD:
			SetModeByte(MODE_TRANSFER);
			SDInitDIR();
			memset(dir_packet_buffer, 0, sizeof(unsigned char) * TELEMETRY_MAX_SIZE);
			bytes_written = snprintf(dir_sd_card_buffer, 10, "%d:", GetIntParam(1));
			if(bytes_written == 0 || bytes_written != 2)
				reportFailure(Uart_PS);
			//count the number of files/folders on the SD card
			f_res = SDCountFilesOnCard( dir_sd_card_buffer );
			if(f_res != FR_OK)
				reportFailure(Uart_PS);
			else
				SDUpdateFileCounts();
			//put the SD card number, most recent Real Time, Total Folders, and Total Files in the packet header
			SDCreateDIRHeader(dir_packet_buffer, GetIntParam(1));
			//transfer the names and sizes of the files on the SD card
			f_res = SDScanFilesOnCard(dir_sd_card_buffer, dir_packet_buffer, Uart_PS);	//we only want to read the entire Root directory for now
			if(f_res != FR_OK)
				reportFailure(Uart_PS);
			SetModeByte(MODE_STANDBY);
			break;
		case TXLOG_CMD:
			//transfer the system log file
			//Transfer options:
			// 0 = data product file
			// 1 = Log File
			// 2 = Config file
//			status = TransferSDFile( Uart_PS, TXLOG_CMD );
			if(status == 0)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case CONF_CMD:
			//transfer the configuration file
			//Transfer options:
			// 0 = data product file
			// 1 = Log File
			// 2 = Config file
//			status = TransferSDFile( Uart_PS, CONF_CMD );
			if(status == 0)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case TRG_CMD:
			//set the trigger threshold
			status = SetTriggerThreshold( GetIntParam(1) );
			//Determine SUCCESS or FAILURE
			if(status)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case ECAL_CMD:
			//set the energy calibration parameters
			status = SetEnergyCalParam( GetFloatParam(1), GetFloatParam(2) );
			//Determine SUCCESS or FAILURE
			if(status)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case NGATES_CMD:
			//set the neutron cuts
			status = SetNeutronCutGates(GetIntParam(1), GetIntParam(2), GetFloatParam(1), GetFloatParam(2), GetFloatParam(3), GetFloatParam(4) );
			//Determine SUCCESS or FAILURE
			if(status)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case HV_CMD:
			//set the PMT bias voltage for one or more PMTs
			//intParam1 = PMT ID
			//intParam2 = Bias Voltage (taps)
			status = SetHighVoltage(&Iic, GetIntParam(1), GetIntParam(2));
			//Determine SUCCESS or FAILURE
			if(status)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case INT_CMD:
			//set the integration times
			//intParam1 = Baseline integration time
			//intParam2 = Short integration time
			//intParam3 = Long integration time
			//intParam4 = Full integration time
			status = SetIntegrationTime(GetIntParam(1), GetIntParam(2), GetIntParam(3), GetIntParam(4));
			//Determine SUCCESS or FAILURE
			if(status)
				reportSuccess(Uart_PS, 0);
			else
				reportFailure(Uart_PS);
			break;
		case INPUT_OVERFLOW:
			//too much input
			//TODO: Handle this problem here and in ReadCommandType
			reportFailure(Uart_PS);
			break;
		default:
			//got a value for menusel we did not expect
			//valid things can come here: Break, End, Start
			//Report CMD_FAILURE
			break;
		}//END OF SWITCH/CASE (MAIN MENU OF FUNCTIONS)

		//check to see if it is time to report SOH information, 1 Hz
		//this may help with functions which take too long during their own loops
//		CheckForSOH(&Iic, Uart_PS);
	}//END OF OUTER LEVEL 2 TESTING LOOP

    return 0;
}

//////////////////////////// InitializeAXIDma////////////////////////////////
// Sets up the AXI DMA
int InitializeAXIDma(void) {
	u32 tmpVal_0 = 0;

	tmpVal_0 = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);

	tmpVal_0 = tmpVal_0 | 0x1001; //<allow DMA to produce interrupts> 0 0 <run/stop>

	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x30, tmpVal_0);
	Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);

	tmpVal_0 = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);

	return 0;
}
//////////////////////////// InitializeAXIDma////////////////////////////////

//////////////////////////// InitializeInterruptSystem////////////////////////////////
int InitializeInterruptSystem(u16 deviceID) {
	int Status;

	GicConfig = XScuGic_LookupConfig (deviceID);
	if(NULL == GicConfig) {

		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;

	}

	Status = SetUpInterruptSystem(&InterruptController);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;

	}

	Status = XScuGic_Connect (&InterruptController,
			XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR,
			(Xil_ExceptionHandler) InterruptHandler, NULL);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;

	}

	XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR );

	return XST_SUCCESS;

}
//////////////////////////// InitializeInterruptSystem////////////////////////////////


//////////////////////////// Interrupt Handler////////////////////////////////
/*
 * This function is called when the system issues an interrupt. Currently, an interrupt is
 *  issued(generated?) when the DMA transfer is completed. When that happens, we need to clear a bit
 *  which is set by the completion of that transfer so that we may acknowledge and clear
 *  the interrupt. If that does not happen, the system will hang.
 *
 *  We have the DMA in Direct Register Mode with Interrupt on Complete enabled.
 *  This means that an interrupt is generated on the completion of a transfer.
 *  The interrupt handler writes to the DMA status register to clear the
 */
void InterruptHandler (void ) {
	u32 tmpValue = 0;
	tmpValue = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x34);	//Read the DMA status register
	tmpValue |= 0x1000;							//bit 12 is write-to-clear, this acknowledges the interrupt generated by IOC
	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x34, tmpValue);	//write to the DMA status register


}
//////////////////////////// Interrupt Handler////////////////////////////////


//////////////////////////// SetUp Interrupt System////////////////////////////////
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr) {
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, XScuGicInstancePtr);
	Xil_ExceptionEnable();
	return XST_SUCCESS;

}
//////////////////////////// SetUp Interrupt System////////////////////////////////
