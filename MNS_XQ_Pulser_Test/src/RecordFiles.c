/*
 * RecordFiles.c
 *
 *  Created on: Aug 23, 2019
 *      Author: gstoddard
 */

#include "RecordFiles.h"

//static variables (file scope globals)
static int sd_count_files;
static int sd_count_folders;
static int sd_total_folders;
static int sd_total_files;

//Variables for LS function
static int dir_is_top_level;
static int iter;
static int dir_sequence_count;
static int dir_group_flags;
static FILINFO sd_fno;			//this is static because we call this function recursively
static TCHAR sd_LFName[_MAX_LFN + 1];	//we keep sd_fno static across recursive calls, keep this, too

/*
 * Total folder handling and variable access functions
 */
int sd_totalFoldersIncrement( void )
{
	return ++sd_total_folders;
}

int sd_totalFoldersDecrement( void )
{
	return --sd_total_folders;
}

int SDGetTotalFolders( void )
{
	return sd_total_folders;
}

void SDSetTotalFolders( int num_folders )
{
	sd_total_folders = num_folders;
	return;
}

/*
 * Total file handling and variable access functions
 */
int sd_totalFilesIncrement( void )
{
	return ++sd_total_files;
}

int sd_totalFilesDecrement( void )
{
	return --sd_total_files;
}

int SDGetTotalFiles( void )
{
	return sd_total_files;
}

void SDSetTotalFiles( int num_files )
{
	sd_total_files = num_files;
	return;
}

/*
 * Initialize variables before calling SDCountFiles() or SDScanFiles()
 *
 * @param	none
 *
 * @return	(int)status variable, success/failure
 */
void SDInitDIR( void )
{
	iter = 0;
	sd_count_folders = 0;
	sd_count_files = 0;
	dir_sequence_count = 0;
	dir_group_flags = 1;

	return;
}

/*
 * Helper function to put together the DIR packet header, which is comprised of the following:
 *  the SD card number (0/1)
 *  the most recent Real Time listed in the configuration file
 *  the total number of folders on the SD card
 *  the total number of files on the SD card
 * The only input needed is the value for the sd card number. The primary SD card is 0, the backup
 *  card is 1. The Real Time is retrieved from the configuration file. The value for the number of files
 *  and folders can be taken from the configuration file, but it can also be checked using the
 *  SDCountFilesOnCard() function below. That will give the most up-to-date number.
 * There is no distinction made between DAQ folders/files and WF folders/files. This could be included by
 *  doing a strcmp when looping over the directories with the SDCountFilesOnCard() function.
 * These values will not change during the course of a DIR function call, so this function only needs
 *  to be called once at the beginning. From then the buffer bytes should be left intact.
 *
 * @param	(char *)pointer to the buffer where we are putting the header in
 * @param	(int)the SD card ID number, either 0/1
 *
 * @return	(int)the status variable indicating success/failure for this function
 */
int SDCreateDIRHeader( unsigned char *packet_buffer, int sd_card_number )
{
	int status = 0;
	unsigned int real_time = GetRealTime();

	packet_buffer[CCSDS_HEADER_FULL] = (unsigned char)sd_card_number;
	packet_buffer[CCSDS_HEADER_FULL + 1] = NEWLINE_CHAR_CODE;
	packet_buffer[CCSDS_HEADER_FULL + 2] = (unsigned char)(real_time >> 24);
	packet_buffer[CCSDS_HEADER_FULL + 3] = (unsigned char)(real_time >> 16);
	packet_buffer[CCSDS_HEADER_FULL + 4] = (unsigned char)(real_time >> 8);
	packet_buffer[CCSDS_HEADER_FULL + 5] = (unsigned char)(real_time);
	packet_buffer[CCSDS_HEADER_FULL + 6] = NEWLINE_CHAR_CODE;
	packet_buffer[CCSDS_HEADER_FULL + 7] = (unsigned char)(sd_total_folders >> 24);
	packet_buffer[CCSDS_HEADER_FULL + 8] = (unsigned char)(sd_total_folders >> 16);
	packet_buffer[CCSDS_HEADER_FULL + 9] = (unsigned char)(sd_total_folders >> 8);
	packet_buffer[CCSDS_HEADER_FULL + 10] = (unsigned char)(sd_total_folders);
	packet_buffer[CCSDS_HEADER_FULL + 11] = NEWLINE_CHAR_CODE;
	packet_buffer[CCSDS_HEADER_FULL + 12] = (unsigned char)(sd_total_files >> 24);
	packet_buffer[CCSDS_HEADER_FULL + 13] = (unsigned char)(sd_total_files >> 16);
	packet_buffer[CCSDS_HEADER_FULL + 14] = (unsigned char)(sd_total_files >> 8);
	packet_buffer[CCSDS_HEADER_FULL + 15] = (unsigned char)(sd_total_files);
	packet_buffer[CCSDS_HEADER_FULL + 16] = NEWLINE_CHAR_CODE;

	return status;
}
/*
 * Function to loop over the SD card directories and count up the number of files and folders.
 * There is a distinction made between
 *
 * @param	(char *)pointer to the SD card path we are counting files on
 *
 * @return	(FRESULT)SD card status result
 *
 */
FRESULT SDCountFilesOnCard( char *path )
{
	/*
	 * This function is taken from the elm-chan website: http://elm-chan.org/fsw/ff/doc/readdir.html
	 * I have changed the function from the website so that we can pull the folder and file names,
	 *  and the file sizes and print them to a char buffer that can be sent as a packet.
	*/
	FRESULT res;
	DIR dir;
	UINT i;
	char *fn;
	sd_fno.lfname = sd_LFName;
	sd_fno.lfsize = sizeof(sd_LFName);

	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		for (;;)
		{
			res = f_readdir(&dir, &sd_fno);							/* Read a directory item */
			if (res != FR_OK || sd_fno.fname[0] == 0)break;			/* Break on error or end of dir */
			if (sd_fno.fname[0] == '.') continue;					/* Ignore the dot entry */
			if (sd_fno.fattrib & AM_HID) continue;					/* Ignore hidden directories */
			if (sd_fno.fattrib & AM_SYS) continue;					/* Ignore system directories */
			//check what type of name we should use
			fn = *sd_fno.lfname ? sd_fno.lfname : sd_fno.fname;
			if (sd_fno.fattrib & AM_DIR)
			{
				i = strlen(path);
				sprintf(&path[i], "/%s", fn);
				sd_count_folders++;
				res = SDCountFilesOnCard(path);						/* Enter the directory */
				if (res != FR_OK)
					break;
				path[i] = 0;
			}
			else
				sd_count_files++;
		}
		f_closedir(&dir);
	}

	return res;
}

void SDUpdateFileCounts( void )
{
	//validate our new number against what the system has recorded from creation and deletion:
	//I am choosing to believe this number over what the current count from the system is
	if(sd_count_folders != sd_total_folders)
	{
		sd_total_folders = sd_count_folders;
		SetSDTotalFolders(sd_count_folders);
	}

	if(sd_count_files != sd_total_files)
	{
		sd_total_files = sd_count_files;
		SetSDTotalFiles(sd_count_files);
	}

	//reset sd_count_files so we may reuse it in SDScanFiles()
	sd_count_folders = 0;
	sd_count_files = 0;

	return;
}

/*
 * Function which handles all the messy stuff relating to sending a packet while we're scanning the SD card
 *
 * @param	(unsigned char *)pointer to the packet buffer that the information will be stored in
 *
 * @return	(int)status variable, command success/failure
 */
int SDPrepareDIRPacket( unsigned char *packet_buffer )
{
	int status = 0;

	if(dir_sequence_count == 0)
	{
		if(sd_total_files > sd_count_files)
			dir_group_flags = GF_FIRST_PACKET;
		else
			dir_group_flags = GF_UNSEG_PACKET;
	}
	else
	{
		if(sd_total_files > sd_count_files)
			dir_group_flags = GF_INTER_PACKET;
		else
			dir_group_flags = GF_LAST_PACKET;
	}

	PutCCSDSHeader(packet_buffer, APID_DIR, dir_group_flags, dir_sequence_count, PKT_SIZE_DIR);
	if(CCSDS_HEADER_PRIM + PKT_HEADER_DIR + iter < PKT_SIZE_DIR)
	{
		memset(&(packet_buffer[CCSDS_HEADER_PRIM + PKT_HEADER_DIR + iter]), APID_DIR, DATA_BYTES_DIR - iter);
	}
	CalculateChecksums(packet_buffer);

	return status;
}
/*
 * Function to scan the contents of the Root directory and all folders on the Root directory.
 *
 * @param	(char *)path to the directory to be scanned
 * 				use this command with "0" or "1" to scan the entire Root directory
 * @param	(unsigned char *)pointer to the buffer to use for the DIR packets
 * 				Since this function is called recursively, we must pass this through so the same
 * 				buffer is used, rather than multiple buffers.
 * @param	(XUartPs)the instance of the UART so that we can pass this to SendPacket() which pushes the packet
 * 				out over the Uart to the flight computer
 *
 * @return	(FRESULT) SD card library status indicator; use this to jump back from directories when looping
 * 				If this is FR_OK, we finished successfully
 * 				If this is not FR_OK, there was an error somewhere; depending on the error it could have
 * 				 been from either f_opendir or f_readdir
 */
FRESULT SDScanFilesOnCard( char *path, unsigned char *packet_buffer, XUartPs Uart_PS )
{
	/*
	 * This function is taken from the elm-chan website: http://elm-chan.org/fsw/ff/doc/readdir.html
	 * I have changed the function from the website so that we can pull the folder and file names,
	 *  and the file sizes and print them to a char buffer that can be sent as a packet.
	*/
	int bytes_written = 0;
	FRESULT res;
	DIR dir;
	UINT i;
	char *fn;
	sd_fno.lfname = sd_LFName;
	sd_fno.lfsize = sizeof(sd_LFName);

	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		for (;;)
		{
			res = f_readdir(&dir, &sd_fno);
			if (res != FR_OK || sd_fno.fname[0] == 0)break;	/* Break on error or end of dir */
			if (sd_fno.fname[0] == '.') continue;			/* Ignore the dot entry */
			if (sd_fno.fattrib & AM_HID) continue;			/* Ignore hidden directories */
			if (sd_fno.fattrib & AM_SYS) continue;			/* Ignore system directories */
			//check what type of name we should use
			fn = *sd_fno.lfname ? sd_fno.lfname : sd_fno.fname;
			if (sd_fno.fattrib & AM_DIR)
			{
				//before we get further into the loop, we need to check how many bytes are still availabe in the buffer
				//If there are fewer than 133 bytes (one folder + 6 files), then we send the packet and reset the loop variables
				//otherwise keep looping
				if(DATA_BYTES_DIR - iter <= TOTAL_FOLDER_BYTES)
				{
					SDPrepareDIRPacket(packet_buffer);
					SendPacket(Uart_PS, packet_buffer, CCSDS_HEADER_FULL + PKT_SIZE_DIR);
					//house keeping
					iter = 0;
					dir_sequence_count++;
					memset(&(packet_buffer[CCSDS_HEADER_PRIM + PKT_HEADER_DIR]), '\0', DATA_BYTES_DIR + CHECKSUM_SIZE);
				}

				i = strlen(path);
				sprintf(&path[i], "/%s", fn);
				//DAQ folders are 11 char long, there is a backslash and a newline added on the end = 13 bytes, add one more for the null terminator = 14 bytes (potential max)
				bytes_written = snprintf((char *)&packet_buffer[CCSDS_HEADER_PRIM + PKT_HEADER_DIR + iter], 14, "%s/\n", fn);
				if(bytes_written == 0)
					res = 20;	//unused SD card library error code //TODO: check if this is valid, handle this better
				else if(bytes_written == DAQ_FOLDER_SIZE + 2)
					iter += DAQ_FOLDER_SIZE + 2;
				else if(bytes_written == WF_FOLDER_SIZE + 2)
					iter += WF_FOLDER_SIZE + 2;
				else
					iter += bytes_written;

				dir_is_top_level++;
				res = SDScanFilesOnCard(path, packet_buffer, Uart_PS);				/* Enter the directory */
				if (res != FR_OK)
					break;
				dir_is_top_level--;
				path[i] = 0;
			}
			else
			{
				//check to see if we are ok to write the filename in:
				if(DATA_BYTES_DIR - iter <= DIR_FILE_BYTES)
				{
					SDPrepareDIRPacket(packet_buffer);
					SendPacket(Uart_PS, packet_buffer, PKT_SIZE_DIR + CCSDS_HEADER_FULL);
					//house keeping
					iter = 0;
					dir_sequence_count++;
					memset(&(packet_buffer[CCSDS_HEADER_PRIM + PKT_HEADER_DIR]), '\0', DATA_BYTES_DIR + CHECKSUM_SIZE);
				}
				//write the filename, spacing byte, file size, and another spacing byte
				//the largest possible file name to write is 10 bytes, so 10 + 1 + 4 + 1 = 16, then add one for the null terminator
				//TODO: since the evt files are still written as "evt_S0001.bin" we need at least 3 more bytes than normal, so 17->20 for now //GJS 12-12-2019
				bytes_written = snprintf((char *)&packet_buffer[CCSDS_HEADER_PRIM + PKT_HEADER_DIR + iter], 20, "%s\t%c%c%c%c\n",
						fn,
						(unsigned char)(sd_fno.fsize >> 24),
						(unsigned char)(sd_fno.fsize >> 16),
						(unsigned char)(sd_fno.fsize >> 8),
						(unsigned char)(sd_fno.fsize));
				sd_count_files++;
				if(bytes_written == 0)
					res = 20;	//unused SD card library error code //TODO: check if this is valid, handle this better
				else
					iter += bytes_written;
			}
		}
		f_closedir(&dir);
		//make sure that we send the final packet
		//the check on iter should keep us from sending a packet, then sending it again here
		//essentially, if iter = 0, then we reset it when we sent the packet last and haven't written anything new in
		if(dir_is_top_level == 0 && iter != 0)
		{
			SDPrepareDIRPacket(packet_buffer);
			SendPacket(Uart_PS, packet_buffer, PKT_SIZE_DIR + CCSDS_HEADER_FULL);
			//house keeping
			iter = 0;
			dir_sequence_count++;
			memset(&(packet_buffer[CCSDS_HEADER_PRIM + PKT_HEADER_DIR]), '\0', DATA_BYTES_DIR + CHECKSUM_SIZE);
		}
	}

	return res;
}
