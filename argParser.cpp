/**
 * @file argParser.cpp
 * @brief Source for the argParser class which gets PowerPMAC connection parameters from the list of command line arguments
 *
 * @author Andrew Wilson <aaw@observatorysciences.co.uk>
 */
#include "argParser.h"

/** @brief Constructor
 * @param argc Argument count from main program
 * @param argv List of arguments from main program
 */
argParser::argParser(const int argc, char * argv[])
{
	// Make the argument count and argument list available to members
	myArgc = argc;
	myArgv = argv;
}

/// Destructor
argParser::~argParser()
{

}

/// Check for Power PMAC IP address
/// @return Power PMAC IP address
std::string argParser::getIp()
{
	std::string ipvalue;
	if (checkForArg("-ip", ipvalue) == 0)
	{
		//std::cout << "Found an IP address: " << ipvalue << std::endl;
		return ipvalue;
	}
	else
	{
		//std::cout << "-ip not found. \tUse default: " << u_ipaddr <<  std::endl;
		return DEFAULT_IPADDR;
	}
}

/// Check for user name
/// @return User name
std::string argParser::getUser()
{
	std::string uservalue;
	if (checkForArg("-user", uservalue) == 0)
	{
		//std::cout << "Found a user: " << uservalue << std::endl;
		return uservalue;
	}
	else
	{
		//std::cout << "-user not found. \tUse default: " << DEFAULT_USER <<  std::endl;
		return DEFAULT_USER;
	}
}
/// Check for password
/// @return Password
std::string argParser::getPassw()
{
	std::string passwvalue;
	if (checkForArg("-passw", passwvalue) == 0)
	{
		//std::cout << "Found a password: " << passwvalue << std::endl;
		return passwvalue;
	}
	else
	{
		//std::cout << "-passw not found. \tUse default: " << DEFAULT_PASSW <<  std::endl;
		return DEFAULT_PASSW;
	}
}
/// Check for SSH Port
/// @return Port number to use for SSH connection
std::string argParser::getPort()
{

	std::string portvalue;
	if (checkForArg("-port", portvalue) == 0)
	{
		//std::cout << "Found a port: " << portvalue << std::endl;
		return portvalue;
	}
	else
	{
		//std::cout << "-port not found. \tUse default: " << DEFAULT_PORT <<  std::endl;
		return DEFAULT_PORT;
	}
}

/// Check for 'No minus 2' option
/// @return True: use 'gpascii'; false: use 'gpascii -2'
bool argParser::getNominus2()
{

	std::string nominus2value;
	if (checkForArg("-nominus2", nominus2value, true) == 0)
	{
		//std::cout << "Found nominus2 option. " << std::endl;
		return true;
	}
	else
	{
		//std::cout << "-nominus2 not found. \tUse default: " << false <<  std::endl;
		return false;
	}
}

/// Scan command line arguments for a keyword.
/// If noValue == false, scan for a matching value and return it.
/// If noValue == true, don't scan for a value but return 0 to say that the keyword is present
int argParser::checkForArg(std::string argName, std::string & value, bool noValue)
{
	// Loop through the supplied command-line arguments.
	// Look for pairs '-argName value';
	// or, if noValue == true, look for '-argName' only.

	int argc = myArgc;
	char ** argv;
	argv = myArgv;
	if (argc > 1)
	{
		for (int i = 0; i < argc; i ++)
		{
			std::string str(argv[i]);
			if (str == argName)
			{
				// Found the argument keyword
				if (noValue == true)
				{
					// If noValue is set to true then we only check for the presence of the argname
					// and don't look for a value.
					return 0;
				}
				else
				{
					if (i >= (argc - 1))
					{
						// There are no more arguments so we can't find a value
						return -1;
					}
					else
					{
						// Look for a value in the next argument
						std::string nextstr(argv[i+1]);
						if (nextstr.find("-") == 0)
						{
							// The next argument is also a keyword (so no value)
							return -1;
						}
						else
						{
							// Return a value for this keyword
							value = argv[i+1];
							return 0;
						}
					}
				}
			}
		} // End for loop
	}

	// Gone through all the arguments without finding this one
	return -1;
}

/// A debugging function which prints the results of the all the members
int argParser::test()
{
	// Print args
	std::cout << "IP:\t\t" << this->getIp();
	std::cout << std::endl;
	std::cout << "User:\t\t" << this->getUser();
	std::cout << std::endl;
	std::cout << "Passw:\t\t" << this->getPassw();
	std::cout << std::endl;
	std::cout << "Port:\t\t" << this->getPort();
	std::cout << std::endl;
	std::cout << "Nominus2:\t" << this->getNominus2();
	std::cout << std::endl;

	return 0;
}
/*
int main(int argc, char *argv[])
{
	argParser * args = new argParser(argc, argv);

	args->test();

	delete args;

	return 0;
}
*/
