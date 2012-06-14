#include "wlan_info.h"		// our local header file


#include <signal.h>


static bool keepRunning = true;

void intHandler(int notUsed) {
	printf("\n\n\"CTRL + C caught, exiting gracefully... ");
	keepRunning = false;
}




void error(const char *msg)
{
    perror(msg);
    exit(0);
} //endmethod error



/** 
  * source: http://code.google.com/p/wfbil-blueprint/source/browse/trunk/scanner.c?spec=svn3&r=3
  *
  * scanner_wifi_free - destroy the wireless scan linked list
  * @wsh - a pointer to the wireless_scan_head
  * returns: 0 on success; -1 on failure
  */
static int scanner_wifi_free(struct wireless_scan_head *wsh)
{
        struct wireless_scan *next;
        struct wireless_scan *current;
        
        current = wsh->result;
        while (current){
                next = current->next;
                free(current);
                current = next; 
        }
        wsh->result = NULL;
        return 0;
} //endmethod scanner_wifi_free



int main ( int argc, char *argv[] )
{
	

	//
	if ( argc != 2 )
	{
		/* We print argv[0] assuming it is the program name */
		printf( "usage: %s /some/file \n", argv[0] );
		return 0;
	}
	
	
	
	signal(SIGINT, intHandler);
	signal(SIGKILL, intHandler);
	
	
	FILE *output_file;
	output_file = fopen(argv[1], "w");
	

	
	
	int iSockFd; 					// file descriptor of the socket
	int iScanSuccess;				// used to store success of failure of  the scan (-1 for error and 0 for success)
	int iSignalStrengthInDb;

	char * cpInterfaceName = "wlan0";	// name of the wireless interface
	struct wireless_scan_head wsh; 	// result of wireless scan will be stored inside struct
	struct wireless_scan *ws;			// needed to itarate through the scan result's entries
	
	int const iWirelessExtensionVersion = iw_get_kernel_we_version();

	printf("wireless extension version: %i \n", iWirelessExtensionVersion);

	// open the network socket
	printf("trying to open network socket. \n");
	iSockFd = socket(AF_INET, SOCK_DGRAM, 0);
	
	// check if opening the sucket went successful
	if (iSockFd < 0)
		error("ERROR opening socket");
	
	
	
	while(keepRunning)
	{
		// do the wlan scan
		printf("starting scan... \n");
		iScanSuccess = iw_scan(iSockFd, cpInterfaceName, iWirelessExtensionVersion, &wsh);
		printf("success: %i \n", iScanSuccess);

		
		// inspired by http://code.google.com/p/wfbil-blueprint/source/browse/trunk/mapcache.c
		/* parse wifi scan results */
		ws = wsh.result;
		while (ws != NULL)
		{
			
			// get signal strength in db
			iSignalStrengthInDb = ws->stats.qual.level;
			
			// simon says: don't know what this is doing! taken from mapcache.c
			if (iSignalStrengthInDb >= 64) {
				iSignalStrengthInDb -= 0x100;
			}
			/* convert to positive */
			iSignalStrengthInDb = 0 - iSignalStrengthInDb;
			
			printf("address: %s; ", ws->ap_addr.sa_data);
			printf("signal strength: %i \n\n", iSignalStrengthInDb);
			
			fprintf(output_file, "%i\n", iSignalStrengthInDb);

			
			// proceed to the next scan entry
			ws = ws->next;

		} //endwhile ws != NULL
		
		
		// wait for one second
		sleep(1);
		
	} //endwhile true
	
	
	
	
	
	
	

	// closing output file
	fclose(output_file);
	
	// free scan result memory
	printf("freeing scan result memory\n");
	scanner_wifi_free(&wsh);
	
	// close socket
	printf("closing socket \n");
	close(iSockFd);

	
	
	return 0;
	
} //endmethod main
