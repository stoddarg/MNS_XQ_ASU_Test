/*
 * TwoDHisto.c
 *
 *  Created on: Feb 4, 2019
 *      Author: gstoddard
 */

#include "TwoDHisto.h"

static int m_x_bin_number;
static int m_y_bin_number;
static unsigned int m_oor_left;
static unsigned int m_oor_right;
static unsigned int m_oor_below;
static unsigned int m_oor_above;
static unsigned int m_valid_multi_hit_event;
static unsigned short m_2DH_pmt0[TWODH_X_BINS][TWODH_Y_BINS];
static unsigned short m_2DH_pmt1[TWODH_X_BINS][TWODH_Y_BINS];
static unsigned short m_2DH_pmt2[TWODH_X_BINS][TWODH_Y_BINS];
static unsigned short m_2DH_pmt3[TWODH_X_BINS][TWODH_Y_BINS];

/*
 * Helper function to allow external functions to get the address of the 2DHs
 *
 * NOTE: You must check that the return value of this function is not NULL, if it is
 * 	and you try and read the pointer, a SEGMENTATION FAULT will occur.
 *
 * @param	(integer)the histogram to get the address of. There are 4 2DHs, one
 * 				for each of the PMT IDs.
 * 				Values should be the macro PMT_ID_#'s from lunah_defines (ie. PMT_ID_1, etc)
 *
 * @return	( integer)CMD_SUCCESS/CMD_FAILURE
 */
int Save2DHToSD( int pmt_ID )
{
	int status = CMD_FAILURE;
	unsigned int numBytesWritten = 0;
	unsigned int m_oor_values[5] = {m_oor_left, m_oor_right, m_oor_below, m_oor_above, m_valid_multi_hit_event};
	char *filename_pointer;
	char filename_buff[100] = "";
	FIL save2DH;
	FRESULT f_res = FR_OK;

	unsigned short (*m_2DH_holder)[TWODH_X_BINS][TWODH_Y_BINS] = NULL;	//pointer to 2D array

	switch(pmt_ID)
	{
	case PMT_ID_0:
		m_2DH_holder = &m_2DH_pmt0;
		filename_pointer = GetFileName( DATA_TYPE_2DH_0 );
		if(filename_pointer == NULL)
			xil_printf("3 return filename pointer 2dh\n");
		else
			snprintf(filename_buff, sizeof(filename_buff), "%s", filename_pointer);
		break;
	case PMT_ID_1:
		m_2DH_holder = &m_2DH_pmt1;
		filename_pointer = GetFileName( DATA_TYPE_2DH_1 );
		if(filename_pointer == NULL)
			xil_printf("4 return filename pointer 2dh\n");
		else
			snprintf(filename_buff, sizeof(filename_buff), "%s", filename_pointer);
		break;
	case PMT_ID_2:
		m_2DH_holder = &m_2DH_pmt2;
		filename_pointer = GetFileName( DATA_TYPE_2DH_2 );
		if(filename_pointer == NULL)
			xil_printf("5 return filename pointer 2dh\n");
		else
			snprintf(filename_buff, sizeof(filename_buff), "%s", filename_pointer);
		break;
	case PMT_ID_3:
		m_2DH_holder = &m_2DH_pmt3;
		filename_pointer = GetFileName( DATA_TYPE_2DH_3 );
		if(filename_pointer == NULL)
			xil_printf("6 return filename pointer 2dh\n");
		else
			snprintf(filename_buff, sizeof(filename_buff), "%s", filename_pointer);
		break;
	default:
		break;
	}

	f_res = f_open(&save2DH, filename_buff, FA_WRITE|FA_OPEN_ALWAYS);
	if(f_res != FR_OK)
	{
		xil_printf("1 open file fail 2dh\n");
		status = CMD_FAILURE;
	}
	f_res = f_lseek(&save2DH, file_size(&save2DH));
	if(f_res != FR_OK)
	{
		xil_printf("4 lseek fail 2dh\n");
		status = CMD_FAILURE;
	}
	if(m_2DH_holder != NULL)
	{
		f_res = f_write(&save2DH, m_2DH_holder, sizeof(unsigned short) * TWODH_X_BINS * TWODH_Y_BINS, &numBytesWritten);
		if(f_res != FR_OK || numBytesWritten != (sizeof(unsigned short) * TWODH_X_BINS * TWODH_Y_BINS))
		{
			//TODO: handle error checking the write
			xil_printf("2 error writing 2dh\n");
			status = CMD_FAILURE;
		}
		else
			status = CMD_SUCCESS;
	}
	//write the out of range values in
	f_res = f_write(&save2DH, m_oor_values, sizeof(unsigned int) * 5, &numBytesWritten);
	if(f_res != FR_OK || numBytesWritten != (sizeof(unsigned int) * 5))
	{
		//TODO: handle error checking the write
		xil_printf("3 error writing 2dh\n");
		status = CMD_FAILURE;
	}
	else
		status = CMD_SUCCESS;

//	sd_updateFileRecords(filename_buff, file_size(&save2DH));
	f_close(&save2DH);
	return status;
}

/*
 * Retrieves and double checks the X array index for the current event being processed.
 * This value will get reported by the EVTs data product.
 *
 * @param	None
 *
 * @return	(int) bin number to be stored in an EVTs event
 */
unsigned int Get_2DHXIndex( void )
{
	return m_x_bin_number;
}

/*
 * Retrieves and double checks the X array index for the current event being processed.
 * This value will get reported by the EVTs data product.
 *
 * @param	None
 *
 * @return	(int) bin number to be stored in an EVTs event
 */
unsigned int Get_2DHYIndex( void )
{
		return m_y_bin_number;
}


/*
 * Takes energy and PSD values from an event and tallies them into 2-D Histograms.
 * This function implements the elliptical neutron cuts to determine if an event
 * was good or not.
 * The PMT ID is a parameter so that we can tally the appropriate histograms, as
 *  well as tally the total, at the same time and in one function.
 *
 *  NOTES: Tallies well for single hit events, but does NOT record multiple hits/PMT IDs within
 *  		one event. These events will still show up in the data products for CPS and EVTs, but
 *  		will not be in the 2DHs.
 *
 * @param	The calculated energy of the event
 * @param	The calculated PSD ratio of the event
 * @param	The PMT ID from the event
 * 			Values should be the macro PMT_ID_#'s from lunah_defines (ie. PMT_ID_1, etc)
 *
 * @return	1 is the point in the given bin numbers was within the 2DH bounds
 * 			0 is the point was not in the bounds
 * 			-1 is the point was a multi-hit and not recorded
 */
int Tally2DH(int energy_bin, int psd_bin, int pmt_ID)
{
	int status = 0;

	if(0 <= energy_bin && energy_bin < TWODH_X_BINS)
	{
		if(0 <= psd_bin && psd_bin < TWODH_Y_BINS)
		{
			//value is good
			switch(pmt_ID)
			{
			case PMT_ID_0:
				m_2DH_pmt0[m_x_bin_number][m_y_bin_number]++;
				break;
			case PMT_ID_1:
				m_2DH_pmt1[m_x_bin_number][m_y_bin_number]++;
				break;
			case PMT_ID_2:
				m_2DH_pmt2[m_x_bin_number][m_y_bin_number]++;
				break;
			case PMT_ID_3:
				m_2DH_pmt3[m_x_bin_number][m_y_bin_number]++;
				break;
			default:
				//don't record non-singleton hits in a 2DH
				m_valid_multi_hit_event++;
				status = -1;
				break;
			}
		}
		else
			status = 0;
	}
	else
		status = 0;

	return status;
}

