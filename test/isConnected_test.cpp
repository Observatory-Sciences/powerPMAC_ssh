/*
 * @file isConnected_test.cpp
 * @author Andrew Wilson <aaw@observatorysciences.co.uk>
 *
 * Connect to the Power PMAC and call PowerPMACcontrol_isConnected() at 1 Hz
 * so that the effect on this function of disrupting the connection can be observed.
 */

#include <iostream>
#include <string>
#include "PowerPMACcontrol.h"
#include "argParser.h"

using namespace PowerPMACcontrol_ns;

int main(int argc, char *argv[])
{
	// Get connection parameters from the command line arguments
	// Default values are defined in argParser.h
	argParser args(argc, argv);

	std::string u_ipaddr 	= args.getIp();
	std::string u_user 		= args.getUser();
	std::string u_passw		= args.getPassw();
	std::string u_port		= args.getPort();
	bool 		u_nominus2	= args.getNominus2();

	PowerPMACcontrol *ppmaccomm = new PowerPMACcontrol();
	int estatus = ppmaccomm->PowerPMACcontrol_connect( u_ipaddr.c_str(), u_user.c_str() , u_passw.c_str(), u_port.c_str(), u_nominus2);
	if (estatus != 0)
	{
	printf("Error connecting to power pmac. exit:\n");
	return 0;
	}
	else
	{
		printf("Connected OK.\n");
	}

	while (true)
	{
		bool status = ppmaccomm->PowerPMACcontrol_isConnected(5000);
		std::cout << "PowerPMACcontrol_isConnected() = " << status << std::endl;
		sleep(1);
	}
}
