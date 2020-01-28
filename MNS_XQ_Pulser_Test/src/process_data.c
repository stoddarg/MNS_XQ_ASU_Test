/*
 * process_data_streamline.c
 *
 *  Created on: Jan 23, 2020
 *      Author: GStoddard
 */

#include "process_data.h"

//File Scope Variables and Buffers
static int evt_iter;										//event buffer iterator
static const GENERAL_EVENT_TYPE evtEmptyStruct;				//use this to reset the holder struct each iteration
static GENERAL_EVENT_TYPE event_buffer[EVENT_BUFFER_SIZE];	//buffer to store events //2048 * 8 bytes = 16384 bytes
static unsigned int m_first_event_time_FPGA;				//the first event time which needs to be written into every data product header
/*
 * Helper function to allow external functions to grab the EVTs buffer and write it to SD
 */
GENERAL_EVENT_TYPE * GetEVTsBufferAddress( void )
{
	return event_buffer;
}


void ResetEVTsBuffer( void )
{
	memset(event_buffer, '\0', sizeof(event_buffer));
	return;
}

void ResetEVTsIterator( void )
{
	evt_iter = 0;
	return;
}


unsigned int GetFirstEventTime( void )
{
	return m_first_event_time_FPGA;
}


/*
 * This function will be called after we read in a buffer of valid data from the FPGA.
 *  Here is where the data stream from the FPGA is scanned for events and each event
 *  is processed to pull the PSD and energy information out. We identify it the event
 *  is within the current 1 second CPS interval, as well as bin the events into a
 *  2-D histogram which is reported at the end of a run.
 *
 * @param	A pointer to the data buffer
 *
 * @return	SUCCESS/FAILURE
 */
int ProcessData( unsigned int * data_raw )
{
	bool valid_event = FALSE;	//Unused (4/8/2019) //could use this to check for reset request //otherwise probably delete
	int iter = 0;
	int m_ret = 0;	//for 2DH tallies
	int m_events_processed = 0;
	int m_energy_bin = 0;
	int m_psd_bin = 0;
	unsigned int m_tagging_bit = 0;
	unsigned int num_bytes_written = 0;
	unsigned int m_event_number_holder = 0;
	unsigned int m_pmt_ID_holder = 0;
	unsigned int m_FPGA_time_holder = 0;
	unsigned int m_bad_event = 0;
	double m_baseline_int = 0.0;
	double m_short_int = 0.0;
	double m_long_int = 0.0;
	double m_full_int = 0.0;
	double bl_avg = 0.0;
	double bl1 = 0.0;
	double bl2 = 0.0;
	double bl3 = 0.0;
	double bl4 = 0.0;
	double si = 0.0;
	double li = 0.0;
	double fi = 0.0;
	double psd = 0.0;
	FRESULT f_res = FR_OK;
	GENERAL_EVENT_TYPE event_holder = evtEmptyStruct;

	//get the integration times
	m_baseline_int = (double)GetBaselineInt();
	m_short_int = (double)GetShortInt();
	m_long_int = (double)GetLongInt();
	m_full_int = (double)GetFullInt();

	FIL *cpsDataFile = GetCPSFilePointer();
	if (cpsDataFile == NULL)
	{
		//TODO: handle error with pointer
		xil_printf("error getting FIL pointer 6\n");
	}

	while(iter < DATA_BUFFER_SIZE)
	{
		event_holder = evtEmptyStruct;	//reset event structure
		m_tagging_bit = 0;

		switch(data_raw[iter])
		{
		case 111111:
			//this is the data event case //0x0001B207
			while(data_raw[iter+1] == 111111 && iter < (DATA_BUFFER_SIZE - EVT_EVENT_SIZE))//handles any number of 111111's in succession
			{
				iter++;
			}
			if(iter >= (DATA_BUFFER_SIZE - 7))	//if we are at the top of the buffer, break out to avoid bad indexing
				break;
			if(data_raw[iter+1] >= cpsGetCurrentTime())	//time must be the same or increasing
			{
				if(((data_raw[iter+3] & 0xFFFFFFF0) >> 4) > m_event_number_holder)
				{
					if((data_raw[iter+4] < data_raw[iter+5]) && (data_raw[iter+5] < data_raw[iter+6]) && (data_raw[iter+6] < data_raw[iter+7]))
					{
						valid_event = TRUE;
						//if the first event time has not been recorded, then set one //this allows us to function without a false event
						if(cpsGetFirstEventTime() == 0)
							cpsSetFirstEventTime(data_raw[iter+1]);
						//loop recording the CPS events until we don't need to //this only happens when the current event belongs to the next one-second time interval
						while(cpsCheckTime(data_raw[iter+1]) == TRUE)
						{
							f_res = f_write(cpsDataFile, (char *)cpsGetEvent(), sizeof(CPS_EVENT_STRUCT_TYPE), &num_bytes_written);
							if(f_res != FR_OK || num_bytes_written != sizeof(CPS_EVENT_STRUCT_TYPE))
							{
								//TODO:handle error with writing
								xil_printf("error writing 4\n");
							}
							f_res = f_sync(cpsDataFile);
							if(f_res != FR_OK || num_bytes_written != sizeof(CPS_EVENT_STRUCT_TYPE))
							{
								//TODO:handle error with writing
								xil_printf("error writing 5\n");
							}
							//reset the neutron counts for the CPS data product
							CPSResetCounts();
						}

						//calculate the moving average of the baseline integral
						si = 0.0;	li = 0.0;	fi = 0.0;	psd = 0.0;
						bl4 = bl3; bl3 = bl2; bl2 = bl1;
						bl1 = (double)data_raw[iter+4] / (16.0 * m_baseline_int);
						if(bl4 == 0.0)
							bl_avg = bl1;
						else
							bl_avg = (bl4 + bl3 + bl2 + bl1) / 4.0;
						//calculate the baseline corrected integrals from the event
						si = ((double)data_raw[iter+5]) / (16.0) - (bl_avg * m_short_int);
						li = ((double)data_raw[iter+6]) / (16.0) - (bl_avg * m_long_int);
						fi = ((double)data_raw[iter+7]) / (16.0) - (bl_avg * m_full_int);

						//li, si must be positive, li greater than si (ensures positive psd values and li != si)
						if( li > 0 && si > 0 && li > si) //TODO: how much should we test here? //si != 0, li > si, si > 0 ?
							psd = si / (li - si);
						else
						{
							//TODO: PSD value not good
							psd = 1.999;	//set to highest good bin
							m_bad_event++;
						}
						//calculate the "bin space" values of the energy and PSD
						m_energy_bin = (int)floor(fi / ((double)TWODH_ENERGY_MAX / (double)TWODH_X_BINS));
						m_psd_bin = (int)floor(psd / ((double)TWODH_PSD_MAX / (double)TWODH_Y_BINS));
						//generate the bin value to use
						if(0 <= m_energy_bin && m_energy_bin < TWODH_X_BINS)
							m_energy_bin &= 0x01FF;
						else
							m_energy_bin = 0x01FF;

						if(0 <= m_psd_bin && m_psd_bin < TWODH_Y_BINS)
							m_psd_bin &= 0x3F;	//move to 6 bits 10-11-2019
						else
							m_psd_bin = 0x3F;
						//assign this value to the PMT ID so that we have something to compare with the defined PMT hit ID
						m_pmt_ID_holder = data_raw[iter+3] & 0x0F;
						if(m_pmt_ID_holder == PMT_ID_0 || m_pmt_ID_holder == PMT_ID_1 || m_pmt_ID_holder == PMT_ID_2 || m_pmt_ID_holder == PMT_ID_3)
						{
							//process the event for CPS
							m_tagging_bit = CPSUpdateTallies(m_energy_bin, m_psd_bin, m_pmt_ID_holder);
							if(m_tagging_bit < 0)
							{
								//TODO: handle the error in the CPS neutron counts
								// -1 = indicates an error with the PMT hit ID
							}

							//process the event for 2DH
							m_ret = Tally2DH(m_energy_bin, m_psd_bin, m_pmt_ID_holder);
							if(m_ret != 1)
							{
								//TODO: identify what can go wrong and handle a bad tally
								// 0 is the bins were not in the 2DGH
								//-1 is the PMT ID indicated a multi-hit
							}
						}
						else
						{
							//mark the event as a bad event //we are not interested in events with PMT ID of 0 or multi-hit events
							m_bad_event++;
						}

						event_holder.field0 = 0xFF;
						event_holder.field1 |= (m_pmt_ID_holder		& 0x000F) << 4;
						m_event_number_holder = (data_raw[iter+3]	& 0xFFF0) >> 4;
						event_holder.field1 |= (unsigned char)((m_event_number_holder	& 0x0F00) >> 8);
						event_holder.field2 |= (unsigned char)( m_event_number_holder	& 0x0FF);
						event_holder.field3 |= (unsigned char)((m_energy_bin	& 0x1FE) >> 1);
						event_holder.field4 |= (unsigned char)((m_energy_bin	& 0x001) << 7);
						event_holder.field4 |= (unsigned char)((m_psd_bin	 	& 0x3F) << 1);
						event_holder.field4 |= (unsigned char)( m_tagging_bit 	& 0x01);
						m_FPGA_time_holder = ((data_raw[iter+1] & 0xFFFFFF00) >> 8);
						event_holder.field5 = (unsigned char)((m_FPGA_time_holder & 0xFF0000) >> 16);
						event_holder.field6 = (unsigned char)((m_FPGA_time_holder & 0x00FF00)>> 8);
						event_holder.field7 = (unsigned char)( m_FPGA_time_holder & 0x0000FF);

						event_buffer[evt_iter] = event_holder;
						evt_iter++;
						iter += 8;
						m_events_processed++;
					}
					else
						valid_event = FALSE;
				}
				else
					valid_event = FALSE;
			}
			else
				valid_event = FALSE;

			if(valid_event == FALSE)
				iter++;
			break;
		case 1073852935:
			//pulser event header //0x4001B207
			// for right now, we just want to see this, we don't care what is in it
			//when we get here, report one event in the buffer
			if(iter >= DATA_BUFFER_SIZE - 9)
			{
				iter++;
				break;	//skip out early
			}
			//if we see the event ID for a triggered event, just write the triggered event into the data stream
			// then bump up the iterator value and keep going
			event_holder.field0 = 0xEE;
			event_holder.field1 = 0xEE;
			event_holder.field2 = 0xEE;
			event_holder.field3 = 0xEE;
			event_holder.field4 = 0xEE;
			//assume the time is in field 2, just like in the
			event_holder.field5 = (unsigned char)(data_raw[iter + 1] >> 24);
			event_holder.field6 = (unsigned char)(data_raw[iter + 1] >> 16);
			event_holder.field7 = (unsigned char)(data_raw[iter + 1] >> 8);

			event_buffer[evt_iter] = event_holder;
			evt_iter++;
			iter += 8;
			m_events_processed++;
			break;
		case 2147594759:
			//this is a false event //0x8001B207
			if(iter >= DATA_BUFFER_SIZE - 9 )	//meant to protect from writing above array indices...
			{
				iter++;
				break;	//skip out early
			}
			while(data_raw[iter+1] == 0x8001B207 && iter < (DATA_BUFFER_SIZE - EVT_EVENT_SIZE))//handles any number of 111111's in succession
			{
				iter++;
			}
			if( data_raw[iter + 1] == 2147594759 && data_raw[iter + 8] == 111111)
			{
				cpsSetFirstEventTime(data_raw[iter + 1]);
				m_first_event_time_FPGA = data_raw[iter + 1];

				event_holder.field0 = 0xDD;
				event_holder.field1 = 0xDD;
				event_holder.field2 = 0xDD;
				event_holder.field3 = 0xDD;
				event_holder.field4 = 0xDD;
				event_holder.field5 = (unsigned char)(data_raw[iter + 1] >> 24);
				event_holder.field6 = (unsigned char)(data_raw[iter + 1] >> 16);
				event_holder.field7 = (unsigned char)(data_raw[iter + 1] >> 8);

				event_buffer[evt_iter] = event_holder;
				evt_iter++;
				iter += 8;
				m_events_processed++;
			}
			else
				iter++;
			break;
		default:
			//this indicates that we miscounted our place in the buffer somewhere
			//or there is junk in the buffer in the middle of an event
			//TODO: handle a bad event or junk in the buffer
			iter++;	//move past it, at least, we can sync back up by finding the 111111 again
			break;
		}//END OF SWITCH ON RAW DATA

		if(iter > (DATA_BUFFER_SIZE - EVT_EVENT_SIZE))	//will read past the array if iter goes above
			break;
		if(evt_iter >= EVENT_BUFFER_SIZE)	//we have run out of open events in the buffer
			break;
		if(m_events_processed >= VALID_BUFFER_SIZE)	//we have processed every event in the buffer (max of 512)
			break;
		//TODO: fully error check the buffering here
	}//END OF WHILE

	//TODO: give this return value a meaning
	return 0;
}

