/*
 * SetInstrumentParam.c
 *
 *  Self terminating functions to set instrument parameters
 *
 *  Created on: Jun 20, 2018
 *      Author: IRDLAB
 */

#include "SetInstrumentParam.h"

//File-Scope Variables
static char cConfigFile[] = "0:/MNSCONF.bin";
static CONFIG_STRUCT_TYPE ConfigBuff;
static int m_trigger_threshold;
static int m_baseline_integration_samples;
static int m_short_integration_samples;
static int m_long_integration_samples;
static int m_full_integration_samples;

/* This can be called for two different reasons:
 *  1.) When there is no config file on the SD card, this holds the default (hard coded)
 *  system parameters which are to be used until changed by the user.
 *  2.) When we want to reset the values in the config file
 *
 * This function will set all the values in the Config Buffer (file scope static buffer)
 *  to their original values.
 *
 * @param	None
 *
 * @return	None
 *
 */
void CreateDefaultConfig( void )
{
	//initialize the struct with all default values
	ConfigBuff = (CONFIG_STRUCT_TYPE){
		.ECalSlope=1.0,
		.ECalIntercept=0.0,
		.TriggerThreshold=9000,
		.IntegrationBaseline=0,
		.IntegrationShort=35,
		.IntegrationLong=131,
		.IntegrationFull=1531,
		.HighVoltageValue[0]=11,
		.HighVoltageValue[1]=11,
		.HighVoltageValue[2]=11,
		.HighVoltageValue[3]=11,
		.SF_E[0] = 1.0,
		.SF_E[1] = 1.0,
		.SF_E[2] = 1.0,
		.SF_E[3] = 1.0,
		.SF_E[4] = 1.0,
		.SF_E[5] = 1.0,
		.SF_E[6] = 1.0,
		.SF_E[7] = 1.0,
		.SF_PSD[0] = 1.0,
		.SF_PSD[1] = 1.0,
		.SF_PSD[2] = 1.0,
		.SF_PSD[3] = 1.0,
		.SF_PSD[4] = 1.0,
		.SF_PSD[5] = 1.0,
		.SF_PSD[6] = 1.0,
		.SF_PSD[7] = 1.0,
		.Off_E[0] = 0.0,
		.Off_E[1] = 0.0,
		.Off_E[2] = 0.0,
		.Off_E[3] = 0.0,
		.Off_E[4] = 0.0,
		.Off_E[5] = 0.0,
		.Off_E[6] = 0.0,
		.Off_E[7] = 0.0,
		.Off_PSD[0] = 0.0,
		.Off_PSD[1] = 0.0,
		.Off_PSD[2] = 0.0,
		.Off_PSD[3] = 0.0,
		.Off_PSD[4] = 0.0,
		.Off_PSD[5] = 0.0,
		.Off_PSD[6] = 0.0,
		.Off_PSD[7] = 0.0,
		.TotalFiles = 0,
		.TotalFolders =	0,
		.MostRecentRealTime = 0
	};

	return;
}

/*
 * Allow a user to have access to the config buffer, so they can write it to a file/etc
 *
 * @param	None
 *
 * @return	Pointer to the config buffer
 */
CONFIG_STRUCT_TYPE * GetConfigBuffer( void )
{
	return &ConfigBuff;
}

/*
 * Access the integration times which are set with the system at the moment.
 * This function returns the value of the requested integration time in SAMPLES, not nanoseconds.
 *  The reason to return samples over ns is so that we don't have to convert to
 *  process the data.
 *
 * @param	None
 *
 * @return	(int)baseline samples
 */
int GetBaselineInt( void )
{
	return m_baseline_integration_samples;
}

int GetShortInt( void )
{
	return m_short_integration_samples;
}

int GetLongInt( void )
{
	return m_long_integration_samples;
}

int GetFullInt( void )
{
	return m_full_integration_samples;
}

/* This function handles initializing the system with the values from the config file.
 * If no config file exists, one will be created using the default (hard coded) values
 *  available to the system.
 * If the config file is found on the SD card, then it is read in and values are assigned
 *  to a config structure, but not set in the system.
 *
 * @param	None
 *
 * @return	FR_OK (0) or command FAILURE (!0)
 *
 */
int InitConfig( void )
{
	uint NumBytesWr;
	uint NumBytesRd;
	FRESULT fres = FR_OK;
	FIL ConfigFile;
	int ConfigSize = sizeof(ConfigBuff);

	// check if the config file exists
	fres = f_open(&ConfigFile, cConfigFile, FA_READ|FA_WRITE|FA_OPEN_EXISTING);
	if( fres == FR_OK )
	{
		fres = f_lseek(&ConfigFile, 0);
		if(fres == FR_OK)
			fres = f_read(&ConfigFile, &ConfigBuff, ConfigSize, &NumBytesRd);
		f_close(&ConfigFile);

		//set the values for the number of files/folders on the SD cards
		SDSetTotalFiles( ConfigBuff.TotalFiles );
		SDSetTotalFolders( ConfigBuff.TotalFolders );
	}
	else if(fres == FR_NO_FILE)// The config file does not exist, create it
	{
		CreateDefaultConfig();
		fres = f_open(&ConfigFile, cConfigFile, FA_READ|FA_WRITE|FA_OPEN_ALWAYS);
		if(fres == FR_OK)
			fres = f_write(&ConfigFile, &ConfigBuff, ConfigSize, &NumBytesWr);
		f_close(&ConfigFile);
		//call the function to scan the SD card and count the files/folder and document each folder
		//TODO: build this function into RecordFiles.c
	}
	else
	{
		//TODO: handle an error which is not OK or No File

	}

	return (int)fres;
}

/* This function will save the current system configuration to the configuration file, if it exists.
 * If no config file exists, then this function will create it and fill it with the current
 *  configuration structure.
 *
 *
 * @param	None
 *
 * @return	FR_OK (0) or command FAILURE (!0)
 *
 */
int SaveConfig()
{
	uint NumBytesWr;
	FRESULT F_RetVal;
	FIL ConfigFile;
	int RetVal = 0;
	int ConfigSize = sizeof(ConfigBuff);

	//we assume the config file exists already
	F_RetVal = f_open(&ConfigFile, cConfigFile, FA_READ|FA_WRITE|FA_OPEN_ALWAYS);
	if(F_RetVal == FR_OK)
		F_RetVal = f_lseek(&ConfigFile, 0);
	if(F_RetVal == FR_OK)
		F_RetVal = f_write(&ConfigFile, &ConfigBuff, ConfigSize, &NumBytesWr);
	//close regardless of the return value
	F_RetVal = f_close(&ConfigFile);

	RetVal = (int)F_RetVal;
    return RetVal;
}

/*
 *    Set Event Trigger Threshold
 * 		  Threshold = (Integer) A value between 0 - 16000
 *        Description: 	Change the instruments trigger threshold.
 *        				This value is recorded as the new default value for the system.
 *        Latency: TBD
 *        Return: command SUCCESS (0) or command FAILURE (1)
 *
 */
int SetTriggerThreshold(int iTrigThreshold)
{
	int status = 0;	//0=SUCCESS, 1=FAILURE

	//check that it's within accepted values
	if((iTrigThreshold > 0) && (iTrigThreshold < 16384))
	{
		//set the threshold in the FPGA
		Xil_Out32(XPAR_AXI_GPIO_10_BASEADDR, (u32)(iTrigThreshold));
		//read back value from the FPGA and compare with intended change
		if(iTrigThreshold == Xil_In32(XPAR_AXI_GPIO_10_BASEADDR))
		{
			ConfigBuff.TriggerThreshold = iTrigThreshold;
			SaveConfig();
			m_trigger_threshold = iTrigThreshold;
			status = CMD_SUCCESS;
		}
		else
			status = CMD_FAILURE;	//indicate we did not set the threshold appropriately
	}
	else //values were not within acceptable range
	{
		status = CMD_FAILURE;
	}

	return status;
}

/*
 * SetEnergyCalParams
 *      Set Energy Calibration Parameters
 *		Syntax: SetEnergyCalParam(Slope, Intercept)
 *			Slope = (Float) value for the slope
 * 			Intercept = (Float) value for the intercept
 * 		Description:  These values modify the energy calculation based on the formula y = m*x + b,
 * 					  where m is the slope from above and b is the intercept.
 * 					  The default values are m (Slope) = 1.0 and b (Intercept) = 0.0
 * 		Latency: TBD
 *      Return: command SUCCESS (0) or command FAILURE (1)
 */
int SetEnergyCalParam(float Slope, float Intercept)
{
	int status = 0;

	//check that it's within accepted values
	if((Slope >= 0.0) && (Slope <= 10.0))
	{
		//check that it's within accepted values
		if((Intercept > -100.0) && (Intercept <= 100.0))
		{
			ConfigBuff.ECalSlope = Slope;
			ConfigBuff.ECalIntercept = Intercept;
			SaveConfig();

			status = CMD_SUCCESS;
		}
		else
			status = CMD_FAILURE;
	}
	else
		status = CMD_FAILURE;
	return status;
}

/*
 * Set the cuts on neutron energy(E) and psd ratio(P) when calculating neutron
 *  totals for the MNS_EVTS, MNS_CPS, and MNS_SOH data files.
 * These will be values to modify the elliptical cuts being placed on the events read in.
 * There are two ellipses by default, one at 1 sigma and one at 2 sigma, but the location in the E-P phase
 *  space can be modified by these parameters. This function allows the user to shift the centroid
 *  around (offsetE/P) in the E-P space or scale the size of the ellipses larger or smaller (scaleE/P).
 *
 * @param moduleID	Assigns the cut values to a specific CLYC module
 * 					Valid input range: 0 - 3
 * @param ellipseNum	Assigns the cut values to a specific ellipse for the module chosen
 * 						Valid input range: 1, 2
 * @param scaleE/scaleP	Scale the size of the ellipse being used to cut neutrons in the data
 * 						Valid input range: 0 - 25.5,
 * @param offsetE/offsetP	Move the centroid of the bounding ellipse around on the plot
 * 							Valid input range, E: 0 - 10 MeV (0-200,000 keV?)
 * 							Valid input range, P: 0 - 2
 * Latency: TBD
 *
 * Return: command SUCCESS (0) or command FAILURE (1)
 *
 */
int SetNeutronCutGates(int moduleID, int ellipseNum, float scaleE, float scaleP, float offsetE, float offsetP)
{
	int status = CMD_SUCCESS;

	//what do we need to check with the parameter input?
	//check that the scale factors are not 0
	if(scaleE != 0 && scaleP != 0)
	{
		if(offsetE)
		{
			//do these checks even make sense?
		}
	}
	//check that the offsets won't move the centroid to a different quadrant

	switch(moduleID)
	{
	case 0: //module 0
		switch(ellipseNum)
		{
		case 0:
			ConfigBuff.SF_E[0] = scaleE;
			ConfigBuff.SF_PSD[0] = scaleP;
			ConfigBuff.Off_E[0] = offsetE;
			ConfigBuff.Off_PSD[0] = offsetP;
			break;
		case 1:
			ConfigBuff.SF_E[1] = scaleE;
			ConfigBuff.SF_PSD[1] = scaleP;
			ConfigBuff.Off_E[1] = offsetE;
			ConfigBuff.Off_PSD[1] = offsetP;
			break;
		default:
			status = CMD_FAILURE;
			break;
		}
		break;
	case 1:	//module 1
		switch(ellipseNum)
		{
		case 0:
			ConfigBuff.SF_E[2] = scaleE;
			ConfigBuff.SF_PSD[2] = scaleP;
			ConfigBuff.Off_E[2] = offsetE;
			ConfigBuff.Off_PSD[2] = offsetP;
			break;
		case 1:
			ConfigBuff.SF_E[3] = scaleE;
			ConfigBuff.SF_PSD[3] = scaleP;
			ConfigBuff.Off_E[3] = offsetE;
			ConfigBuff.Off_PSD[3] = offsetP;
			break;
		default:
			status = CMD_FAILURE;
			break;
		}
		break;
	case 2:	//module 2
		switch(ellipseNum)
		{
		case 0:
			ConfigBuff.SF_E[4] = scaleE;
			ConfigBuff.SF_PSD[4] = scaleP;
			ConfigBuff.Off_E[4] = offsetE;
			ConfigBuff.Off_PSD[4] = offsetP;
			break;
		case 1:
			ConfigBuff.SF_E[5] = scaleE;
			ConfigBuff.SF_PSD[5] = scaleP;
			ConfigBuff.Off_E[5] = offsetE;
			ConfigBuff.Off_PSD[5] = offsetP;
			break;
		default:
			status = CMD_FAILURE;
			break;
		}
		break;
	case 3:	//module 3
		switch(ellipseNum)
		{
		case 0:
			ConfigBuff.SF_E[6] = scaleE;
			ConfigBuff.SF_PSD[6] = scaleP;
			ConfigBuff.Off_E[6] = offsetE;
			ConfigBuff.Off_PSD[6] = offsetP;
			break;
		case 1:
			ConfigBuff.SF_E[7] = scaleE;
			ConfigBuff.SF_PSD[7] = scaleP;
			ConfigBuff.Off_E[7] = offsetE;
			ConfigBuff.Off_PSD[7] = offsetP;
			break;
		default:
			status = CMD_FAILURE;
			break;
		}
		break;
	default: //bad value for the module ID, just use the defaults
		//just leave the cuts, no change
		status = CMD_FAILURE;
		break;
	}
	if(status == CMD_SUCCESS)
		SaveConfig();

	return status;
}

/*
 * Set the High Voltage of the HV potentiometers on the analog board
 *
 * NOTE: connections to pot 1 and pot 2 are reversed - handled in the function
 * 	What this means is that the address for PMT 1 and PMT 2 got swapped, so when
 * 	we are trying to change the pot value for PMT 1, we need the address for PMT 2
 * 	and the reverse for PMT 2.
 *
 * As of 7/25/2019 this swap is not going to be fixed for the Mini-NS code.
 * 12-18-2019 - the swap is handled by this function, disregard that the two HV pot addresses are swapped.
 *
 * 		Syntax: SetHighVoltage(PMTID, Value)
 * 			PMTID = (Integer) PMT ID, 0 - 3, 4 to choose all tubes
 * 			Value = (Integer) high voltage to set, 0 - 255 (not linearly mapped to volts)
 * 		Description: Set the bias voltage on any PMT in the array. The PMTs may be set individually or as a group.
 *			Latency: TBD
 *			Return: command SUCCESS (0) or command FAILURE (1)
 */
int SetHighVoltage(XIicPs * Iic, unsigned char PmtId, int Value)
{
	int IIC_SLAVE_ADDR1 = 0x20; //HV on the analog board - write to HV pots, RDAC
	unsigned char i2c_Send_Buffer[2];
	unsigned char i2c_Recv_Buffer[2];
	unsigned char cntrl = 16;  // write command
	int RetVal = 0;
	int status = 0;
	int iterator = 0;

	//TODO: have the system check whether or not it is enabled
	// is this a system parameter that we should set in the configuration buffer?
	// or is this something that should just be a global parameter we can reference?

	//TODO: error checking for this function includes making sure that we keep track of the HV value that was requested
	// even if it doesn't get set. The value can be rejected if the system/analog board is not enabled because
	// the I2C will not respond and we'll get RetVal != XST_SUCCESS and the HV value will not be saved in the config file

	// Fix swap of pot 2 and 3 connections if PmtId == 1 make it 2 and if PmtId == 2 make it 1
	switch(PmtId)
	{
	case 1:
		PmtId = 2;
		break;
	case 2:
		PmtId = 1;
		break;
	default:
		break;
	}

	//check the PMT ID is ok
	if((PmtId > -1) && (PmtId <= 4))
	{
		//check tap value is an acceptable number
		if((Value >= 0) && (Value <= 255))
		{
			//We just want to do a single tube
			if(PmtId != 4)
			{
				//create the send buffer
				i2c_Send_Buffer[0] = cntrl | PmtId;
				i2c_Send_Buffer[1] = Value;
				//send the command to the HV
				RetVal = IicPsMasterSend(Iic, IIC_DEVICE_ID_0, i2c_Send_Buffer, i2c_Recv_Buffer, &IIC_SLAVE_ADDR1);
				if(RetVal == XST_SUCCESS)
				{
					// write to config file
					//swap the PMT ID back before setting in the configuration file
					//this should completely obscure the fact that there is a swap happening
					if(PmtId == 1)
						PmtId = 2;
					else if(PmtId == 2)
						PmtId = 1;
					ConfigBuff.HighVoltageValue[PmtId] = Value;
					SaveConfig();
					status = CMD_SUCCESS;
				}
				else
					status = CMD_FAILURE;
			}
			else if(PmtId == 4)
			{
				//do the above code for each PMT
				for(iterator = 0; iterator < 4; iterator++)
				{
					//cycle over PmtId 0, 1, 2, 3 to set the voltage on each PMT
					PmtId = iterator;
					//create the send buffer
					i2c_Send_Buffer[0] = cntrl | PmtId;
					i2c_Send_Buffer[1] = Value;
					//send the command to the HV
					RetVal = IicPsMasterSend(Iic, IIC_DEVICE_ID_0 ,i2c_Send_Buffer, i2c_Recv_Buffer, &IIC_SLAVE_ADDR1);
					if(RetVal == XST_SUCCESS)
					{
						// write to config file
						ConfigBuff.HighVoltageValue[PmtId] = Value;
						SaveConfig();
						status = CMD_SUCCESS;
					}
					else
					{
						status = CMD_FAILURE;
						break;
					}
				}
			}
			else
				status = CMD_FAILURE;
		}
		else
			status = CMD_FAILURE;
	}
	else
		status = CMD_FAILURE;

	return status;
}

/*
 * SetIntergrationTime
 * 		Set Integration Times
 * 		Syntax: SetIntergrationTime(baseline, short, long ,full)
 * 			Values = (Signed Integer) values in microseconds
 * 		Description: Set the integration times for event-by-event data.
 *		Latency: TBD
 *		Return: command SUCCESS (0) or command FAILURE (1)
 */
int SetIntegrationTime(int Baseline, int Short, int Long, int Full)
{
	int status = 0;

	//should do more error checking on the range for baseline and full
	//TODO: Get the range of acceptable values for the integration times
	if((Baseline >= -200) && (Full < 16384))
	{
		if((Baseline < Short) && ( Short < Long) && (Long < Full))
		{
			Xil_Out32 (XPAR_AXI_GPIO_0_BASEADDR, ((u32)(Baseline+52)/4));
			Xil_Out32 (XPAR_AXI_GPIO_1_BASEADDR, ((u32)(Short+52)/4));
			Xil_Out32 (XPAR_AXI_GPIO_2_BASEADDR, ((u32)(Long+52)/4));
			Xil_Out32 (XPAR_AXI_GPIO_3_BASEADDR, ((u32)(Full+52)/4));

			//error check to make sure that we have set all the values correctly
			//we can worry about error checking later on in the FSW process
			//read back the values from the FPGA //They should be equal to the values we set
			if(Xil_In32(XPAR_AXI_GPIO_0_BASEADDR) == (Baseline+52)/4)
			{
				if(Xil_In32(XPAR_AXI_GPIO_1_BASEADDR) == (Short+52)/4)
				{
					if(Xil_In32(XPAR_AXI_GPIO_2_BASEADDR) == (Long+52)/4)
					{
						if(Xil_In32(XPAR_AXI_GPIO_3_BASEADDR) == (Full+52)/4)
						{
							ConfigBuff.IntegrationBaseline = Baseline;
							ConfigBuff.IntegrationShort = Short;
							ConfigBuff.IntegrationLong = Long;
							ConfigBuff.IntegrationFull = Full;
							SaveConfig();
							m_baseline_integration_samples = (INTEG_TIME_START + Baseline) / NS_TO_SAMPLES + 1;	//add one sample to each
							m_short_integration_samples = (INTEG_TIME_START + Short) / NS_TO_SAMPLES + 1;
							m_long_integration_samples = (INTEG_TIME_START + Long) / NS_TO_SAMPLES + 1;
							m_full_integration_samples = (INTEG_TIME_START + Full) / NS_TO_SAMPLES + 1;
							status = CMD_SUCCESS;
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
			else
				status = CMD_FAILURE;

			//if this comes back as CMD_FAILURE, try and set the default value?
		}
		else
			status = CMD_FAILURE;
	}
	else
		status = CMD_FAILURE;

	return status;
}

/*
 * Helper functions to get and set the total number of files and folders on the SD card.
 */
void SetSDTotalFiles( int total_files )
{
	ConfigBuff.TotalFiles = total_files;
	return;
}

int GetSDTotalFiles( void )
{
	return ConfigBuff.TotalFiles;
}

void SetSDTotalFolders( int total_folders )
{
	ConfigBuff.TotalFolders = total_folders;
	return;
}

int GetSDTotalFolders( void )
{
	return ConfigBuff.TotalFolders;
}
/*
 * Record the state of the SD card. This function gets the values of the total number of files and folders
 *  which are recorded by the RecordFiles module.
 *
 * @param	none
 *
 * @return
 */
int RecordSDState( void )
{
	int status = 0;

	//get the values from the RecordFiles module
	ConfigBuff.TotalFiles = SDGetTotalFiles();
	ConfigBuff.TotalFolders = SDGetTotalFolders();
	status = SaveConfig();
	if(status == FR_OK)
		status = CMD_SUCCESS;
	else
		status = CMD_FAILURE;

	return status;
}

/*
 * Keep track of the most recently input Real Time from the spacecraft.
 * The Real Time is a 32-bit unsigned integer type which represents a time value and will be used
 *  to track when operations happened on the Mini-NS, as well as time tagging when the system state
 *  is reported either with DIR, LOG, or CFG.
 *
 * @param	(unsigned int)the Real Time from the spacecraft
 *
 * @return	(int)status variable, success/failure
 */
int SetRealTime( unsigned int real_time )
{
	int status = 0;

	if(real_time < ConfigBuff.MostRecentRealTime)
		status = 1;

	ConfigBuff.MostRecentRealTime = real_time;

	return status;
}

/*
 * Helper function to allow for retrieving the most Recent Real Time.
 * As long as we are setting this whenever we get it, this doesn't have to be labeled as "Most Recent"
 *
 * @param	none
 *
 * @return	(unsigned int)the value of the most recent Real Time recorded in the configuration file
 */
unsigned int GetRealTime( void )
{
	return ConfigBuff.MostRecentRealTime;
}

/*
 * Applies all the settings which are currently recorded in the configuration file. This goes one-by-one
 *  through all the different parameters that can be set and makes sure they get updated.
 * NOTE: Make sure to call MNS_ENABLE_ACT before this to enable power to the analog board. Otherwise, the
 *  High Voltage values will not be set on the pots.
 * NOTE: If any of the sub-functions called here fail, then the rest of the function calls will fail, which
 *  will result in those parameters not being update/set in the system. This function needs to be error checked
 *  better, perhaps, to try and prevent/recover that situation.
 *
 *  @param	(XIicPs *)pointer to the I2C instance which lets us talk to the HV pots
 *
 *  @return	(int)status variable, success/failure
 */
int ApplyDAQConfig( XIicPs * Iic )
{
	int status = CMD_SUCCESS;

	status = SetEnergyCalParam(ConfigBuff.ECalSlope, ConfigBuff.ECalIntercept);
	if(status == CMD_SUCCESS)
		status = SetTriggerThreshold(ConfigBuff.TriggerThreshold);
	if(status == CMD_SUCCESS)
		status = SetIntegrationTime(ConfigBuff.IntegrationBaseline, ConfigBuff.IntegrationShort, ConfigBuff.IntegrationLong, ConfigBuff.IntegrationFull);
	if(status == CMD_SUCCESS)
		status = SetHighVoltage(Iic, 0, ConfigBuff.HighVoltageValue[0]);
	if(status == CMD_SUCCESS)
		status = SetHighVoltage(Iic, 1, ConfigBuff.HighVoltageValue[1]);
	if(status == CMD_SUCCESS)
		status = SetHighVoltage(Iic, 2, ConfigBuff.HighVoltageValue[2]);
	if(status == CMD_SUCCESS)
		status = SetHighVoltage(Iic, 3, ConfigBuff.HighVoltageValue[3]);
	//set n cuts
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(0, 1, ConfigBuff.SF_E[0], ConfigBuff.SF_PSD[0], ConfigBuff.Off_E[0], ConfigBuff.Off_PSD[0]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(0, 2, ConfigBuff.SF_E[1], ConfigBuff.SF_PSD[1], ConfigBuff.Off_E[1], ConfigBuff.Off_PSD[1]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(1, 1, ConfigBuff.SF_E[2], ConfigBuff.SF_PSD[2], ConfigBuff.Off_E[2], ConfigBuff.Off_PSD[2]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(1, 2, ConfigBuff.SF_E[3], ConfigBuff.SF_PSD[3], ConfigBuff.Off_E[3], ConfigBuff.Off_PSD[3]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(2, 1, ConfigBuff.SF_E[4], ConfigBuff.SF_PSD[4], ConfigBuff.Off_E[4], ConfigBuff.Off_PSD[4]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(2, 2, ConfigBuff.SF_E[5], ConfigBuff.SF_PSD[5], ConfigBuff.Off_E[5], ConfigBuff.Off_PSD[5]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(3, 1, ConfigBuff.SF_E[6], ConfigBuff.SF_PSD[6], ConfigBuff.Off_E[6], ConfigBuff.Off_PSD[6]);
	if(status == CMD_SUCCESS)
		status = SetNeutronCutGates(3, 2, ConfigBuff.SF_E[7], ConfigBuff.SF_PSD[7], ConfigBuff.Off_E[7], ConfigBuff.Off_PSD[7]);

	//TODO: error check

	return status;
}

