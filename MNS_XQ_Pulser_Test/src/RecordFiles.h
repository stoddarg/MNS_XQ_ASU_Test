/*
 * RecordFiles.h
 *
 *  Created on: Aug 23, 2019
 *      Author: gstoddard
 */

#ifndef SRC_RECORDFILES_H_
#define SRC_RECORDFILES_H_

#include <stdio.h>
#include "xil_printf.h"
#include "ff.h"
#include "lunah_defines.h"
#include "lunah_utils.h"
#include "SetInstrumentParam.h"

#define NO_TX_CHANGE			-1
#define MAX_FILENAME_SIZE		14
#define NUM_FILES_PER_FOLDER	25
#define DIR_FILE_BYTES			19
#define TOTAL_FOLDER_BYTES		127	//the number of bytes to write the folder name and six file names and sizes //TEMP, GJS 12-12-2019

#define DIR_PACKET_HEADER		17	//Number of bytes in the DIR packet header

int sd_totalFoldersIncrement( void );
int sd_totalFoldersDecrement( void );
int SDGetTotalFolders( void );
void SDSetTotalFolders( int num_folders );
int sd_totalFilesIncrement( void );
int sd_totalFilesDecrement( void );
int SDGetTotalFiles( void );
void SDSetTotalFiles( int num_folders );
void SDInitDIR( void );
int SDCreateDIRHeader( unsigned char *packet_buffer, int sd_card_number );
FRESULT SDCountFilesOnCard( char *path );
void SDUpdateFileCounts( void );
int SDPrepareDIRPacket( unsigned char *packet_buffer);
FRESULT SDScanFilesOnCard( char *path, unsigned char *packet_buffer, XUartPs Uart_PS  );

#endif /* SRC_RECORDFILES_H_ */
