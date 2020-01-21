/*
 * LogFileControl.c
 *
 *  Created on: Nov 6, 2018
 *      Author: gstoddard
 *
 * Code to handle operations using a command log file.
 * This source file and its associated header file will allow the user to
 * 	mount and initialize the log file and to write to it.
 *
 */

#include "LogFileControl.h"

//filenames for the log file
static char cLogFile0[] = "0:/MNSLOG.txt";	//The name of the log file on SD card 0
static char cLogFile1[] = "1:/MNSLOG.txt";	//The name of the log file on SD card 1

/*
 * Initialize the log file on SD0.
 *
 * Checks to make sure the SD card has been mounted, then checks to see
 * if there is an existing log file with the appropriate name.
 * If that file exists, access it and record that we power cycled the first time.
 * If that file does not exist, create it and record that we power cycled.
 *
 *
 */
int InitLogFile0( void )
{
	FIL logFile;
	FRESULT ffs_res;
	FILINFO fno;	//SD card information object

	char log_file_write_buffer[LOG_FILE_BUFF_SIZE] = "";
	unsigned int i_sprintf_ret = 0;
	unsigned int num_bytes_written = 0;
	int status = 0;

	/***** Handle SD0 First *****/
	// f_stat returns non-zero(true) if no file exists, so open/create the file
	// f_stat returns zero (false) if a file exists
	if( f_stat( cLogFile0, &fno) )
	{
		//Open and write to a new Log File
		ffs_res = f_open(&logFile, cLogFile0, FA_WRITE|FA_OPEN_ALWAYS);
		if(ffs_res == FR_OK)
		{
			i_sprintf_ret = snprintf(log_file_write_buffer, LOG_FILE_BUFF_SIZE, "FIRST POWER ON\n");
			ffs_res = f_write(&logFile, log_file_write_buffer, i_sprintf_ret, &num_bytes_written );
		}
		if(ffs_res == FR_OK)
			ffs_res = f_close(&logFile);
		if(ffs_res == FR_OK)
			status = CMD_SUCCESS;
		else
			status = CMD_FAILURE;
	}
	else //The Log File exists
	{
		//open with read/write access
		ffs_res = f_open(&logFile, cLogFile0, FA_READ|FA_WRITE);
		//move the read/write pointer so we may append the file
		if(ffs_res == FR_OK)
			ffs_res = f_lseek(&logFile, file_size(&logFile));
		if(ffs_res == FR_OK)
		{
			//write that the system was power cycled
			i_sprintf_ret = snprintf(log_file_write_buffer, LOG_FILE_BUFF_SIZE, "POWER RESET\n");
			ffs_res = f_write(&logFile, log_file_write_buffer, i_sprintf_ret, &num_bytes_written);
		}
		if(ffs_res == FR_OK)
			ffs_res = f_close(&logFile);
		if(ffs_res == FR_OK)
			status = CMD_SUCCESS;
		else
			status = CMD_FAILURE;
	}

	return status;
}

/*
 * Initialize the log file on SD1.
 *
 * Checks to make sure the SD card has been mounted, then checks to see
 * if there is an existing log file with the appropriate name.
 * If that file exists, access it and record that we power cycled the first time.
 * If that file does not exist, create it and record that we power cycled.
 *
 *
 */
int InitLogFile1( void )
{
	FIL logFile;
	FRESULT ffs_res;
	FILINFO fno;	//SD card information object

	char log_file_write_buffer[LOG_FILE_BUFF_SIZE] = "";
	unsigned int i_sprintf_ret = 0;
	unsigned int num_bytes_written = 0;
	int status = 0;

	// f_stat returns non-zero(true) if no file exists, so open/create the file
	// f_stat returns zero (false) if a file exists
	if( f_stat( cLogFile1, &fno) )
	{
		//Open and write to a new Log File
		ffs_res = f_open(&logFile, cLogFile1, FA_WRITE|FA_OPEN_ALWAYS);
		if(ffs_res == FR_OK)
		{
			//record that this is the first time we are writing into the log
			i_sprintf_ret = snprintf(log_file_write_buffer, LOG_FILE_BUFF_SIZE, "FIRST POWER ON\n");
			ffs_res = f_write(&logFile, log_file_write_buffer, i_sprintf_ret, &num_bytes_written );
		}
		if(ffs_res == FR_OK)
			ffs_res = f_close(&logFile);	//close the file
		if(ffs_res == FR_OK)
			status = CMD_SUCCESS;
		else
			status = CMD_FAILURE;
	}
	else //The Log File exists
	{
		//open with read/write access
		ffs_res = f_open(&logFile, cLogFile1, FA_READ|FA_WRITE);
		//move the read/write pointer so we may append the file
		if(ffs_res == FR_OK)
			ffs_res = f_lseek(&logFile, file_size(&logFile));
		if(ffs_res == FR_OK)
		{
			//write that the system was power cycled
			i_sprintf_ret = snprintf(log_file_write_buffer, LOG_FILE_BUFF_SIZE, "POWER RESET\n");
			ffs_res = f_write(&logFile, log_file_write_buffer, i_sprintf_ret, &num_bytes_written);
		}
		if(ffs_res == FR_OK)
			ffs_res = f_close(&logFile);
		if(ffs_res == FR_OK)
			status = CMD_SUCCESS;
		else
			status = CMD_FAILURE;
	}

	return status;
}

/*
 * Mount both of the SD Cards.
 *
 * This function may be run to unmount then re-mount the SD cards
 * This will run the same mounting for both SD cards on the board.
 * Check the return variable to determine whether or not the
 * operation succeeded.
 *
 * @param	A pointer to the FAT File System Object to be worked on
 *
 * @return	Command SUCCESS (0) or FAILURE (1)
 */
int MountSDCards( FATFS * fs )
{
	int status = 0;
	FRESULT ffs_res;

	//unmount SD card 0
	ffs_res = f_mount(NULL,"0:/",0);
	//mount SD card 0
	if(ffs_res == FR_OK)
		ffs_res = f_mount(&fs[0], "0:/",0);
	//unmount SD card 1
	if(ffs_res == FR_OK)
		ffs_res = f_mount(NULL,"1:/",0);
	//mount SD card 1
	if(ffs_res == FR_OK)
		ffs_res = f_mount(&fs[1],"1:/",0);
	//set a pseudo-global to indicate not to do this again
	//may want to do some better checking here to allow
	//users to re-mount the SD cards at a future time
	if(ffs_res == FR_OK)
		status = CMD_SUCCESS;
	else
	{
		status = CMD_FAILURE;
	}

	return status;
}

/*
 * Mount SD card 0.
 *
 * This function may be run to unmount then re-mount SD card 0
 *
 * Check the return variable to determine whether or not the
 * operation succeeded. If failure, how do we handle that?
 *
 * @param	A pointer to the FAT File System Object to be worked on
 *
 * @return	Command SUCCESS (0) or FAILURE (1)
 */
int MountSD0( FATFS * fs )
{
	int status = 0;
	FRESULT ffs_res;

	//unmount SD card 1
	ffs_res = f_mount(NULL,"1:/",0);
	//mount SD card 1
	if(ffs_res == FR_OK)
		ffs_res = f_mount(&fs[1],"1:/",0);
	//determine if we succeeded or failed
	if(ffs_res == FR_OK)
		status = CMD_SUCCESS;
	else
	{
		status = CMD_FAILURE;
	}

	return status;
}

/*
 * Mount SD card 1.
 *
 * This function may be run to unmount then re-mount SD card 1
 *
 * Check the return variable to determine whether or not the
 * operation succeeded. If failure, how do we handle that?
 *
 * @param	A pointer to the FAT File System Object to be worked on
 *
 * @return	Command SUCCESS (0) or FAILURE (1)
 */
int MountSD1( FATFS * fs )
{
	int status = 0;
	FRESULT ffs_res;

	//unmount SD card 0
	ffs_res = f_mount(NULL,"0:/",0);
	//mount SD card 0
	if(ffs_res == FR_OK)
		ffs_res = f_mount(&fs[0], "0:/",0);
	//determine if we succeeded or failed
	if(ffs_res == FR_OK)
		status = CMD_SUCCESS;
	else
	{
		status = CMD_FAILURE;
	}

	return status;
}

/*
 * Log File Write
 *
 * This function takes a pointer to a char buffer and writes it
 *  into the log file.
 *
 * @param	(char *)Pointer to a char buffer with the information to write
 * @param	(unsigned int)number of bytes to write into the log file
 *
 */
int LogFileWrite( char * write_buff, unsigned int bytes_to_write )
{
	FIL logFile;		//the FAT file we want to work with
	FRESULT ffs_res;	//FAT file system return type
	char mynewline[2] = {'\n','\0'};
	char last_command[50] = "";
	int status = 0;
	unsigned int num_bytes_written = 0;

	//need to append a newline '\n' to the end of the command to store it correctly
	//write the contents of write_buff into a local buffer
	strcat(last_command, write_buff);
	//now append a newline on the end
	strcat(last_command, mynewline);

	/***** Write to log file on SD0 first *****/
	//open with read/write access
	ffs_res = f_open(&logFile, cLogFile0, FA_READ|FA_WRITE);
	if(ffs_res == FR_OK)
		ffs_res = f_lseek(&logFile, file_size(&logFile));
	if(ffs_res == FR_OK)
		ffs_res = f_write(&logFile, last_command, bytes_to_write, &num_bytes_written);
	if(ffs_res == FR_OK)
		ffs_res = f_close(&logFile);
	if(ffs_res == FR_OK)
		status = CMD_SUCCESS;
	else
		status = CMD_FAILURE;

	/***** Write to log file on SD1 *****/
	//open with read/write access
	ffs_res = f_open(&logFile, cLogFile1, FA_READ|FA_WRITE);
	if(ffs_res == FR_OK)
		ffs_res = f_lseek(&logFile, file_size(&logFile));
	if(ffs_res == FR_OK)
		ffs_res = f_write(&logFile, last_command, bytes_to_write, &num_bytes_written);
	if(ffs_res == FR_OK)
		ffs_res = f_close(&logFile);
	if(ffs_res == FR_OK)
		status = CMD_SUCCESS;
	else
		status = CMD_FAILURE;

	return status;
}
