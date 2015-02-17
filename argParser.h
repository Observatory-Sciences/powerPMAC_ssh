/**
 * @file argParser.h
 * @brief Header file for the argParser class which gets PowerPMAC connection parameters from the list of command line arguments
 * @author aaw@observatorysciences.co.uk
 */

#ifndef ARGPARSER_H_
#define ARGPARSER_H_

// Default connection parameters
#define DEFAULT_IPADDR 		"192.168.0.48" ///< Default Power PMAC IP address
#define DEFAULT_USER   		"root" ///< Default user name
#define DEFAULT_PASSW  		"deltatau" ///< Default password
#define DEFAULT_PORT   		"22" ///< Default SSH port on the Power PMAC

#include <iostream>
#include <string>

/// Get Power PMAC connection parameters from the list of command line arguments
class argParser{
public:
	argParser(const int argc, char * argv[]);
	~argParser();

	std::string getIp();
	std::string getUser();
	std::string getPassw();
	std::string getPort();
	bool getNominus2();

	int test();

private:
	int checkForArg(std::string argName, std::string & value, bool noValue = false);

	int myArgc;
	char ** myArgv;
};

#endif /* ARGPARSER_H_ */
