//==========================================================================================
//
//    File Name:      main.c
//    Description:    Illustrates use of the InterSense driver API
//    Created:        1998-12-07
//    Last updated:   2014-05-15
//    Authors:        Yury Altshuler, Rand Kmiec
//    Copyright:      InterSense 2014 - All rights Reserved.
//
//    Comments:       This program illustrates the use of many of the functions
//                    defined in isense.h. It includes logging functionality that
//                    enables data from one or more trackers/stations to be logged to
//                    a CSV file.  Please note that this sample uses extended data;
//                    for many applications, this is not needed, and retrieving it
//                    may result in reduced update rates when using even a single
//                    tracker over a serial port connection.
//                    
//==========================================================================================

#include <stdio.h> 
#include <stdlib.h> 
#include <memory.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
#include <conio.h>
#endif

// Needed to write the Unix equivalent of kbhit()
#if defined(UNIX)
#include <termios.h>
#include <unistd.h>
#endif

#include "isense.h"

#define ESC 0x1B
#define VER "1.1.0"

// DLL version, set once the first tracker is detected
float libVersion = -1;

//==========================================================================================
const char *systemType( int Type ) 
{
	switch( Type ) 
	{
	case ISD_NONE:
		return "Unknown";
	case ISD_PRECISION_SERIES:
		return "IS Precision Series";
	case ISD_INTERTRAX_SERIES:
		return "InterTrax Series";
	}
	return "Unknown";
}

//==========================================================================================
//
//  Configure defaults for a tracker, on all stations
//
//==========================================================================================
void configureStations( ISD_TRACKER_HANDLE currentTrackerH, ISD_STATION_INFO_TYPE *Stations, 
					    WORD validStation[ISD_MAX_TRACKERS][ISD_MAX_STATIONS])
{
	ISD_STATION_INFO_TYPE *currentStation;
	ISD_TRACKER_INFO_TYPE tracker;
	WORD i;

	for(i=1; i <= ISD_MAX_STATIONS; i++)
	{
		currentStation = &Stations[i-1];

		// Try to get the station configuration; if this fails then it'll be set to all
		// zero values
		if( ISD_GetStationConfig( currentTrackerH, currentStation, i, FALSE ))
		{
			// Obtain the library version from the first valid tracker (if the station
			// is valid, so is its tracker
			if(libVersion < 0)
			{
				ISD_GetTrackerConfig( currentTrackerH, &tracker, FALSE );
				libVersion = tracker.LibVersion;
			}

			// Set default configuration for each station
			// NOTE: Requesting extended data significantly increases the amount
			// of data sent back from the IS-900; if using this in your application
			// it is HIGHLY recommended to use Ethernet to ensure optimal data
			// record rates; even a single tracker may exhibit reduced data rates
			// when this is enabled over a serial port connection
			currentStation->GetExtendedData = TRUE;
			currentStation->GetAuxInputs = FALSE;
			currentStation->TimeStamped = TRUE;
			currentStation->GetInputs = FALSE;

			// Send the new configuration to the tracker 
			// (return value not checked since it's OK if this fails)
			ISD_SetStationConfig( currentTrackerH, currentStation, i, FALSE );

			if(currentStation->State == 1)
			{
				validStation[currentTrackerH-1][i-1] = 1;

				// Configure ring buffer for logging purposes
				// 180 samples is ~1 second at typical data rate for wired devices
				ISD_RingBufferSetup(currentTrackerH, i , NULL, 180);
				ISD_RingBufferStart(currentTrackerH, i);
			}
		}
	}
}

//==========================================================================================
//
//  Get and display tracker information
//
//==========================================================================================
void showTrackerStats( ISD_TRACKER_HANDLE Trackers[ISD_MAX_TRACKERS], ISD_TRACKER_HANDLE currentTrackerH, WORD currentStation, WORD logType, WORD recordsToSkip )
{
	ISD_TRACKER_INFO_TYPE				Tracker;
	ISD_STATION_INFO_TYPE				Station;
	ISD_HARDWARE_INFO_TYPE				hwInfo;
	ISD_STATION_INFO_TYPE				Stations[ISD_MAX_STATIONS];
	ISD_STATION_HARDWARE_INFO_TYPE		stationHwInfo[ISD_MAX_STATIONS];

	WORD i, j, thisStation, numStations = 4;
	DWORD maxStations;

	printf( "\n\n========================== Tracker Information ==========================\n" );
	printf( "Ver: %s, Lib: %0.4f. LogRatio: 1:%d. Press 'h' for help\nLOGGING: ", VER, libVersion, recordsToSkip+1 );
	
	switch(logType)
	{
		case 1:
			printf( "Current Tracker / Current Station (stationdata.log)" );
			break;

		case 2:
			printf( "Current Tracker / All Stations (stationsdata.log)" );
			break;

		case 3:
			printf( "All Trackers / All Stations (alldata.log)" );
			break;

		case 0:
		default:
			printf( "None" );
			break;
	}
	printf("\n\n");

	
	// Cycle through trackers first, then stations for each of those trackers, and display them
	for(i = 0; i <= ISD_MAX_TRACKERS; i++)
	{
		// Break out of loop once we no longer have a valid tracker (all trackers before a valid 
		// tracker in the array will be valid
		if(Trackers[i] < 1)
			break;

		// Get tracker configuration info 
		ISD_GetTrackerConfig( Trackers[i], &Tracker, FALSE );

		memset((void *) &hwInfo, 0, sizeof(hwInfo));

		if( ISD_GetSystemHardwareInfo( Trackers[i], &hwInfo ) )
		{
			if( hwInfo.Valid )
			{
				maxStations = hwInfo.Capability.MaxStations;
			}
		}

		// Clear station configuration info to make sure GetAnalogData and other flags are FALSE 
		memset( (void *) Stations, 0, sizeof(Stations) );

		// General procedure for changing any setting is to first retrieve current 
		// configuration, make the change, and then apply it. Calling 
		// ISD_GetStationConfig is important because you only want to change 
		// some of the settings, leaving the rest unchanged. 

		if( Tracker.TrackerType == ISD_PRECISION_SERIES )
		{
			for( thisStation = 1; thisStation <= maxStations; thisStation++ )
			{         
				// Fill ISD_STATION_INFO_TYPE structure with current station configuration 
				if( !ISD_GetStationConfig( Trackers[i], 
					&Stations[thisStation-1], thisStation, FALSE ) ) break;

				if( !ISD_GetStationHardwareInfo( Trackers[i], 
					&stationHwInfo[thisStation-1], thisStation ) ) break;
			}
		}

		if( ISD_GetTrackerConfig( Trackers[i], &Tracker, FALSE ) )
		{
			printf( "[%c] Tracker %d, port %d: %s\n", 
				Trackers[i] == currentTrackerH ? '*': ' ', i+1, Tracker.Port,
				hwInfo.Valid ? hwInfo.ModelName : "Unknown Tracker");

			switch( Tracker.TrackerModel ) 
			{
			case ISD_IS300:
			case ISD_IS1200:
				numStations = 4;
				break;
			case ISD_IS600:
			case ISD_IS900:
				numStations = ISD_MAX_STATIONS;
				break;
			default:
				numStations = 1;
				break;
			}

			printf( "  Sta Serial  FWVer TStamp State Enh. Sens. Comp. Pred.\n" );
			for(j = 1; j <= numStations; j++)
			{
				if( stationHwInfo[j-1].Valid) // Skip invalid stations
				{
					if( ISD_GetStationConfig( Trackers[i], &Station, j, FALSE ))
					{
						if(Station.State == 1) // Don't display stations that are not connected
						{
							if(currentStation==j && currentTrackerH-1==i)
								printf(">");
							else
								printf(" ");

							printf( " %-4d", j );
							printf( "%-8u%-6g%-8s%-5s%-5u%-6u%-6u%-5u\n",
								stationHwInfo[j-1].SerialNum,
								stationHwInfo[j-1].FirmwareRev, 
								Station.TimeStamped ? "ON" : "OFF", 
								Station.State ? "ON" : "OFF", 
								Station.Enhancement, 
								Station.Sensitivity, 
								Station.Compass, 
								Station.Prediction );
						}
					}
				}
			}
		}
		else
		{
			printf("ISD_GetTrackerConfig() failed\n");
		}
		printf("\n");
	}
}


//==========================================================================================
//
//  Log Tracker/Station data; one line at a time
//
//  The log format consists of 3 sections of metadata (for the log itself, the
//  trackers, andeach of the tracker stations), followed by a header row and
//  lines of data for each of the stations being recorded:
//
//		tracker number
//		station number (within a given tracker)
//		x/y/z (centimeters)
//		yaw/pitch/roll (degrees)
//		timestamp (seconds)
//		accurate_timestamp [double precision] (seconds)
//		accurate_os_timestamp [double precision] (seconds)
//		tracking quality (%)
//		communication integrity (%)
//		measurement quality (%)
//		extended yaw/pitch/roll rate (radians/second)
//		extended x/y/z accel (meters/second^2)
//		extended compass heading (degrees)
//		joystick axis 1 (1 byte)
//		joystick axis 2 (1 byte)
//		button data (1 byte)
//      AUX input data (byte 0)
//      AUX input data (byte 1)
//      AUX input data (byte 2)
//      AUX input data (byte 3)
//		still time (float)
//      battery voltage (float)
//      temperature (float)
//==========================================================================================
void logData(ISD_TRACKER_HANDLE Trackers[ISD_MAX_TRACKERS], ISD_STATION_DATA_TYPE *data, 
			 WORD trackerNum, WORD stationNum, FILE *fp,
			 ISD_STATION_INFO_TYPE *staInfo,
			 ISD_STATION_HARDWARE_INFO_TYPE	stationHwInfo[ISD_MAX_TRACKERS][ISD_MAX_STATIONS])
{
	ISD_TRACKER_INFO_TYPE				Tracker;
	ISD_STATION_INFO_TYPE				Station;
	ISD_HARDWARE_INFO_TYPE				hwInfo;
	ISD_STATION_INFO_TYPE				Stations[ISD_MAX_STATIONS];

	static double						osLibTimeDiff = 0.0;
	static WORD							recordsSkipped = 0;
	WORD								i, j, thisStation, numOpenTrackers = 0, numStations = 1;
	DWORD								maxStations;
	BYTE								buttons = 0;
	time_t								now;

	// Initial write to the file:
	// First metadata, then actual logged data.  The metadata provides information about what device(s)
	// was/were logged, which makes it much easier to remember all of the specific settings that applied
	// when the data was taken
	if(ftell(fp) == 0)
	{
		// Only get the time the first time through the log
		time(&now);

		// Determine the number of currently open trackers
		ISD_NumOpenTrackers(&numOpenTrackers);

		// Print program information
		fprintf(fp,"[BEGIN LOG INFO]\n");
		fprintf(fp,"Version,LogDate\n");
		fprintf(fp,"%s,%s", VER, asctime(localtime(&now)) );
		fprintf(fp,"[END LOG INFO]\n\n");

		// Print tracker information
		fprintf(fp,"[BEGIN TRACKER INFO]\n");
		fprintf(fp,"TrackerNum,LibVersion,TrackerType,TrackerModel,Port,SyncState,SyncRate,SyncPhase,");
		fprintf(fp,"Interface,UltTimeout,UltVolume,FirmwareRev,LedEnable\n");
		for(i = 0; i < numOpenTrackers; i++)
		{
			if( ISD_GetTrackerConfig( Trackers[i], &Tracker, FALSE ) )
			{
				fprintf(fp,"%u,%3.4f,%u,%u,%u,%u,%f,%u,%u,%u,%u,%3.4f,%u\n",
					i+1,Tracker.LibVersion,Tracker.TrackerType,Tracker.TrackerModel,
					Tracker.Port,Tracker.SyncState,Tracker.SyncRate,Tracker.SyncPhase,
					Tracker.Interface,Tracker.UltTimeout,Tracker.UltVolume,Tracker.FirmwareRev,
					Tracker.LedEnable);
			}
			else
			{
				fprintf(fp,"ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR\n");
			}
		}
		fprintf(fp,"[END TRACKER INFO]\n\n");

		// Print station information
		fprintf(fp,"[BEGIN STATION INFO]\n");
		fprintf(fp,"TrackerNum,StationNum,Serial,FW,StationType,Descriptor,CalDate,Port,");
		fprintf(fp,"Timestamp,State,Enhancement,Sensitivity,Compass,Prediction\n" );

		for(i = 0; i < numOpenTrackers; i++)
		{
			if( ISD_GetTrackerConfig( Trackers[i], &Tracker, FALSE ) )
			{
				if( ISD_GetSystemHardwareInfo( Trackers[i], &hwInfo ) )
				{
					if( hwInfo.Valid )
					{
						maxStations = hwInfo.Capability.MaxStations;
					}
				}

				if( Tracker.TrackerType == ISD_PRECISION_SERIES )
				{
					for( thisStation = 1; thisStation <= maxStations; thisStation++ )
					{         
						// Fill ISD_STATION_INFO_TYPE structure with current station configuration 
						if( !ISD_GetStationConfig( Trackers[i], 
							&Stations[thisStation-1], thisStation, FALSE ) ) break;
					}
				}

				switch( Tracker.TrackerModel ) 
				{
				case ISD_IS300:
				case ISD_IS1200:
					numStations = 4;
					break;
				case ISD_IS600:
				case ISD_IS900:
					numStations = ISD_MAX_STATIONS;
					break;
				default:
					numStations = 1;
					break;
				}

				printf("");

				for(j = 0; j < numStations; j++)
				{
					if( stationHwInfo[i][j].Valid == 1)
					{
						if( ISD_GetStationConfig( Trackers[i], &Station, j+1, FALSE ))
						{
							if(Station.State == 1) // Don't log stations that are not connected
							{
								fprintf(fp,"%u,%u,%u,%g,%u,%s,%s,%u,%s,%s,%u,%u,%u,%u\n",
									i+1, j+1,
									stationHwInfo[i][j].Valid ? (&stationHwInfo[i][j])->SerialNum : -1,
									stationHwInfo[i][j].Valid ? (&stationHwInfo[i][j])->FirmwareRev : -1, 
									stationHwInfo[i][j].Type,
									stationHwInfo[i][j].DescVersion,
									stationHwInfo[i][j].CalDate,
									stationHwInfo[i][j].Port,
									Station.TimeStamped ? "ON" : "OFF",
									Station.State ? "ON" : "OFF",
									Station.Enhancement,
									Station.Sensitivity, 
									Station.Compass, 
									Station.Prediction );
							}
						}
						else
						{
							fprintf(fp,"%u,%u,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR\n",
								i+1,j+1);
						}
					}
				}
			}
			else
			{
				fprintf(fp,"%u,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR\n",i+1);
			}
		}
		fprintf(fp,"[END STATION INFO]\n");
		fprintf(fp,"\n");

		fprintf(fp,"TrackerNum,StationNum,X,Y,Z,Yaw,Pitch,Roll,Time,DoubleTime,DoubleOSTime,TQ,CI,MQ,GXBF,");
		fprintf(fp,"GYBF,GZBF,GXNF,GYNF,GZNF,GXRAW,GYRAW,GZRAW,AXBF,AYBF,AZBF,AXNF,AYNF,AZNF," );
		fprintf(fp,"MagX,MagY,MagZ,CompassYaw,JoystickAxis1,");
		fprintf(fp,"JoystickAxis2,Buttons,AuxIn0,AuxIn1,AuxIn2,AuxIn3,StillTime,Vbatt,Temperature\n" );
	}

	// Set the initial logged timestamp
	if(osLibTimeDiff == 0.0)
		osLibTimeDiff = data->OSTimeStampSeconds + data->OSTimeStampMicroSec * 1.0e-6 - data->TimeStamp;

	// Tracker and station identification numbers
	fprintf( fp, "%d,%d,", trackerNum + 1, stationNum );

	// X, Y, Z (m)
	fprintf( fp, "%.5f,%.5f,%.5f,",
				data->Position[0], data->Position[1], data->Position[2]);

	// Yaw, pitch, roll (degrees)
	fprintf( fp, "%.3f,%.3f,%.3f,",
				data->Euler[0], data->Euler[1], data->Euler[2]);

	// Timestamp (seconds)
	fprintf( fp, "%.4f,", data->TimeStamp);

	// Double precision sensor timestamp (seconds)
	fprintf( fp, "%.4f,", (double)data->TimeStampSeconds + 
						  (double)data->TimeStampMicroSec * 1.0e-6);

	// Double precision OS timestamp, offset to coincide with DLL timestamp (seconds)
	fprintf( fp, "%.4f,", (double)data->OSTimeStampSeconds + 
						  (double)data->OSTimeStampMicroSec * 1.0e-6 - osLibTimeDiff);

	// TQ, CI, MQ
	fprintf( fp, "%d,%d,%d,",
		(int)(data->TrackingStatus/2.55), data->CommIntegrity, data->MeasQuality);

	// AngularVelBodyFrame (rad/sec)
	fprintf( fp, "%.5f,%.5f,%.5f,",
		data->AngularVelBodyFrame[0], data->AngularVelBodyFrame[1], data->AngularVelBodyFrame[2]);

	// AngularVelNavFrame (rad/sec)
	fprintf( fp, "%.5f,%.5f,%.5f,",
		data->AngularVelNavFrame[0], data->AngularVelNavFrame[1], data->AngularVelNavFrame[2]);

	// AngularVelRaw (rad/sec)
	fprintf( fp, "%.5f,%.5f,%.5f,",
		data->AngularVelRaw[0], data->AngularVelRaw[1], data->AngularVelRaw[2]);

	// AccelBodyFrame (m/s^2)
	fprintf( fp, "%.5f,%.5f,%.5f,",
		data->AccelBodyFrame[0], data->AccelBodyFrame[1], data->AccelBodyFrame[2]);

	// AccelNavFrame (m/s^2)
	fprintf( fp, "%.5f,%.5f,%.5f,",
		data->AccelNavFrame[0], data->AccelNavFrame[1], data->AccelNavFrame[2]);

	// Magnetometer data (Gauss)
	fprintf( fp, "%.5f,%.5f,%.5f,",
		data->MagBodyFrame[0], data->MagBodyFrame[1], data->MagBodyFrame[2]);

	// Compass yaw, from magnetometers (degrees)
	fprintf( fp, "%.3f,",
		data->CompassYaw ); 

	// Joystick axis 1,2 (0-255)
	if(staInfo->GetInputs)
	{
		fprintf( fp, "%u,%u,", data->AnalogData[0],data->AnalogData[1] ); 
	}
	else
	{
		fprintf( fp, "-1,-1," );
	}

	// Button data (written as a byte, as 8 inputs are allowed currently)
	if(staInfo->GetInputs)
	{
		for(i=0; i < ISD_MAX_BUTTONS; i++)
		{
			buttons |= data->ButtonState[i] << i;
		}
		fprintf( fp, "%u,", buttons );
	}
	else
		fprintf( fp, "-1," );

	// AUX input (input from station) data bytes
	if(staInfo->GetAuxInputs)
	{
		for(i=0; i < ISD_MAX_AUX_INPUTS; i++)
		{
			if(i < stationHwInfo[trackerNum][stationNum-1].Capability.AuxInputs)
				fprintf( fp, "%d,", data->AuxInputs[i] );
			else
				fprintf( fp, "-1," );
		}
	}
	else
	{
		fprintf( fp, "-1,-1,-1,-1," );
	}

	// Still time
	fprintf( fp, "%.4f,", data->StillTime );

	// Battery and temperature (3DOF sensors)
	fprintf( fp, "%.3f,%.3f", data->BatteryLevel, data->Temperature);
	
	// End of line
	fprintf( fp, "\n" );
}


//==========================================================================================
//
//  Display Tracker data
//
//==========================================================================================
void showStationData( ISD_TRACKER_HANDLE				currentTrackerH, 
					  ISD_TRACKER_INFO_TYPE				*Tracker,
					  ISD_STATION_INFO_TYPE				*Station,
					  ISD_STATION_DATA_TYPE				*data,
					  ISD_STATION_HARDWARE_INFO_TYPE	*stationHwInfo,
					  BYTE showTemp)
{
	DWORD i;

	// Get comm port statistics for display with tracker data 
	if( ISD_GetCommInfo( currentTrackerH, Tracker ) )
	{
		printf( "%3.0fKb %dR TQ/CI:%3d/%3d", 
			Tracker->KBitsPerSec, Tracker->RecordsPerSec, (int)(data->TrackingStatus/2.55),
			data->CommIntegrity );

		// display position only if system supports it 
		if( Tracker->TrackerModel == ISD_IS600 || 
			Tracker->TrackerModel == ISD_IS900 ||
			Tracker->TrackerModel == ISD_IS1200 )
		{
			printf( " (%5.0f,%5.0f,%4.0f)cm ",
				data->Position[0]*100.f, data->Position[1]*100.f, data->Position[2]*100.f );
		}

		// all products can return orientation 
		if( Station->AngleFormat == ISD_QUATERNION )
		{
			printf( "(%5.1f,%5.1f,%5.1f,%5.1f)quat ",
				data->Quaternion[0], data->Quaternion[1], 
				data->Quaternion[2], data->Quaternion[3] );
		}
		else // Euler angles
		{
			printf( "(%6.1f,%5.1f,%6.1f)deg ",
				data->Euler[0], data->Euler[1], data->Euler[2]);
		}

		if( Station->GetAuxInputs ) 
		{
			printf( "AUX:" );
			for( i=0; i < stationHwInfo->Capability.AuxInputs; i++ )
			{
				printf( "%02X", data->AuxInputs[i] );
			}
			printf( " " );
		}

		if( showTemp == TRUE )
		{
			printf( "%0.1fC ", data->Temperature );
		}

		// if system is configured to read stylus or wand buttons 
		if( Station->GetInputs ) 
		{
			// Currently available products have at most 6 buttons,
			printf( "JOY:" );
			
			for( i=0; i < stationHwInfo->Capability.NumButtons; i++ )
			{
				printf( "%d", data->ButtonState[i] );
			}

			for( i=0; i < stationHwInfo->Capability.NumChannels; i++ )
			{
				printf( " %02X", data->AnalogData[i] );
			}

			printf( " " );
		}

		printf( "%1.1fs ", data->TimeStamp );

		printf( "\r" );
		fflush(0);
	}
}

// Unix doesn't have kbhit(), so a similar function is needed
// Similarly nothing equivalent to getch(), since getc/getchar() require an enter press
#if defined(UNIX)

// Check to make sure getchar() won't block
int unix_kbhit(void)
{
	int i, ch;
	fd_set fds;
	struct timeval tv;
	struct termios t_new, t_old;

	tcgetattr( STDIN_FILENO, &t_new );
	t_old = t_new;								// Copy settings
	t_new.c_lflag &= ~ICANON;					// Turn off echo and buffering
	tcsetattr( STDIN_FILENO, TCSANOW, &t_new ); // Apply settings

	FD_ZERO(&fds);                              // Clear the fd_set
	FD_SET(STDIN_FILENO, &fds);                 // Watch STDIN for input
	tv.tv_sec = tv.tv_usec = 0;                 // 0 second timeout value
	i = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

	if (i != 0)
		ch = getchar();

	// Reload old settings
	tcsetattr( STDIN_FILENO, TCSANOW, &t_old );

	// If there is something typed so that getchar() won't block, return true, else return false
	if (i == -1) return (FALSE);                // Only returned on error
	if (i != 0) return (ch);    				// Returned if getchar() will be non-blocking

	return (FALSE);                             // Returned if getchar() would block
}
#endif

//==========================================================================================
//
// The main function shows how to initialize and obtain data from InterSense trackers. 
//
//==========================================================================================
int main()
{
	ISD_TRACKER_HANDLE              Trackers[ISD_MAX_TRACKERS];
	ISD_TRACKER_HANDLE				currentTrackerH;
	ISD_TRACKING_DATA_TYPE          data, alternateTrackerData;
	ISD_STATION_INFO_TYPE           Stations[ISD_MAX_STATIONS];
	ISD_STATION_HARDWARE_INFO_TYPE	StationsHwInfo[ISD_MAX_TRACKERS][ISD_MAX_STATIONS];
	ISD_TRACKER_INFO_TYPE           TrackerInfo;
	ISD_HARDWARE_INFO_TYPE			hwInfo;
	FILE 							*fpStation = NULL;
	FILE 							*fpStations = NULL;
	FILE 							*fpAll = NULL;
	FILE							*fpCurrent = NULL;

	// These are 1-based indexes specifying the currently selected tracker and station
	WORD							tracker = 1, station = 1;
	WORD							numRecordsToSkip = 0;
	WORD							numRecordsSkipped[ISD_MAX_TRACKERS][ISD_MAX_STATIONS];
	WORD							i, j, done = FALSE, trackerIdx = 0;
	WORD							getInputs = FALSE, logType = 0, numOpenTrackers = 0;
	BYTE							auxOut = 0;
	BYTE							showTemp = FALSE;
	DWORD							openSuccess = FALSE, maxStations = 4;

	// Array to keep track of valid stations: 0 for invalid stations, 1 for valid
	WORD							validStation[ISD_MAX_TRACKERS][ISD_MAX_STATIONS];

	float lastTime; 
#if defined UNIX
	char ch_key;
#endif

	// Detect all trackers, using ISD_OpenAllTrackers().  Note that all InertiaCube
	// products are considered "trackers" (even when on the same receiver for wireless
	// sensors), while stations connected to an IS-900 or similar product are "stations"
	//
	// For a single tracker, ISD_OpenTracker() may be used, although in most
	// cases ISD_OpenAllTrackers is preferable as it is more generic.
	// If you have more than one InterSense device and would like to have a specific
	// tracker, connected to a known port, initialized first, then enter the port 
	// number instead of 0. Otherwise, the tracker connected to the RS232 port with
	// lowest number is found first
	// 

	openSuccess = ISD_OpenAllTrackers( (Hwnd) NULL, Trackers, FALSE, TRUE );

	// Check value of currentTrackerH to see if at least one tracker was located 
	if( openSuccess < 1 )
	{
		printf( "Did not detect any InterSense tracking devices -- please verify COM ports have been created and are not in use\n" );
	}
	else
	{
		currentTrackerH = Trackers[0];

		// Determine the number of currently open trackers
		ISD_NumOpenTrackers(&numOpenTrackers);

		// Zero out the validStation array
		memset((void *) &validStation, 0, sizeof(validStation));

		// Clear station configuration info
		memset( (void *) Stations, 0, sizeof(Stations) );

		// Set up some default configurations that are generally useful for data logging/display
		// And turn on ring buffers for this tracker
		// Other trackers are configured when switching to them, which resets any state for that tracker
		printf( "Found/opened %d trackers\n", numOpenTrackers); 

		for( i=0; i < numOpenTrackers; i++ )
		{
			printf( "Configuring default settings and obtaining info for tracker %d\n", i+1); 
			configureStations(Trackers[i], Stations, validStation);

			// Get hardware information for each station on each tracker; this is used in later functions
			for( j=0; j < ISD_MAX_STATIONS; j++)
				ISD_GetStationHardwareInfo(Trackers[i], &StationsHwInfo[i][j], j+1);
		}

		// Get tracker configuration info
		currentTrackerH = Trackers[0];
		ISD_GetTrackerConfig( currentTrackerH, &TrackerInfo, TRUE );

		memset((void *) &hwInfo, 0, sizeof(hwInfo));

		if( ISD_GetSystemHardwareInfo( currentTrackerH, &hwInfo ) )
		{
			if( hwInfo.Valid )
			{
				maxStations = hwInfo.Capability.MaxStations;
			}
		}

		station = 1;
		lastTime = ISD_GetTime();

		// Move the selected station to the first valid station
		for( i=0; i < ISD_MAX_STATIONS; i++ )
		{
			if(validStation[0][i])
			{
				station = i+1;
				break;
			}
		}

		// Show information for all trackers, initially with first tracker/station selected:
		showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );

		while( !done )
		{
			// Keyboard handling functions
#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
			if( _kbhit() )
			{
				char inputChar = _getch();

				// Make the input character lowercase, if a letter (except for 'L', used in logging
				if(inputChar >= 'A' && inputChar <= 'Z' && inputChar != 'L')
					inputChar += 'a' - 'A';

				switch( inputChar )
#else
			ch_key = unix_kbhit();
			if( ch_key != FALSE )
			{
				switch( ch_key )
#endif
				{
				case ',':
					{
						BYTE AuxOutput[4];
						char buf[1024];
						int numOutputs = StationsHwInfo[currentTrackerH-1][station-1].Capability.AuxOutputs;

						if(numOutputs == 0)
						{
							printf( "\nThis station does not support AUX output, please check descriptor\n" );
						}
						else
						{
							printf( "\nPlease enter the values to send out in hex format, no leading '0x'\n" );
						
							for(i = 0; i < numOutputs; i++)
							{
								printf( "AUX%d (hex): 0x", i );
								fgets( buf, sizeof(buf), stdin );

								if(!sscanf( buf, "%x", &AuxOutput[i] ))
								{
									printf( "Invalid entry for AUX%d, using 00 instead\n", i );
									AuxOutput[i] = 0;
								}
							}

							ISD_AuxOutput( currentTrackerH, station, AuxOutput, numOutputs );
						}
					}
					break;
				
				case '/':
					{
						char buf[4096];
						
						printf( "\n***WARNING***\nThe library may not be aware of some changes made with\n" );
						printf( "this command; recommended only for testing and configuration of options not\n" );
						printf( "configurable using normal keyboard commands.\n\n" );
						printf( "Please enter a protocol command to send (4096 byte limit):\n" );
						
						fgets( buf, sizeof(buf), stdin );

						ISD_SendScript( currentTrackerH, buf );
					}
					break;

				case '<':
					// Clear out the line, in case the display used to be wider than it is now
					printf( "                                                                              \r" );
					fflush(0);
					ISD_GetStationConfig( currentTrackerH, &Stations[station-1], station, TRUE );
					Stations[station-1].GetAuxInputs = !Stations[station-1].GetAuxInputs;
					ISD_SetStationConfig( currentTrackerH, &Stations[station-1], station, TRUE );
					break;

				case '>':
					// Clear out the line, in case the display used to be wider than it is now
					printf( "                                                                              \r" );
					fflush(0);
					ISD_GetStationConfig( currentTrackerH, &Stations[station-1], station, TRUE );
					Stations[station-1].GetInputs = !Stations[station-1].GetInputs;
					ISD_SetStationConfig( currentTrackerH, &Stations[station-1], station, TRUE );					
					getInputs = !getInputs;
					break;

				case '[':
					// Increase the number of records to skip (useful for reducing the size of the recorded file)
					if(numRecordsToSkip < USHRT_MAX)
						numRecordsToSkip++;
					memset((void *) &numRecordsSkipped, 0, sizeof(numRecordsSkipped));
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case ']':
					// Decrease the number of records to skip (useful for recording a larger portion of the data)
					if(numRecordsToSkip > 0)
						numRecordsToSkip--;
					memset((void *) &numRecordsSkipped, 0, sizeof(numRecordsSkipped));
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case 'm':
					// Clear out the line, in case the display used to be wider than it is now
					printf( "                                                                              \r" );
					fflush(0);
					
					if(showTemp)
						showTemp = FALSE;
					else
						showTemp = TRUE;
					break;

				case 'e': // IS-x products only, not for InterTrax 

					// First get current station configuration 
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, TRUE ) )
					{
						Stations[station-1].Enhancement = (Stations[station-1].Enhancement+1) % 3;

						Stations[station-1].GetInputs = getInputs;

						// Send the new configuration to the tracker 
						if( ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, TRUE ) )
						{
							// display the results 
							showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
						}
					}
					break;

				case 't':
					// First get current station configuration
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, TRUE ) )
					{
						Stations[station-1].TimeStamped = !(Stations[station-1].TimeStamped);

						// Send the new configuration to the tracker
						if( ISD_SetStationConfig( currentTrackerH,
							&Stations[station-1], station, TRUE ) )
						{
							// display the results
							showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
						}
					}

					break;

				case 'd':
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case 'c': // IS-x products only, not for InterTrax

					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, TRUE ))
					{
						Stations[station-1].Compass = (Stations[station-1].Compass + 1) % 3;

						if( ISD_SetStationConfig( currentTrackerH,
							&Stations[station-1], station, TRUE ) )
						{
							showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
						}
					}
					break;

				case 'p': // IS-x products only, not for InterTrax 

					if( ISD_GetStationConfig( currentTrackerH, 
						&Stations[station-1], station, TRUE ))
					{
						Stations[station-1].Prediction = (Stations[station-1].Prediction + 10) % 60;

						if( ISD_SetStationConfig( currentTrackerH,
							&Stations[station-1], station, TRUE ) )
						{
							showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
						}
					}
					break;
				case 's': // IS-x products only, not for InterTrax 

					if( ISD_GetStationConfig( currentTrackerH, 
						&Stations[station-1], station, TRUE ))
					{
						Stations[station-1].Sensitivity = (Stations[station-1].Sensitivity + 1) % 5;

						if( Stations[station-1].Sensitivity == 0)
							Stations[station-1].Sensitivity = 1;

						if( ISD_SetStationConfig( currentTrackerH,
							&Stations[station-1], station, TRUE ) )
						{
							showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
						}
					}
					break;

				case 'n': // Cycle the currently selected tracker
					
					// Stop logging if we are (same thing as 'x'), unless the log
					// type is 'all'
					if(logType != 3)
					{
						logType = 0;

						// Close any open files
						if(fpStation)
						{
							fclose(fpStation);
							fpStation = NULL;
						}

						if(fpStations)
						{
							fclose(fpStations);
							fpStations = NULL;
						}

						if(fpAll)
						{
							fclose(fpAll);
							fpAll = NULL;
						}

						fpCurrent = NULL;
					}

					// Clear station configuration info to make sure GetAnalogData and other flags are FALSE 
					memset( (void *) Stations, 0, sizeof(Stations) );

					if(Trackers[trackerIdx+1] < 1)
						trackerIdx = 0;
					else
						trackerIdx++;
					
					currentTrackerH = Trackers[trackerIdx];
					station = 1;  // Revert to the first station on tracker switches

					// Reconfigure stations
					configureStations(currentTrackerH, Stations, validStation);

					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case 'r':
					ISD_ResetHeading( currentTrackerH, station );
					break;

				case 'b':
					ISD_Boresight( currentTrackerH, station, TRUE );
					break;

				case 'u':
					ISD_Boresight( currentTrackerH, station, FALSE );
					break;

				case 'x':
					logType = 0;

					// Close any open files
					if(fpStation)
					{
						fclose(fpStation);
						fpStation = NULL;
					}

					if(fpStations)
					{
						fclose(fpStations);
						fpStations = NULL;
					}

					if(fpAll)
					{
						fclose(fpAll);
						fpAll = NULL;
					}

					fpCurrent = NULL;
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );

					// Reset the number of skipped records for all stations
					memset((void *) &numRecordsSkipped, 0, sizeof(numRecordsSkipped));
					break;

				case 'l':
					logType = 1;

					if(!fpStation)
						fpStation = fopen("stationdata.log","w");
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case 'L':
					logType = 2;

					if(!fpStations)
						fpStations = fopen("stationsdata.log","w");
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case 'a':
					logType = 3;

					if(!fpAll)
						fpAll = fopen("alldata.log","w");
					showTrackerStats( Trackers, currentTrackerH, station, logType, numRecordsToSkip );
					break;

				case '1': // IS-x products only, not for InterTrax 

					station = 1;
					printf("\n>> Current Station is set to %d <<\n", station);

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '2': // IS-x products only, not for InterTrax 

					if( maxStations > 1 ) 
					{
						station = 2;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '3': // IS-x products only, not for InterTrax 

					if( maxStations > 2 ) 
					{
						station = 3;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '4': // IS-x products only, not for InterTrax 

					if( maxStations > 3 ) 
					{
						station = 4;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '5': // IS-x products only, not for InterTrax

					if( maxStations > 4 )
					{
						station = 5;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '6': // IS-x products only, not for InterTrax

					if( maxStations > 5 )
					{
						station = 6;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '7': // IS-x products only, not for InterTrax

					if( maxStations > 6 )
					{
						station = 7;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case '8': // IS-x products only, not for InterTrax

					if( maxStations > 7 )
					{
						station = 8;
						printf("\n>> Current Station is set to %d <<\n", station);
					}

					// Request extended data, don't get AUX or joystick inputs unless requested later
					if( ISD_GetStationConfig( currentTrackerH,
						&Stations[station-1], station, FALSE ) )
					{
						Stations[station-1].GetExtendedData = TRUE;

						// Send the new configuration to the tracker (return not checked since it's OK if this fails)
						ISD_SetStationConfig( currentTrackerH, 
							&Stations[station-1], station, FALSE );
					}
					break;

				case ESC:
				case 'q':
					printf( "\n\n" );
					done = TRUE;
					break;

				case 'h':
				default:
					printf( "\n\n" );
					printf( "h -- Display this help text\n" );
					printf( "=========================================\n" );
					printf( "[1-8] -- Make station number current\n" );
					printf( "n -- Cycle tracker number\n" );
					printf( "=========================================\n" );
					printf( "l -- Log current tracker / current station to 'stationdata.log' (overwrites)\n" );
					printf( "L -- Log current tracker / all stations to 'stationsdata.log' (overwrites)\n" );
					printf( "a -- Log all trackers / all stations to 'alldata.log' (overwrites)\n" );
					printf( "x -- Stop logging (log file NOT deleted)\n" );
					printf( "=========================================\n" );
					printf( "< -- Show/hide AUX data (hex)\n" );
					printf( "> -- Show/hide joystick data (binary, hex)\n" );
					printf( "m -- Show/hide temperature\n" );
					printf( ", -- Set AUX out bytes (hex)\n" );
					printf( "[ -- Reduce recording rate (skip 1 more record)\n" );
					printf( "] -- Increase recording rate (skip 1 less record)\n" );
					printf( "=========================================\n" );
					printf( "d -- Display current settings\n" );
					printf( "e/p/c/s -- Cycle perceptual enhancement, prediction, compass, sensitivity\n" );
					printf( "t -- Toggle timestamps\n" );
					printf( "r -- Reset heading (3DOF only)\n" );
					printf( "b/u -- Full boresight/unboresight (all trackers)\n" );
					printf( "/ -- ISD_SendScript() (send protocol commands)\n" );
					printf( "q -- Quit application\n" );
					break;
				}
			}

			if( currentTrackerH > 0 )
			{
				// Logging: Single station
				if(logType == 1 && fpStation)
				{
					ISD_GetTrackingData( currentTrackerH, &data );

					while(data.Station[station-1].NewData == TRUE)
					{
						if(numRecordsSkipped[currentTrackerH-1][station-1] < numRecordsToSkip)
						{
							numRecordsSkipped[currentTrackerH-1][station-1]++;
						}
						else
						{
							numRecordsSkipped[currentTrackerH-1][station-1] = 0;
							logData(Trackers,&data.Station[station-1],trackerIdx,
							    station,fpStation,&Stations[station-1],StationsHwInfo);
						}
						ISD_GetTrackingData( currentTrackerH, &data );
					}
				}
				// Logging: All stations on selected tracker
				else if(logType == 2 && fpStations)
				{
					ISD_GetTrackingData( currentTrackerH, &data );

					while(data.Station[station-1].NewData == TRUE)
					{
						for(j=0; j < ISD_MAX_STATIONS; j++)
						{
							if(numRecordsSkipped[currentTrackerH-1][j] < numRecordsToSkip)
							{
								numRecordsSkipped[currentTrackerH-1][j]++;
							}
							else
							{
								numRecordsSkipped[currentTrackerH-1][j] = 0;
								if(validStation[trackerIdx][j])
									logData(Trackers,&data.Station[j],trackerIdx,
											j+1,fpStations,&Stations[station-1],StationsHwInfo);
							}
						}
						ISD_GetTrackingData( currentTrackerH, &data );
					}
				}
				// Logging: All stations on all trackers
				else if(logType == 3 && fpAll)
				{
					for(i=0; i < numOpenTrackers; i++)
					{
						// Get all data for all trackers (into alternateTrackerData, 
						// so we don't mess up the display)
						ISD_GetTrackingData( Trackers[i], &alternateTrackerData );

						while(alternateTrackerData.Station[station-1].NewData == TRUE)
						{
							for(j=0; j < ISD_MAX_STATIONS; j++)
							{
								if(numRecordsSkipped[i][j] < numRecordsToSkip)
								{
									numRecordsSkipped[i][j]++;
								}
								else
								{
									numRecordsSkipped[i][j] = 0;
									if(validStation[i][j])
										logData(Trackers,&alternateTrackerData.Station[j],i,
												j+1,fpAll,&Stations[station-1],StationsHwInfo);
								}
							}
							ISD_GetTrackingData( Trackers[i], &alternateTrackerData );
						}
					}
				} else { // Logging is turned off

					// Get the latest data for display purposes, since we're using the Ring Buffer
					ISD_GetTrackingData( currentTrackerH, &data );
					
					while(data.Station[station-1].NewData == TRUE)
					{
						ISD_GetTrackingData( currentTrackerH, &data );
					}
				}

				// Get the data one more time for display purposes, if logging
				ISD_GetTrackingData( currentTrackerH, &data );
			}

			// Data display from current station
			if( ISD_GetTime() - lastTime > 0.05f )
			{
				lastTime = ISD_GetTime();

				if( currentTrackerH > 0 )
				{
					showStationData( currentTrackerH, &TrackerInfo,
									 &Stations[station-1], &data.Station[station-1],
									 &StationsHwInfo[currentTrackerH-1][station-1],
									 showTemp);
				}
			}
#ifdef _WIN32
			Sleep( 2 );
#elif defined UNIX
			usleep(5e3);
#endif
		}

		printf( "Closing ismain application\n" );
		ISD_CloseTracker( currentTrackerH );

		// Close any open files
		if(fpStation)
			fclose(fpStation);

		if(fpStations)
			fclose(fpStations);

		if(fpAll)
			fclose(fpAll);
		exit(0);
	}
}
