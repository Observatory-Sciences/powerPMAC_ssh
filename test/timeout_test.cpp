/*
 * @file timeout_test.cpp
 * @author Andrew Wilson <aaw@observatorysciences.co.uk>
 *
 * Test getting and setting the length of the common communications timeout for the Power PMAC SSH communications library.
 */

#include <iostream>
#include <string>
#include "../PowerPMACcontrol.h"
#include "../argParser.h"
#include <ctime>
#include <cmath>

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

	// Read default timeout
	printf("\nRead default timeout.\n");
	{
		int default_timeout;
		int s1 = ppmaccomm->PowerPMACcontrol_getTimeout(default_timeout);
		printf("PowerPMACcontrol_getTimeout returned %i.\n", s1);
		std::cout << "Default timeout = " << default_timeout << std::endl;
		bool status = ppmaccomm->PowerPMACcontrol_isConnected();
		printf("With default timeout, PowerPMACcontrol_isConnected() = %i.\n", status);

		if (s1 == ppmaccomm->PPMACcontrolNoError)
			printf("--- PASS\n");
		else
			printf("--- FAIL\n");
	}

	// Check that negative values are not allowed
	printf("\nTry to set negative value for timeout.\n");
	{
		int timeout_before = 0;
		int s1 = ppmaccomm->PowerPMACcontrol_getTimeout(timeout_before);
		printf("Timeout before = %i (Return %i).\n", timeout_before, s1);


		int negative_timeout = -123;
		int status = ppmaccomm->PowerPMACcontrol_setTimeout(negative_timeout);
		printf("PowerPMACcontrol_setTimeout( %i ) returned %i.\n", negative_timeout, status);

		int timeout_after = 0;
		int s2 = ppmaccomm->PowerPMACcontrol_getTimeout(timeout_after);
		printf("Tiemout after = %i (Return %i).\n", timeout_after, s2);

		if (status != ppmaccomm->PPMACcontrolInvalidParamError)
			printf("--- FAIL : Did not receive correct error code\n");
		else if (timeout_after != timeout_before)
			printf("--- FAIL : Function should not have changed timeout\n");
		else
			printf("--- PASS \n");
	}

	// Try out changing the timeout and calling a function
	printf ("\nSet some different values for timeout\n");
	{
		for (int timeout = 2000; timeout <= 5000; timeout += 1000)
		{

			int rc1 = ppmaccomm->PowerPMACcontrol_setTimeout(timeout);
			printf("Set common timeout to %i; return %i.\n", timeout, rc1);
			int new_timeout;
			int rc2 = ppmaccomm->PowerPMACcontrol_getTimeout(new_timeout);
			printf("Common timeout now %i; return %i.\n", new_timeout, rc2);
			bool status = ppmaccomm->PowerPMACcontrol_isConnected();
			printf("PowerPMACcontrol_isConnected() = %i.\n", status);

			if ((rc1== ppmaccomm->PPMACcontrolNoError)
					&& (rc2 == ppmaccomm->PPMACcontrolNoError)
					&& (new_timeout == timeout))
				printf("--- PASS\n");
			else
				printf("--- FAIL\n");
		}
	}

	// Test that timeout is correct length after we change it
	// Ask the user to break the connection so commands will time out.
	printf("\nNow we will test that the timeout changes are taking effect.\nPlease break the network connection to the PPMAC then type anything + RETURN to continue.\n");
	{
		std::string goAhead;
		std::cin >> goAhead;
		for (int timeout = 1000; timeout <= 5000; timeout += 1000)
		{

			int rc1 = ppmaccomm->PowerPMACcontrol_setTimeout(timeout);
			printf("Set common timeout to %i; return %i.\n", timeout, rc1);
			int new_timeout;
			int rc2 = ppmaccomm->PowerPMACcontrol_getTimeout(new_timeout);
			printf("Common timeout now %i; return %i.\n", new_timeout, rc2);
			clock_t begin1 = clock();
			bool status = ppmaccomm->PowerPMACcontrol_isConnected();
			clock_t end1 = clock();
			printf("PowerPMACcontrol_isConnected() = %i.\n", status);

			clock_t duration = end1 - begin1;
			double secs = (double)duration / CLOCKS_PER_SEC;
			printf("Took %f s.\n", secs );

			if ((rc1 != ppmaccomm->PPMACcontrolNoError))
				printf("--- FAIL : PowerPMACcontrol_setTimeout returned error.\n");
			else if (rc2 != ppmaccomm->PPMACcontrolNoError)
				printf("--- FAIL : PowerPMACcontrol_getTimeout returned error.\n");
			else if (new_timeout != timeout)
				printf("--- FAIL : Reported timemout doesn't match set value\n");
			else if (secs <= 0.5)
				printf("--- FAIL : Doesn't look like the connection is broken; PowerPMACcontrol_isConnected() returned after < 0.5 s.\n");
			else if ((int)secs > 0 && fabs(secs * 1000 - timeout) > 500)
				printf("--- FAIL : Timeout set to %i ms but waited for %i ms.\n", timeout, abs(secs * 1000));
			else
				printf("--- PASS\n");
		}
	}
	// Do the same thing with axisGetCurrentPosition
	printf("\nNow do the same with axisGetCurrentPositioin.\n");
	{
		for (int timeout = 1000; timeout <= 5000; timeout += 1000)
		{

			int rc1 = ppmaccomm->PowerPMACcontrol_setTimeout(timeout);
			printf("Set common timeout to %i; return %i.\n", timeout, rc1);
			int new_timeout;
			int rc2 = ppmaccomm->PowerPMACcontrol_getTimeout(new_timeout);
			printf("Common timeout now %i; return %i.\n", new_timeout, rc2);
			clock_t begin1 = clock();
			double d;
			bool status = ppmaccomm->PowerPMACcontrol_axisGetCurrentPosition(1, d);
			clock_t end1 = clock();
			printf("PowerPMACcontrol_axisGetCurrentPosition(1) = %f.\n", d);

			clock_t duration = end1 - begin1;
			double secs = (double)duration / CLOCKS_PER_SEC;
			printf("Took %f s.\n", secs );

			if ((rc1 != ppmaccomm->PPMACcontrolNoError))
				printf("--- FAIL : PowerPMACcontrol_setTimeout returned error.\n");
			else if (rc2 != ppmaccomm->PPMACcontrolNoError)
				printf("--- FAIL : PowerPMACcontrol_getTimeout returned error.\n");
			else if (new_timeout != timeout)
				printf("--- FAIL : Reported timemout doesn't match set value\n");
			else if (secs <= 0.5)
				printf("--- FAIL : Doesn't look like the connection is broken; PowerPMACcontrol_axisGetCurrentPosition(1) returned after < 0.5 s.\n");
			else if ((int)secs > 0 && fabs(secs * 1000 - timeout) > 500)
				printf("--- FAIL : Timeout set to %i ms but waited for %i ms.\n", timeout, abs(secs * 1000));
			else
				printf("--- PASS\n");
		}
	}
}
