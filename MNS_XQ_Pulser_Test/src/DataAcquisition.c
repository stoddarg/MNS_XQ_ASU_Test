/*
 * DataAcquisition.c
 *
 *  Created on: Dec 4, 2018
 *      Author: IRDLab
 */

#include "DataAcquisition.h"

//FILE SCOPE VARIABLES
static char current_run_folder[100];
static char current_filename_EVT[100];
static char current_filename_CPS[100];
static char current_filename_2DH_0[100];
static char current_filename_2DH_1[100];
static char current_filename_2DH_2[100];
static char current_filename_2DH_3[100];
static char current_filename_WAV[100];
static unsigned int daq_run_id_number;
static unsigned int daq_run_run_number;
static unsigned int daq_run_set_number;

static FIL m_EVT_file;
static FIL m_CPS_file;
static FIL m_2DH_file;



static DATA_FILE_HEADER_TYPE file_header_to_write;	//320 bytes
static DATA_FILE_SECONDARY_HEADER_TYPE file_secondary_header_to_write;	//16 bytes
static DATA_FILE_FOOTER_TYPE file_footer_to_write;	//20 bytes

/*
 * Getter function to get the folder name for the DAQ run which has been started.
 * We need to let the user know what the internally tracked value of the RUN number is
 *  so that they can request a specific run's files. The way we can do that is we
 *  return the ID number and run number used at the beginning of a run in a SUCCESS packet.
 *
 *  @param	None
 *
 *  @return	(char *)Pointer to the name of the folder that is being used for the run.
 *  				Note that the name is appended with the ROOT directory (0:)
 */
char *GetFolderName( void )
{
	return current_run_folder;
}

/*
 * Getter function to get the folder name size for the DAQ run which has been started.
 *
 *  @param	None
 *
 *  @return	(int)Number of bytes in the folder name string
 */
int GetFolderNameSize( void )
{
	return (int)strlen(current_run_folder);
}

/* Getter function for the current data acquisition filename string
 *
 * Each function and file name will have to be assembled from this string
 * which will be composed of the following parts:
 *
 * IDNum_RunNum_TYPE.bin
 *
 * ID Number 	= user input value which is the first unique value
 * Run Number 	= Mini-NS tracked value which counts how many runs have been made since the last POR
 * TYPE			= EVTS	-> event-by-event data product
 * 				= CPS 	-> counts-per-second data product
 * 				= WAV	-> waveform data product
 * 				= 2DH	-> two-dimensional histogram data product
 *
 * @param	None
 *
 * @return	Pointer to the buffer holding the filename.
 *
 */
char *GetFileName( int file_type )
{
	char * current_filename;

	switch(file_type)
	{
	case DATA_TYPE_EVT:
		current_filename = current_filename_EVT;
		break;
	case DATA_TYPE_WAV:
		current_filename = current_filename_WAV;
		break;
	case DATA_TYPE_CPS:
		current_filename = current_filename_CPS;
		break;
	case DATA_TYPE_2DH_0:
		current_filename = current_filename_2DH_0;
		break;
	case DATA_TYPE_2DH_1:
		current_filename = current_filename_2DH_1;
		break;
	case DATA_TYPE_2DH_2:
		current_filename = current_filename_2DH_2;
		break;
	case DATA_TYPE_2DH_3:
		current_filename = current_filename_2DH_3;
		break;
	default:
		current_filename = NULL;
		break;
	}

	return current_filename;
}

/*
 * Getter function for the size of the filenames which are assembled by the system
 * This function doesn't need a file type because the filenames were designed to
 * have the same length.
 *
 * @param	None
 *
 * @return	The number of bytes in the length of the filename string
 */
int GetFileNameSize( void )
{
	return (int)strlen(current_filename_EVT);
}

/*
 * Getter function for the current DAQ run ID number.
 * This value is provided by the user and stored.
 *
 * @param	None
 *
 * @return	The ID number provided to the MNS_DAQ command from the most recent/current run
 */
unsigned int GetDAQRunIDNumber( void )
{
	return daq_run_id_number;
}

/*
 * Getter function for the current DAQ run RUN number.
 * This value is calculated by the system and stored for internal use and in creating
 *  unique filenames for the data products being stored to the SD card.
 * This value is zeroed out each time the Mini-NS power cycles.
 * This value is incremented each time the Mini-NS begins a new DAQ run.
 *
 * @param	None
 *
 * @return	The number of DAQ runs since the system power cycled last, the RUN number
 */
unsigned int GetDAQRunRUNNumber( void )
{
	return daq_run_run_number;
}

/*
 * Getter function for the current DAQ run SET number.
 * As the Mini-NS is collecting data, the EVTS data product files will become quite large. To
 *  mitigate the problem of having to downlink very large files with limited bandwidth, the
 *  system will close a file which exceeds ~1MB in size. It will then open a file with the same
 *  filename, except the SET number will be incremented by 1. It will continue recording data in
 *  that file for the run.
 * This value is zeroed out for each DAQ run.
 * This value is incremented each time the Mini-NS closes a data product file to start a new one.
 *
 * @param	None
 *
 * @return	The number of DAQ runs since the system power cycled last, the RUN number
 */
unsigned int GetDAQRunSETNumber( void )
{
	return daq_run_set_number;
}

/*
 * Create the file names for the various data products which will be created when we start a DAQ run.
 * This will create the file name strings for CPS, EVT, and 2DH data products. There is one file for
 *  both CPS and EVT data, but 2DH has 4 four files which are created.
 *
 *  @param	(integer) ID number for the run, this comes from the spacecraft
 *  @param	(integer) Run number, is internally generated and tracked from run-to-run
 *  @param	(integer) Set number, this will get incremented each time that the MNS changes file mid-DAQ
 *
 *  @return	(integer) CMD_SUCCESS/CMD_FAILURE
 */
int SetFileName( int ID_number, int run_number, int set_number )
{
	int status = CMD_SUCCESS;
	int bytes_written = 0;

	//save the values so we can access them later, we can put them in the file headers
	daq_run_id_number = ID_number;
	daq_run_run_number = run_number;
	daq_run_set_number = set_number;

	//create a folder for the run //0:/I0000_R0001/
	bytes_written = snprintf(current_run_folder, 100, "0:/I%04d_R%04d", ID_number, run_number);
	if(bytes_written == 0 || bytes_written != ROOT_DIR_NAME_SIZE + DAQ_FOLDER_SIZE)
		status = CMD_FAILURE;

	bytes_written = snprintf(current_filename_EVT, 100, "evt_S%04d.bin", set_number);
	if(bytes_written == 0)
		status = CMD_FAILURE;

	bytes_written = snprintf(current_filename_CPS, 100, "cps.bin");
	if(bytes_written == 0)
		status = CMD_FAILURE;

	bytes_written = snprintf(current_filename_2DH_0, 100, "2d0.bin");
	if(bytes_written == 0)
		status = CMD_FAILURE;

	bytes_written = snprintf(current_filename_2DH_1, 100, "2d1.bin");
	if(bytes_written == 0)
		status = CMD_FAILURE;

	bytes_written = snprintf(current_filename_2DH_2, 100, "2d2.bin");
	if(bytes_written == 0)
		status = CMD_FAILURE;

	bytes_written = snprintf(current_filename_2DH_3, 100, "2d3.bin");
	if(bytes_written == 0)
		status = CMD_FAILURE;

	return status;
}

/*
 * Check if the folder for the run exists already or not
 * 		returns FALSE if folder does NOT exist
 *		returns TRUE if folder does exist
 *	This function tells our loop in main whether or not to generate another folder
 *	 to put the current run into. If the folder already exists, then we'll try the
 *	 next run number after the current one. We'll loop until we get to a combination
 *	 of the ID number requested by the user and the run number which is unique.
 *
 *	 @param		none
 *
 *	 @return	CMD_FAILURE if the folder does not exist
 *	 			CMD_SUCCESS if the folder does exist
 */
int DoesFileExist( void )
{
	int status = CMD_SUCCESS;
	FILINFO fno;		//file info structure			//TESTING 6-14-19
	//Initialize the FILINFO struct with something //If using LFN, then we need to init these values, otherwise we don't
	TCHAR LFName[256];
	fno.lfname = LFName;
	fno.lfsize = sizeof(LFName);
	FRESULT ffs_res;	//FAT file system return type	//TESTING 6-14-19

	//check the SD card for the folder
	ffs_res = f_stat(current_run_folder, &fno);
	if(ffs_res == FR_NO_FILE)
		status = CMD_FAILURE;
	else if(ffs_res == FR_NO_PATH)
		status = CMD_FAILURE;
	else
		status = CMD_SUCCESS;


//	else if(ffs_res == FR_INVALID_NAME)
//		status = CMD_ERROR;
//	else
//		status = CMD_ERROR;
	//TODO: can't return CMD_ERROR yet because we don't handle the return from this function other than success/failure
	// handle this here and at the return

	return status;
}

/* Creates the data acquisition files for the run requested by the DAQ command.
 * Uses the filenames which are created from the ID number sent with the DAQ
 *  command to open and write the header into the files.
 * The FIL pointers are left open by this function intentionally so that DAQ doesn't
 *  have to spend the time opening them.
 *NOTE: Upon successful completion, this function leaves the SD card in the directory
 *		 which was created. The user needs to change directories before doing other
 *		 file operations.
 *
 * @param	None
 *
 * @return	Success/failure based on how we finished the run:
 * 			BREAK (0)	 = failure
 * 			Time Out (1) = success
 * 			END (2)		 = success
 */
int CreateDAQFiles( void )
{
	char * file_to_open = NULL;
	int iter = 0;
	int status = CMD_SUCCESS;
	uint NumBytesWr;
	FIL *DAQ_file = NULL;
	FRESULT ffs_res;

	//a blank struct to write into the CPS file //reserves space for later
	DATA_FILE_SECONDARY_HEADER_TYPE blank_file_secondary_header_to_write = {}; //i'm assuming that we can write space even if it's blank

	//gather the header information
	//TODO: check the return was not NULL?
	file_header_to_write.configBuff = *GetConfigBuffer();	//dereference to copy the struct into our local struct
	file_header_to_write.IDNum = daq_run_id_number;
	file_header_to_write.RunNum = daq_run_run_number;
	file_header_to_write.SetNum = daq_run_set_number;
	file_header_to_write.TempCorrectionSetNum = 1;		//will have to get this from somewhere
	file_header_to_write.EventID1 = 0xFF;
	file_header_to_write.EventID2 = 0xFF;

	//open the run folder so we can create the files there
	ffs_res = f_mkdir(current_run_folder);
	if(ffs_res != FR_OK)
	{
		//TODO: handle folder creation fail
	}
	else
	{
		ffs_res = f_chdir(current_run_folder);
		if(ffs_res != FR_OK)
		{
			//TODO: handle change directory fail
		}

		//once we create the folder and change to it, update the number of folders
		sd_totalFoldersIncrement();
		//create the TX_bytes file to record the individual files
//		sd_createTXBytesFile();	//comment 12-16-2019
	}

	for(iter = 0; iter < 6; iter++)
	{
		switch(iter)
		{
		case 0:
			file_to_open = current_filename_EVT;
			file_header_to_write.FileTypeAPID = DATA_TYPE_EVT;
			DAQ_file = &m_EVT_file;
			break;
		case 1:
			file_to_open = current_filename_CPS;
			file_header_to_write.FileTypeAPID = DATA_TYPE_CPS;
			DAQ_file = &m_CPS_file;
			break;
		case 2:
			file_to_open = current_filename_2DH_0;
			file_header_to_write.FileTypeAPID = DATA_TYPE_2DH_0;
			DAQ_file = &m_2DH_file;
			break;
		case 3:
			file_to_open = current_filename_2DH_1;
			file_header_to_write.FileTypeAPID = DATA_TYPE_2DH_1;
			DAQ_file = &m_2DH_file;
			break;
		case 4:
			file_to_open = current_filename_2DH_2;
			file_header_to_write.FileTypeAPID = DATA_TYPE_2DH_2;
			DAQ_file = &m_2DH_file;
			break;
		case 5:
			file_to_open = current_filename_2DH_3;
			file_header_to_write.FileTypeAPID = DATA_TYPE_2DH_3;
			DAQ_file = &m_2DH_file;
			break;
		default:
			status = CMD_FAILURE;
			break;
		}

		//TODO: do we need to check to see if any of the FILs are NULL? //they should be automatically created when the program starts, but...good practice to check them
		ffs_res = f_open(DAQ_file, file_to_open, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
		if(ffs_res == FR_OK)
		{
			ffs_res = f_lseek(DAQ_file, 0);
			if(ffs_res == FR_OK)
			{
				ffs_res = f_write(DAQ_file, &file_header_to_write, sizeof(file_header_to_write), &NumBytesWr);
				if(ffs_res == FR_OK && NumBytesWr == sizeof(file_header_to_write))
				{
					if(iter == 1)	//this is for CPS files only
					{
						ffs_res = f_write(DAQ_file, &blank_file_secondary_header_to_write, sizeof(blank_file_secondary_header_to_write), &NumBytesWr);
						if(ffs_res == FR_OK)
							status = CMD_SUCCESS;
						else
							status = CMD_FAILURE;

						//record the new file information in the tx_bytes file
//						sd_updateFileRecords(file_to_open, file_size(DAQ_file));
					}
					if(iter < 2)	//sync the EVT, CPS files; we're leaving them open during the run
					{
						ffs_res = f_sync(DAQ_file);
						if(ffs_res == FR_OK)
							status = CMD_SUCCESS;
						else
							status = CMD_FAILURE;

						//record the new file information in the tx_bytes file
//						sd_updateFileRecords(file_to_open, file_size(DAQ_file));
					}
					else
					{
						//record the new file information in the tx_bytes file
//						sd_updateFileRecords(file_to_open, file_size(DAQ_file));

						f_close(DAQ_file);
						status = CMD_SUCCESS;
					}

					//record that we have created a new file
					sd_totalFilesIncrement();
				}
				else
					status = CMD_FAILURE;
			}
			else
				status = CMD_FAILURE;
		}
		else
			status = CMD_FAILURE;
	}

	//TODO: if we have a failure, check the FIL pointers and make sure they're closed safely
	// This should avoid messing up the SD card with improperly closed files.

	return status;
}


FIL *GetEVTFilePointer( void )
{
	return &m_EVT_file;
}

FIL *GetCPSFilePointer( void )
{
	return &m_CPS_file;
}

int WriteRealTime( unsigned long long int real_time )
{
	int status = CMD_SUCCESS;
//	uint NumBytesWr;
//	FRESULT F_RetVal;
//	FILINFO CnfFno;
//	FIL ConfigFile;
//	int RetVal = 0;
//	int ConfigSize = sizeof(ConfigBuff);
//
//	//take the config buffer and put it into each data product file
//	// check that data product file exists
//	if( f_stat( cConfigFile, &CnfFno) )	//f_stat returns non-zero (true) if no file exists, so open/create the file
//	{
//		F_RetVal = f_open(&ConfigFile, cConfigFile, FA_WRITE|FA_OPEN_ALWAYS);
//		if(F_RetVal == FR_OK)
//			F_RetVal = f_write(&ConfigFile, &ConfigBuff, ConfigSize, &NumBytesWr);
//		if(F_RetVal == FR_OK)
//			F_RetVal = f_close(&ConfigFile);
//	}
//	else // If the file exists, write it
//	{
//		F_RetVal = f_open(&ConfigFile, cConfigFile, FA_READ|FA_WRITE);	//open with read/write access
//		if(F_RetVal == FR_OK)
//			F_RetVal = f_lseek(&ConfigFile, 0);							//go to beginning of file
//		if(F_RetVal == FR_OK)
//			F_RetVal = f_write(&ConfigFile, &ConfigBuff, ConfigSize, &NumBytesWr);	//Write the ConfigBuff to config file
//		if(F_RetVal == FR_OK)
//			F_RetVal = f_close(&ConfigFile);							//close the file
//		}
//
//	RetVal = (int)F_RetVal;
//    return RetVal;

	return status;
}

//Clears the BRAM buffers
// I need to refresh myself as to why this is important
// All that I remember is that it's important to do before each DRAM transfer
//Resets which buffer we are reading from
// issuing this "clear" allows us to move to the next buffer to read from it
//Tells the FPGA, we are done with this buffer, read from the next one
void ClearBRAMBuffers( void )
{
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,1);
	usleep(1);
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,0);
}

void DAQReadDataIn( unsigned int *raw_array, int buffer_number )
{
	unsigned int dram_addr = DRAM_BASE;
	int array_index = DATA_BUFFER_SIZE * buffer_number;

	while(dram_addr < DRAM_CEILING)
	{
		raw_array[array_index] = Xil_In32(dram_addr);
		dram_addr += 4;
		array_index++;
	}

	return;
}

/* What it's all about.
 * The main event.
 * This is where we interact with the FPGA to receive data,
 *  then process and save it. We are reporting SOH and various SUCCESS/FAILURE packets along
 *  the way.
 *
 * @param	(XIicPs *) Pointer to Iic instance (for read temp while in DAQ)
 *
 * @param	(XUartPs) UART instance for reporting SOH
 *
 * @param	(char *) Pointer to the receive buffer for getting user input
 *
 * @param	(integer) Time out value indicating when to break out of DAQ in minutes
 * 			Ex. 1 = loop for 1 minute
 *
 *
 * @return	Success/failure based on how we finished the run:
 * 			BREAK (0)	 = failure
 * 			Time Out (1) = success
 * 			END (2)		 = success
 */
int DataAcquisition( XIicPs * Iic, XUartPs Uart_PS, char * RecvBuffer, int time_out )
{
	//initialize variables
	int done = 0;					//local status variable for keeping track of progress within loops
	int status = CMD_SUCCESS;		//monitors the status of how we break out of DAQ
	int status_SOH = CMD_SUCCESS;	//local status variable
	int poll_val = 0;				//local polling status variable
	int valid_data = 0;				//goes high/low if there is valid data within the FPGA buffers
	int buff_num = 0;				//keep track of which buffer we are writing
	int buff_offset = 0;			//keep track of the buffer offset
	int m_buffers_written = 0;		//keep track of how many buffers are written, but not synced
//	int array_index = 0;			//the index of our array which will hold data
//	int dram_addr = 0;				//the address in the DRAM we are reading from
//	int dram_base = DRAM_BASE;		//where the buffer starts	//0x0A 00 00 00 = 167,772,160 //0x0A 00 40 00 = 167,788,544 - 167,788,544 = 16384
	int m_run_time = time_out * 60;	//multiply minutes by 60 to get seconds
	int m_write_header = 1;			//write a file header the first time we use a file
	XTime m_run_start; 				//timing variable
	XTime_GetTime(&m_run_start);	//record the "start" time to base a time out on
	XTime m_run_current_time;		//timing variable
	char m_write_blank_space_buff[16384] = "";
	unsigned int bytes_written = 0;
	FRESULT f_res = FR_OK;
	GENERAL_EVENT_TYPE * evts_array = NULL;
	//Data buffer which can hold 4096*4 integers, each buffer holds 512 8-integer events, x4 for four buffers
	unsigned int data_array[DATA_BUFFER_SIZE * 4];
	//if this doesn't work try allocating the array dynamically and try to see if the access time goes down
//	buffers are 4096 ints long (512 events total)
//	unsigned int *data_array;
//	data_array = (unsigned int *)malloc(sizeof(unsigned int) * DATA_BUFFER_SIZE * 4);
//	memset(data_array, '\0',  sizeof(unsigned int) * DATA_BUFFER_SIZE * 4);



	//timing variables //delete when not timing
//	XTime tBegin = 0;
//	XTime tStart = 0;
//	XTime tEnd = 0;

	memset(&m_write_blank_space_buff, 186, 16384);

	ResetEVTsBuffer();
	ResetEVTsIterator();

	SetModeByte(MODE_DAQ);

	//create file to save the raw integers to
#ifdef PRODUCE_RAW_DATA
	FIL m_raw_data_file;
	f_res = f_open(&m_raw_data_file, "raw_data.bin", FA_OPEN_ALWAYS|FA_READ|FA_WRITE);
#endif

	while(done != 1)
	{
		valid_data = Xil_In32 (XPAR_AXI_GPIO_11_BASEADDR);
		if(valid_data == 1)
		{
//**************//Start timing here for tracking the latency
//			XTime_GetTime(&tBegin);
//			XTime_GetTime(&tStart);

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

//			XTime_GetTime(&tEnd);
//			printf("DMA Transfer took %.2f us\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
//*************//Time just the read-in loop

//			dram_addr = dram_base;
//			array_index = DATA_BUFFER_SIZE * buff_num;
//			while(dram_addr < DRAM_CEILING)
//			{
//				data_array[array_index] = Xil_In32(dram_addr);
//				dram_addr += 4;
//				array_index++;
//			}

//**************//here is where we just finished the DMA transfer between the FPGA and the processor
//			XTime_GetTime(&tStart);
			DAQReadDataIn( data_array, buff_num);

//			XTime_GetTime(&tEnd);
//			printf("Read-in loop took %.2f us\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
//*************//Time just the read-in loop


//*************//Time just the process data loop
//			XTime_GetTime(&tStart);

			buff_offset = DATA_BUFFER_SIZE * buff_num;
			status_SOH = ProcessData( &data_array[buff_offset] );
			buff_num++;

//			XTime_GetTime(&tEnd);
//			printf("ProcessData loop took %.2f us\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
//*************//End of timing just the process data loop

			if(buff_num == 4)	//keep this to every 4th buffer, that's what it's set up for...
			{

//*************//Time the writing to SD card part of the loop
//				XTime_GetTime(&tStart);

				buff_num = 0;

#ifdef PRODUCE_RAW_DATA
				f_res = f_write(&m_raw_data_file, &data_array, DATA_BUFFER_SIZE * 4 * 4, &bytes_written);
				if(f_res != FR_OK || bytes_written != DATA_BUFFER_SIZE * 4 * 4)
					status = CMD_FAILURE;

				f_res = f_sync(&m_raw_data_file);
				if(f_res != FR_OK)
				{
					//TODO: error check
					xil_printf("8 error syncing DAQ\n");
				}
#endif

				//check the file size and see if we need to change files
				if(m_EVT_file.fsize >= SIZE_10_MIB)
				{
					//prepare and write in footer for file here
					file_footer_to_write.digiTemp = GetDigiTemp();
					f_res = f_write(&m_EVT_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
					if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
						status = CMD_FAILURE;

					f_close(&m_EVT_file);
					//create the new file name (increment the set number)
					daq_run_set_number++; file_header_to_write.SetNum = daq_run_set_number;
					file_header_to_write.FileTypeAPID = DATA_TYPE_EVT;	//change back to EVTS
					bytes_written = snprintf(current_filename_EVT, 100, "evt_S%04d.bin", daq_run_set_number);
					if(bytes_written == 0)
						status = CMD_FAILURE;
					f_res = f_open(&m_EVT_file, current_filename_EVT, FA_OPEN_ALWAYS|FA_READ|FA_WRITE);
					if(f_res == FR_OK)
					{
						f_res = f_lseek(&m_EVT_file, 0);
						if(f_res != FR_OK)
							status = CMD_FAILURE;
						//write file header
						f_res = f_write(&m_EVT_file, &file_header_to_write, sizeof(file_header_to_write), &bytes_written);
						if(f_res != FR_OK || bytes_written != sizeof(file_header_to_write))
							status = CMD_FAILURE;
						//write secondary header
						f_res = f_write(&m_EVT_file, &file_secondary_header_to_write, sizeof(file_secondary_header_to_write), &bytes_written);
						if(f_res != FR_OK || bytes_written != sizeof(file_secondary_header_to_write))
							status = CMD_FAILURE;
						//write blank bytes up to Cluster edge (16384
						f_res = f_write(&m_EVT_file, m_write_blank_space_buff, 16384 - file_size(&m_EVT_file), &bytes_written);
						if(f_res != FR_OK || bytes_written != sizeof(file_secondary_header_to_write))
							status = CMD_FAILURE;
						//record that we have created a new file
						sd_totalFilesIncrement();
						//record the new file information in the tx_bytes file
//						sd_updateFileRecords(current_filename_EVT, file_size(&m_EVT_file));
					}
					else
						status = CMD_FAILURE;
					//update any more data structures?
				}

				if(m_write_header == 1)
				{
					//get the first event and the real time
					file_secondary_header_to_write.RealTime = GetRealTimeParam();
					file_secondary_header_to_write.EventID1 = 0xFF;
					file_secondary_header_to_write.EventID2 = 0xEE;
					file_secondary_header_to_write.EventID3 = 0xDD;
					file_secondary_header_to_write.EventID4 = 0xCC;
					file_secondary_header_to_write.FirstEventTime = GetFirstEventTime();
					file_secondary_header_to_write.EventID5 = 0xCC;
					file_secondary_header_to_write.EventID6 = 0xDD;
					file_secondary_header_to_write.EventID7 = 0xEE;
					file_secondary_header_to_write.EventID8 = 0xFF;

					f_res = f_write(&m_EVT_file, &file_secondary_header_to_write, sizeof(file_secondary_header_to_write), &bytes_written);
					if(f_res != FR_OK || bytes_written != sizeof(file_secondary_header_to_write))
					{
						//TODO: handle error checking the write
						xil_printf("10 error writing DAQ\n");
					}
					//write blank bytes up to Cluster edge (16384)
					f_res = f_write(&m_EVT_file, m_write_blank_space_buff, 16384 - file_size(&m_EVT_file), &bytes_written);
					if(f_res != FR_OK || bytes_written != sizeof(file_secondary_header_to_write))
						status = CMD_FAILURE;

//					sd_updateFileRecords(current_filename_EVT, file_size(&m_EVT_file));

					f_res = f_lseek(&m_CPS_file, sizeof(file_header_to_write));	//want to move to the reserved space we allocated before the run, directly after header
					f_res = f_write(&m_CPS_file, &file_secondary_header_to_write, sizeof(file_secondary_header_to_write), &bytes_written);
					if(f_res != FR_OK || bytes_written != sizeof(file_secondary_header_to_write))
					{
						//TODO: handle error checking the write
						xil_printf("10 error writing DAQ\n");
					}
//					sd_updateFileRecords(current_filename_CPS, file_size(&m_CPS_file));

					f_res = f_lseek(&m_CPS_file, file_size(&m_CPS_file));	//forward the file pointer so we're at the top of the file again

					//also write the footer information that isn't going to change //this way we only do it once
					file_footer_to_write.eventID1 = 0xFF;
					file_footer_to_write.eventID2 = 0x45;
					file_footer_to_write.eventID3 = 0x4E;
					file_footer_to_write.eventID4 = 0x44;
					file_footer_to_write.RealTime = GetRealTimeParam();
					file_footer_to_write.eventID5 = 0xFF;
					file_footer_to_write.eventID6 = 0x45;
					file_footer_to_write.eventID7 = 0x4E;
					file_footer_to_write.eventID8 = 0x44;
					file_footer_to_write.eventID9 = 0xFF;
					file_footer_to_write.eventID10 = 0x45;
					file_footer_to_write.eventID11 = 0x4E;
					file_footer_to_write.eventID12 = 0x44;
					m_write_header = 0;	//turn off header writing //never come back here
				}

				evts_array = GetEVTsBufferAddress();
				//TODO: check that the evts_array address is not NULL
				f_res = f_write(&m_EVT_file, evts_array, EVT_DATA_BUFF_SIZE, &bytes_written); //write the entire events buffer
				if(f_res != FR_OK || bytes_written != EVT_DATA_BUFF_SIZE)
				{
					//TODO: handle error checking the write here
					//now we need to check to make sure that there is a file open, if we get specific return values from f_write, need to check to see if we can open a file
					xil_printf("7 error writing DAQ\n");
				}
				m_buffers_written++;
				if(f_res == FR_OK && m_buffers_written == 4)
				{
					f_res = f_sync(&m_EVT_file);
					if(f_res != FR_OK)
					{
						//TODO: error check
						xil_printf("8 error syncing DAQ\n");
					}
					m_buffers_written = 0;	//reset
				}

//				sd_updateFileRecords(current_filename_EVT, file_size(&m_EVT_file));

				ResetEVTsBuffer();
				ResetEVTsIterator();

//****************//End timing of the loop here //we have finished processing and finished saving
//				XTime_GetTime(&tEnd);
//				printf("Write to SD loop took %.2f us\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
			}
			valid_data = 0;	//reset

//****************//End timing of the loop here //we have finished processing and finished saving
//			XTime_GetTime(&tEnd);
//			printf("Loop %d-%d took %.2f us\n", m_buffers_written, buff_num, 1.0 * (tEnd - tBegin) / (COUNTS_PER_SECOND/1000000));
		}//END OF IF VALID DATA

		//check to see if it is time to report SOH information, 1 Hz
		CheckForSOH(Iic, Uart_PS);	//disable SOH during DAQ so that it is easier to parse the timing output here //12-17-2019

		//check for timeout
		XTime_GetTime(&m_run_current_time);
		if(((m_run_current_time - m_run_start)/COUNTS_PER_SECOND) >= m_run_time)
		{
			file_footer_to_write.digiTemp = GetDigiTemp();
			//just keeping the Real Time from the space craft as the RealTime value
			f_res = f_write(&m_EVT_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
			if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
				status = CMD_FAILURE;
//			sd_updateFileRecords(current_filename_EVT, file_size(&m_EVT_file));
			f_res = f_write(&m_CPS_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
			if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
				status = CMD_FAILURE;
//			sd_updateFileRecords(current_filename_CPS, file_size(&m_CPS_file));
			status = DAQ_TIME_OUT;
			done = 1;
		}

		poll_val = ReadCommandType(RecvBuffer, &Uart_PS);
		switch(poll_val)
		{
		case -1:
			//this is bad input or an error in input
			//no real need for a case if we aren't handling it
			//just leave this to default
			break;
		case READ_TMP_CMD:
			status_SOH = report_SOH(Iic, GetLocalTime(), Uart_PS, READ_TMP_CMD);
			if(status_SOH == CMD_FAILURE)
				reportFailure(Uart_PS);
			break;
		case BREAK_CMD:
			file_footer_to_write.digiTemp = GetDigiTemp();
			//have no END time to write here, so we use the START real time
			f_res = f_write(&m_EVT_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
			if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
				status = CMD_FAILURE;
//			sd_updateFileRecords(current_filename_EVT, file_size(&m_EVT_file));
			f_res = f_write(&m_CPS_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
			if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
				status = CMD_FAILURE;
//			sd_updateFileRecords(current_filename_CPS, file_size(&m_CPS_file));
			status = DAQ_BREAK;
			done = 1;
			break;
		case END_CMD:
			file_footer_to_write.RealTime = GetRealTimeParam();
			file_footer_to_write.digiTemp = GetDigiTemp();
			f_res = f_write(&m_EVT_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
			if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
				status = CMD_FAILURE;
//			sd_updateFileRecords(current_filename_EVT, file_size(&m_EVT_file));
			f_res = f_write(&m_CPS_file, &file_footer_to_write, sizeof(file_footer_to_write), &bytes_written);
			if(f_res != FR_OK || bytes_written != sizeof(file_footer_to_write))
				status = CMD_FAILURE;
//			sd_updateFileRecords(current_filename_CPS, file_size(&m_CPS_file));
			status = DAQ_END;
			done = 1;
			break;
		default:
			break;
		}
	}//END OF WHILE DONE != 1

	//here is where we should transfer the CPS, 2DH files?
	status_SOH = Save2DHToSD( PMT_ID_0 );
	if(status_SOH != CMD_SUCCESS)
		xil_printf("9 save sd 0 DAQ\n");
	status_SOH = Save2DHToSD( PMT_ID_1 );
	if(status_SOH != CMD_SUCCESS)
		xil_printf("10 save sd 1 DAQ\n");
	status_SOH = Save2DHToSD( PMT_ID_2 );
	if(status_SOH != CMD_SUCCESS)
		xil_printf("11 save sd 2 DAQ\n");
	status_SOH = Save2DHToSD( PMT_ID_3 );
	if(status_SOH != CMD_SUCCESS)
		xil_printf("12 save sd 3 DAQ\n");

	//TESTING 6-13-2019
#ifdef PRODUCE_RAW_DATA
//	sd_updateFileRecords("raw_data.bin", file_size(&m_raw_data_file));
	f_close(&m_raw_data_file);
#endif
	//TESTING 6-13-2019

	//cleanup operations
	//2DH files are closed by that module
	f_close(&m_EVT_file);
	f_close(&m_CPS_file);

	return status;
}
