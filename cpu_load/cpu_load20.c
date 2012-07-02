#include "cpu_load.h"

/**
 * adapted from http://www.linuxquestions.org/questions/programming-9/proc-stat-file-problem-for-cpu-usage-369302/#post3580984
 */






// determines if the main program loop should keep running
static bool keepRunning = true;



void intHandler(int notUsed) 
{
	printf("\n\n\"CTRL + C caught, exiting gracefully... ");
	keepRunning = false;
}



void error(const char *msg) 
{
    perror(msg);
    exit(0);
} //endmethod error



/**
 * returns a double value which represents the current time like this: seconds.microseconds
 */
double time_so_far() 
{
	// this structure contains the seconds and the microseconds into the current second
	struct timeval tp;

	// get the current time ()
	if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
		perror("gettimeofday");
  
	// create one time value from seconds and microseconds
	return ((double) (tp.tv_sec)) + (((double) tp.tv_usec) * 0.000001 );
} //endmethod time_so_far



/**
 * used to scale a value
 */
int scaleFromTo(int iValue, int iScaleFrom, int iScaleTo)
{
	return ((iValue*iScaleTo/iScaleFrom));
} //endmethid scaleFromTo



/**
 * used to generate the bit stream as hex values
 */
void generateBitStream(int iValue, int * bit_stream_container)
{

	// set all to 0 just to be safe
	bit_stream_container[0] = 0;
	bit_stream_container[1] = 0;
	bit_stream_container[2] = 0;
	bit_stream_container[3] = 0;
	bit_stream_container[4] = 0;


	int i = 0;
	int iArrayIndex = -1;


	// generate '1's
	for(i = 0; i < iValue; i++)
	{

		// do we need to increment the array index to write into the next chat?
		if ( i % 8 == 0 )
		{
			iArrayIndex++;
		} //endif

		// add a '1'
		//printf("\n\niArrayIndex: %i\n", iArrayIndex);
		//printf("before shift: %i\n", bit_stream_container[iArrayIndex]);
		bit_stream_container[iArrayIndex] = bit_stream_container[iArrayIndex] << 1;
		//printf("after shift: %i\n",  bit_stream_container[iArrayIndex]);
		bit_stream_container[iArrayIndex] += 1;
		//printf("after +1: %i\n",     bit_stream_container[iArrayIndex]);

	} //endfor


	/*printf("result: %i %i %i %i %i\n", 
			bit_stream_container[4], 
			bit_stream_container[3], 
			bit_stream_container[2], 
			bit_stream_container[1], 
			bit_stream_container[0]);
*/

} //endmethod generateBitStream



char hex[16]={"0123456789ABCDEF"};
void bitStreamToHex(int * cDecimcal, char * cHex)
{
	
	int i;
	for(i = 0; i < 5; i++)
	{
		sprintf(&cHex[i * 2], "%02X", cDecimcal[i]);
	}

} //endmethod bitStreamToHex



/**
 * main program logic
 */
int main(int argc, char *argv[]) 
{
  
	FILE *fhProcStat;	// file handle for /proc/stat
	FILE *fhOutput;		// the file we want to write to


	// needed time values 
	double dOldTime = -1;	// will hold the time from the previous iteration
	double dNewTime = -1;	// will hold the current time
	

	char cpu_text[10];	// needed for running fscanf on the first row of /proc/stat

	int  bit_stream_dec[5]; // will hold the bit stream we want to send as decimal values
	char bit_stream_hex[10];// will hold the bit stream we want to send as hex values

	// previous iteration
	int iUserTimeOld;	// user: normal processes executing in user mode
	int iNiceTimeOld;	// nice: niced processes executing in user mode
	int iSystemTimeOld;	// system: processes executing in kernel mode

	// current iteration
	int iUserTimeNew; 	// user: normal processes executing in user mode
	int iNiceTimeNew;	// nice: niced processes executing in user mode
	int iSystemTimeNew;	// system: processes executing in kernel mode

	bool bIsFirstIteration = true;

	int iCpuTimeTotal   = -1;	
	int iCpuUsage       = -1;	// calculated cpu usage
	int iCpuUsageScaled = -1;	// cpu usage scaled to 40 since we have 40 leds


	// set up interrupt handler so we can exit gracefully	
	signal(SIGINT, intHandler);
	


	// check if the output file was supplied
	if(argc != 2)
	{
		/* We print argv[0] assuming it is the program name */
		printf( "usage: %s /some/file \n", argv[0] );
		return 0;
	}

	
	// open the output file for writing
	fhOutput = fopen(argv[1], "w");
	

	while(keepRunning)
	{
		
		// get the current time with microsecond accuracy (seconds.microseconds)
		dNewTime = time_so_far();
		
		// re-open the file to read the new values
		fhProcStat = fopen("/proc/stat", "r");
		
		/* we're only interested in the first line since it aggregates the values 
		 * from the individual cores
		 * 1st line example: cpu  54095 7770 22351 2252753 27278 0 1483 0 0 0
		 *
		 * we are only interested in the first 3 values. these represent CPU time spent for:
		 * - user: normal processes executing in user mode
		 * - nice: niced processes executing in user mode
		 * - system: processes executing in kernel mode
		 *
		 * more info at: http://kernel.org/doc/Documentation/filesystems/proc.txt
		 */
		fscanf(fhProcStat, "%s\t%d\t%d\t%d\n", cpu_text, 
			&iUserTimeNew, &iNiceTimeNew, &iSystemTimeNew);

		// close file again
		fclose(fhProcStat);

	
		// we can't calculate cpu usage in the first iteration
		if(bIsFirstIteration)
		{
			bIsFirstIteration = false;
		}
		else
		{
			// calculate the total cpu time for this iteration		
			iCpuTimeTotal = (iUserTimeNew + iNiceTimeNew + iSystemTimeNew) - (iUserTimeOld + iNiceTimeOld + iSystemTimeOld);
	
			// calculate cpu usage in percent
			iCpuUsage = (iCpuTimeTotal / ((dNewTime - dOldTime) * 100)) * 100;

			// write to stdout	
			//printf("cpu usage: %i%\n", iCpuUsage);
			printf("%i\n", iCpuUsage);

			// scale to 40 
			iCpuUsageScaled = scaleFromTo(iCpuUsage, 100, 40);
			//printf("cpu usage scaled: %i\n", iCpuUsageScaled);

			// create bitstream
			generateBitStream(iCpuUsageScaled, bit_stream_dec);

			// convert the values to hex
			bitStreamToHex(bit_stream_dec, bit_stream_hex);

			
			//fprintf(fhOutput, "%i\n", iCpuUsageScaled);

			// write to output file
			// we need a loop since c will print elements 0,1,...
			// but we need MSB first...
			int i;
			for(i = 9; i >= 0; i=i-2)
			{
				//printf("%c", bit_stream_hex[i-1]);
				fprintf(fhOutput, "%c", bit_stream_hex[i-1]);
				//printf("%c", bit_stream_hex[i]);
				fprintf(fhOutput, "%c", bit_stream_hex[i]);
			}
			//printf("\n");
			fprintf(fhOutput, "\n");

		} //endif
		

		// set previous values for the next iteration
		dOldTime       = dNewTime;
		iUserTimeOld   = iUserTimeNew;
		iNiceTimeOld   = iNiceTimeNew;	
		iSystemTimeOld = iSystemTimeNew;


		// wait for one second
		//sleep(1);
        // 10Hz
        usleep(500 * 1000);
		
	} //endwhile true
	

	// close output file
		fclose(fhOutput);

	// app has finished successfully
	return 0;

} //endmethod main


