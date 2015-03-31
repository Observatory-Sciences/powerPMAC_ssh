/********************************************
 *  PowrePMACcontrol.cpp
 * 
 *  A class which manages the communication with Power PMAC 
 *  over a SSH connection.
 * 
 *  Aya Yoshimura <aya@observatorysciences.co.uk>
 * 
 ********************************************/

/**
* @mainpage
*  PowerPMACcontrol is a C++ communications library which provides a set of functions
*  to send commands to, and get status from a Delta Tau PowerPMAC motion controller.\n
*  Communication is performed using the SSH protocol over Ethernet.\n
*  Supported platforms are Linux and Windows (Microsoft Visual Studio C++).
*/

/** 
 * @file PowerPMACcontrol.cpp
 * @brief C++ source file for the PowerPMACcontrol_ns::PowerPMACcontrol class. This file implements the class and its functions.
 *  
 * The PowerPMACcontrol class manages the communication with Power PMAC. 
 * To connect to the Power PMAC, call PowerPMACcontrol_connect function with
 * IP Address, user name and password parameters. 
 */



//#include <sstream>
#include <fstream>
#include "PowerPMACcontrol.h"
#ifndef WIN32
#include <errno.h>
#endif

namespace PowerPMACcontrol_ns
{


    /**
 * @brief Destructor for the PowerPMACcontrol.
 * 
 * If it is still connected to a Power PMAC, it will disconnect it.
 */
PowerPMACcontrol::~PowerPMACcontrol() {
    debugPrint_ppmaccomm("~PowerPMACcontrol() called\n");
    if (connected!=0)
    {
        this->PowerPMACcontrol_disconnect();
    }
    delete sshdriver;
    
#ifndef WIN32
    sem_destroy(&sem_writeRead);
#endif
    
}

/**
 * Constructor for the PowerPMACcontrol.
 */
PowerPMACcontrol::PowerPMACcontrol(){
    
    sshdriver = NULL;
    connected = 0;

    // Initialise the timeout
    common_timeout_ms = DEFAULT_TIMEOUT_MS;

#ifdef WIN32    
	ghSemaphore = CreateSemaphore(
		NULL,
		1,
		1,
		NULL);
	if (this->ghSemaphore == NULL)
#else
    int semaphore_ret = sem_init(&sem_writeRead, 0,1);
    if (semaphore_ret != 0)
#endif
	{
		debugPrint_ppmaccomm("PowerPMACcontrol() : Error creating a semaphore\n");
	}
	else
		debugPrint_ppmaccomm("PowerPMACcontrol() : a semaphore created\n");

}

/**
 * @brief Attempt to create an SSH connection to Power PMAC using the username
 * and the password.
 * 
 * Once the connection has
 * been established, it will start gpascii program on the Power PMAC.
 * 
 * @param host - Host name/IP address of the Power PMAC
 * @param user - User name for the SSH connection 
 * @param pwd - Password for the SSH connection 
 * @param port - Port number for the SSH connection (default 22)
 * @param nominus2 - If true, remove the '-2' option on the command sent to start gpascii:
 * 					'gpascii -2' (nominus2 = false, default),
 * 					'gpascii' (nominus2 = true).
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorUnknownHost (-114)
 *      - PPMACcontrolSSHDriverErrorSshInit (-110)
 *      - PPMACcontrolSSHDriverErrorSockfail (-109)
 *      - PPMACcontrolSSHDriverErrorSshSession (-111)
 *      - PPMACcontrolSSHDriverErrorPassword (-105)
 *      - PPMACcontrolSSHDriverErrorPublicKey (-107)
 *      - PPMACcontrolSSHDriverErrorPty (-106)
 *      - PPMACcontrolSSHDriverErrorShell (-108)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSemaphoreTimeoutError (-239)
 *      - PPMACcontrolSemaphoreError (-240)
 *      - PPMACcontrolSemaphoreReleaseError (-241)
 *      .
 */
int PowerPMACcontrol::PowerPMACcontrol_connect(const char *host, const char *user, 
                                                        const char *pwd, const char *port, const bool nominus2){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_connect";
    
    if (strlen(host) > 255)
    {
        debugPrint_ppmaccomm("%s : Error - Host name too long\n", functionName);
        //libssh2driver can't take host name longer than 255.
        return PPMACcontrolInvalidHostNameError;
        
    }
    
    
    int return_val = PPMACcontrolNoError;
    //get semaphore
#ifdef WIN32    
	DWORD dwWaitResult = WaitForSingleObject(this->ghSemaphore, INFINITE);
    if (dwWaitResult == WAIT_OBJECT_0)
#else
    int sem_return = sem_wait(&sem_writeRead);
    if (sem_return == 0)
#endif
    {
    
    
        // In case it's already connected
        if (sshdriver!=NULL)   //If sshdriver has been created
        {
            printf("this connected is %d\n",this->connected );
            this->PowerPMACcontrol_disconnect();
            printf("this connected is %d\n",this->connected );
            delete sshdriver;
        }


        sshdriver = new SSHDriver( host );
        SSHDriverStatus ret = sshdriver->setUsername(user);
        if (ret != SSHDriverSuccess)
        {
            if (ret == SSHDriverErrorInvalidParameter)
                return_val = PPMACcontrolInvalidUserNameError;
            else
                return_val = PPMACcontrolSSHDriverError; //This can't happen
        }
        else
        {
            ret = sshdriver->setPassword(pwd);
            if (ret != SSHDriverSuccess)
            {
                if (ret == SSHDriverErrorInvalidParameter)
                    return_val = PPMACcontrolInvalidPasswordError;
                else
                    return_val = PPMACcontrolSSHDriverError; //This can't happen
            }
            else
            {
                debugPrint_ppmaccomm("%s : Setting port to %s\n", functionName, port);
                ret = sshdriver->setPort(port);
                if (ret != SSHDriverSuccess)
                {
                    if (ret == SSHDriverErrorInvalidParameter)
                        return_val = PPMACcontrolInvalidPortError;
                    else
                        return_val = PPMACcontrolSSHDriverError; //This can't happen
                }
                else
                {
                

                    ret = sshdriver->connectSSH();
                    if (ret != SSHDriverSuccess)
                    {
                        int estatus;
                        switch (ret){
                            case SSHDriverErrorUnknownHost:
                                estatus = PPMACcontrolSSHDriverErrorUnknownHost;
                                break;
                            case SSHDriverErrorSshInit:
                                estatus = PPMACcontrolSSHDriverErrorSshInit;
                                break;
                            case SSHDriverErrorSockfail:
                                estatus = PPMACcontrolSSHDriverErrorSockfail;
                                break;
                            case SSHDriverErrorSshSession:
                                estatus = PPMACcontrolSSHDriverErrorSshSession;
                                break;
                            case SSHDriverErrorPassword:
                                estatus = PPMACcontrolSSHDriverErrorPassword;
                                break;                
                            case SSHDriverErrorPublicKey:
                                estatus = PPMACcontrolSSHDriverErrorPublicKey;
                                break;      
                            case SSHDriverErrorPty:
                                estatus = PPMACcontrolSSHDriverErrorPty;
                                break;                 
                            case SSHDriverErrorShell:
                                estatus = PPMACcontrolSSHDriverErrorShell;
                                break;
                            case SSHDriverErrorInvalidParameter:
                                estatus = PPMACcontrolSSHDriverErrorInvalidParameter;
                                break;
                            default:
                                estatus = PPMACcontrolSSHDriverError;
                        }
                        return_val = estatus;
                    }
                    else
                    {
                        //Start the gpascii program on the powerPmac
                        char buff[512] = "";
                        size_t bytes = 0;
                        if (nominus2)
                        {
                        	// don't use the -2 option
                        	strcpy(buff, "gpascii\n");
                        }
                        else
                        {
                        	// use the -2 option
                        	strcpy(buff, "gpascii -2\n");
                        }

                        debugPrint_ppmaccomm("%s : Writing '%s' to the powerpmac\n", functionName, buff);
                        int ret2 = this->PowerPMACcontrol_write(buff, strlen(buff), &bytes, 1000);

                        if (ret2 != PPMACcontrolNoError)
                        {
                          debugPrint_ppmaccomm("%s : Error while writing 'gpascii -2' to the powerpmac\n", functionName);
                          return_val = ret2;
                        }
                        else
                        {
                            debugPrint_ppmaccomm("%s : Reading reply to 'gpascii -2' from the powerpmac\n", functionName);
                            ret2 = this->PowerPMACcontrol_read(buff, 512, &bytes, '\n', 2000);
                            if (ret2 != PPMACcontrolNoError)
                            {
                              debugPrint_ppmaccomm("%s : Error while reading reply to 'gpascii -2' command from the powerpmac\n", functionName);
                              return_val = ret2;
                            }
                            else
                            {
                                debugPrint_ppmaccomm("%s : Setting 'echo7' to the powerpmac\n", functionName);
                                strcpy(buff, "echo7\n");
                                debugPrint_ppmaccomm("%s : Writing 'echo7' to the powerpmac\n", functionName);
                                ret2 = this->PowerPMACcontrol_write(buff, strlen(buff), &bytes, 1000);

                                if (ret2 != PPMACcontrolNoError)
                                {
                                  debugPrint_ppmaccomm("%s : Error while writing 'echo7' to the powerpmac\n", functionName);
                                  return_val = ret2;
                                }
                                else
                                {
                                    ret2 = this->PowerPMACcontrol_read(buff, 512, &bytes, '\n', 2000);
                                    if (ret2 != PPMACcontrolNoError)
                                    {
                                      debugPrint_ppmaccomm("%s : Error while reading reply to 'gpascii' command from the powerpmac\n", functionName);
                                      return_val = ret2;
                                    }
                                    else
                                        this->connected = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        //Release Semaphore
#ifdef WIN32 
		if (!ReleaseSemaphore(ghSemaphore, 1, NULL))
		{
            return_val = PPMACcontrolSemaphoreReleaseError;
			debugPrint_ppmaccomm("%s : Error releasing Semaphore\n",functionName);
		}
        else
        {
            debugPrint_ppmaccomm("%s : Released Semaphore\n",functionName);
        }
#else
        if (sem_post(&sem_writeRead)!=0)
        {
            return_val = PPMACcontrolSemaphoreReleaseError;
            debugPrint_ppmaccomm("%s : Error releasing Semaphore\n",functionName);
        }
        else
        {
            debugPrint_ppmaccomm("%s : Released Semaphore\n",functionName);
        }
#endif
        return return_val;
    }
#ifdef WIN32 
	else if (dwWaitResult == WAIT_TIMEOUT)
	{
        debugPrint_ppmaccomm("%s : Semaphore timed out\n",functionName);
        return PPMACcontrolSemaphoreTimeoutError;
	}	
    else
	{
        debugPrint_ppmaccomm("%s : Error obtaining a Semaphore (%d)\n",functionName, dwWaitResult);
        return PPMACcontrolSemaphoreError;
	}
#else
    else
	{
        if (errno == ETIMEDOUT)
        {
			debugPrint_ppmaccomm("%s : Semaphore timed out\n",functionName);
			return PPMACcontrolSemaphoreTimeoutError;
        }
        else
        {	
            debugPrint_ppmaccomm("%s : Semaphore error %d\n",functionName, errno);
			return PPMACcontrolSemaphoreError;
        }    
    }
#endif
}

/**
 * @brief Close the SSH connection.
 * 
 * @return If successful, PPMACcontrolNoError(0) is returned. 
 * If not PPMACcontrolSSHDriverError (-102).
 */
int PowerPMACcontrol::PowerPMACcontrol_disconnect(){
//    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_disconnect";
    if (this->sshdriver != NULL)
    {
        int ret = sshdriver->disconnectSSH();
        if (ret == SSHDriverSuccess)
        {
            this->connected = 0;
            return PPMACcontrolNoError;
        }
        return PPMACcontrolSSHDriverError; //sshdriver->disconnectSSH() always return No error.
    }
    return PPMACcontrolNoError; //Not connected. No error.
}

/**
 * @brief Checks the state of the SSH connection to the Power PMAC.
 *
 * If a connection is open, a global status command is sent; the function will check that the command is sent and the reply received successfully.
 * 
 * @param timeout Connection timeout in milliseconds. If not specified, the common timeout is used: default 1000 ms; this can be updated using PowerPMACcontrol_setTimeout.
 * @return Return true if a connection is open and operating normally. Return false if communication fails or times out, or if no connection is open.
 */
bool PowerPMACcontrol::PowerPMACcontrol_isConnected(int timeout){
    // If no timeout specified, use the common value
    if (timeout == TIMEOUT_NOT_SPECIFIED)
    {
    	timeout = common_timeout_ms;
    }

	if (this->connected > 0)
    {
    	// Connection has been opened
    	// Send a global status command; check for successful send and receive
    	char cmd[128] = {0};
		sprintf( cmd, "?\n");
		std::string reply;
		int ret = writeRead(cmd, reply, timeout);
		if (ret != PPMACcontrolNoError)
		{
			// Communication error
			return false;
		}
		if (reply.length() != 9)
		{
			// Expected reply not received
			return false;
		}
		else
    	{
    		// Command send/receive OK
    		return true;
    	}
    }
    else
    {
    	// Connection has not been opened
        return false;
    }
}
/**
 * @brief Return the length of the communications timeout used for all functions
 * @param timeout_ms Variable to receive the timeout in milliseconds
 * @return Always returns PPMACcontrolNoError(0) because no communication occurs in this function
 */
int PowerPMACcontrol::PowerPMACcontrol_getTimeout(int & timeout_ms){
	timeout_ms = common_timeout_ms;
	return PPMACcontrolNoError;
}
/**
 * @brief Set the length of the communications timeout used for all functions
 * @param timeout_ms Timeout in milliseconds
 * @return If successful, PPMACcontrolNoError (0) is returned.
 * If an invalid timeout value is entered, PPMACcontrolInvalidParamError (-242) is returned.
 */
int PowerPMACcontrol::PowerPMACcontrol_setTimeout(int timeout_ms){
	// Check validity of supplied value
	if (timeout_ms > 0){
		common_timeout_ms = timeout_ms;
		return PPMACcontrolNoError;
	}
	else
	{
		// Return error code to indicate invalid timeout parameter supplied
		return PPMACcontrolInvalidParamError;
	}

}
/**
 * @brief Write data to the connected SSH channel.
 * 
 * A timeout should be specified in milliseconds.
 *
 * @param buffer - The string buffer to be written.
 * @param bufferSize - The number of bytes to write.
 * @param bytesWritten - The number of bytes that were written.
 * @param timeout - A timeout in ms for the write.
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 */
int PowerPMACcontrol::PowerPMACcontrol_write(const char *buffer, size_t bufferSize, size_t *bytesWritten, int timeout)
{
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_write";
    //if (this->connected == 0)
    //{
    //    debugPrint_ppmaccomm("%s : PMAC is not connected\n", functionName);
    //    return PPMACcontrolNoSSHDriverSet;
    //}
    if (sshdriver == NULL)
    {
        debugPrint_ppmaccomm("%s : SSH driver not set\n", functionName);
        return PPMACcontrolNoSSHDriverSet;        
    }
    int ret = sshdriver->write(buffer, bufferSize, bytesWritten, timeout);
    if (ret == SSHDriverSuccess)
    {
        return PPMACcontrolNoError;
    }
    else
    {
        int estatus;
        switch (ret){
            case SSHDriverErrorNoconn:
                estatus = PPMACcontrolSSHDriverErrorNoconn;
                break;
            case SSHDriverErrorNobytes:
                estatus = PPMACcontrolSSHDriverErrorNobytes;
                break;
            case SSHDriverErrorWriteTimeout:
                estatus = PPMACcontrolSSHDriverErrorWriteTimeout;
                break;
            default:
                estatus = PPMACcontrolSSHDriverError;
        }
        return estatus;
    }
}


/**
 * @brief Read data from the connected SSH channel.
 * 
 * A timeout should be
 * specified in milliseconds.  The read method will continue to
 * read data from the channel until either the specified 
 * terminator is read or the timeout is reached.
 *
 * @param buffer - A string buffer to hold the read data.
 * @param bufferSize - The maximum number of bytes to read.
 * @param bytesWritten - The number of bytes that have been read.
 * @param readTerm - A terminator to use as a check for EOM (End Of Message).
 * @param timeout - A timeout in ms for the read.
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 */
int PowerPMACcontrol::PowerPMACcontrol_read(char *buffer, size_t bufferSize, size_t *bytesRead, int readTerm, int timeout)
{
    static const char *functionName = "PowerPMACcontrol::powerpmac_connect";
    //if (this->connected == 0)
    if (this->sshdriver == NULL)
    {
        debugPrint_ppmaccomm("%s : SSH driver is not set\n", functionName);
        return PPMACcontrolNoSSHDriverSet;
    }
    debugPrint_ppmaccomm("%s : buffer size is %d\n", functionName, bufferSize);
    SSHDriverStatus ret = sshdriver->read(buffer, bufferSize, bytesRead, readTerm, timeout);
    if (ret == SSHDriverSuccess)
    {
        return PPMACcontrolNoError;
    }
    else
    {
        int estatus;
        switch (ret){
            case SSHDriverErrorNoconn:
                estatus = PPMACcontrolSSHDriverErrorNoconn;
                break;
            case SSHDriverErrorReadTimeout:
                estatus = PPMACcontrolSSHDriverErrorReadTimeout;
                break;
            default:
                estatus = PPMACcontrolSSHDriverError;
        }
        return estatus;
    }
}

/**
 * @brief Get firmware version.
 * 
 * Power PMAC command string sent = "vers"
 * @param vers - Firmware version will be written to this parameter.
 * @return If version is successfully received, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getVers(std::string& vers){
//    static const char *functionName = "PowerPMACcontrol::powerpmac_getVers";
    
    char cmd[128] = "";
    sprintf( cmd, "vers\n");
    
    return this->writeRead(cmd,vers);

} 


/**
 * @brief Get IDs of embedded software
 * 
 * Power PMAC command string sent = "buffer"
 * 
 * @param num - Number of motion and program names.
 * @param names - Names of the programs. 
 * @return If names of the programs are successfully received, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getProgNames(int& num, std::vector<std::string>& names){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_getProgNames";
    debugPrint_ppmaccomm("%s called", functionName);
    names.clear();
    num=0;
    char cmd[128] = {0};
    sprintf( cmd, "buffer\n");
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;

    debugPrint_ppmaccomm("%s : reply length is %d", functionName, reply.length());
    if (reply.length() < 1)
    {
        return PPMACcontrolNoError; //Nothing returned.
    }
    
    debugPrint_ppmaccomm("%s : reply compare return is  is %d", functionName, reply.compare("Buffer is empty"));
    //Check if the reply is 'Buffer is empty'
    if ((reply.compare("Buffer is empty")) == 0)
    {
        debugPrint_ppmaccomm("%s : Buffer is empty", functionName);
        return PPMACcontrolNoError; //It is empty
    }
    
    std::vector<std::string> progstatus;
    
    ret = PowerPMACcontrol::splitit(reply, "\r\n", progstatus);
    if (ret != PPMACcontrolNoError)
        return PPMACcontrolPMACUnexpectedReplyError;
    try{
        for (size_t i=0; i<progstatus.size(); i++)
        {
            std::vector<std::string> tmp;
            ret = PowerPMACcontrol::splitit(progstatus.at(i), " ", tmp);
            if (ret != PPMACcontrolNoError)
            {
                num=0;
                names.clear();
                return PPMACcontrolPMACUnexpectedReplyError;
            }
            if (tmp.size() == 0)
            {
                num=0;
                names.clear();
                return PPMACcontrolPMACUnexpectedReplyError;
            }
            
            names.push_back(tmp.at(0));
            
        }
    }
    catch(...)
    {
        debugPrint_ppmaccomm("%s : Error trying to get Prog Names", functionName);
        num=0;
        names.clear();
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    num = names.size();
    return PPMACcontrolNoError;
    
}

/**
 * @brief Get powered state of a motor
 * 
 * Power PMAC command string sent = "Motor[<motor index>].ServoCtrl"
 * 
 * @param mnum - Index of a motor.
 * @param powered - Powered state of the motor. 
 * @return If powered state of the motor is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241) 
 */
int PowerPMACcontrol::PowerPMACcontrol_motorPowered(int mnum, bool& powered){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_MotorPowered";
    if (this->connected == 0)
    {
        debugPrint_ppmaccomm("%s : PowerPMACcontrol::write: PMAC is not connected", functionName);
        return PPMACcontrolNoSSHDriverSet;
    }
    char cmd[512] = "";
    sprintf( cmd, "Motor[%d].ServoCtrl\n", mnum);
    
    std::string reply="";
    int ret = this->writeRead(cmd,reply);
    if (ret != PPMACcontrolNoError)
        return ret;
   
    if (reply.compare("1") == 0)
    {
        powered = true;
        return PPMACcontrolNoError;
    }
    if (reply.compare("0") == 0)
    {
        powered = false;
        return PPMACcontrolNoError;
    }
    
    // The reply from Power PMAC was not as expected. 
    // Failed to get powered state.
    return PPMACcontrolPMACUnexpectedReplyError;
}

/**
 * @brief Get velocity of an axis
 * 
 * Power PMAC command string sent = "Motor[<axis index>].JogSpeed"
 * 
 * @param axis - Index of axis.
 * @param velocity - Velocity of the axis
 * @return If velocity of the axis is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisGetVelocity(int axis, double& velocity){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisGetVelocity";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = "";
    sprintf( cmd, "Motor[%d].JogSpeed\n", axis);
    
    std::string reply;
    int ret = this->writeRead(cmd,reply);
    if (ret != PPMACcontrolNoError)
        return ret;

    double dval = 0.0;
    std::istringstream stream(reply);
    stream >> dval;
    if (stream.fail())
    {
        //failed to convert to double value
        return PPMACcontrolPMACUnexpectedReplyError;
    }

    velocity = dval;
    return PPMACcontrolNoError;
    
}

/**
 * @brief Set velocity of an axis
 * 
 * Power PMAC command string sent = "Motor[<axis index>].JogSpeed=<velocity>"
 *
 * @param axis - Index of axis.
 * @param velocity - New velocity of the axis to be set
 * @return If the velocity of the axis is set successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisSetVelocity(int axis, double velocity){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisSetVelocity";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "Motor[%d].JogSpeed=%f\n", axis, velocity);
  
    return writeRead(cmd);
}

/**
 * @brief Define position of an axis.
 * 
 * This function sends a command to kill the axis movement and set position.\n
 * Power PMAC command string sent = "#<axis index>k Motor[<axis index>].Pos=<new position>"
 * 
 * @param axis - Index of axis.
 * @param newpos - New position of the axis to be defined
 * @return If the position of the axis is defined successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisDefCurrentPos(int axis, double newpos){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisDefCurrentPos";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dk Motor[%d].Pos=%f\n", axis, axis, newpos);
  
    return writeRead(cmd);
}

/**
 * @brief Get acceleration of an axis.
 * 
 * If 'inf' or 'nan' is received from the Power PMAC, 
 * this function will return PPMACcontrolPMACUnexpectedReplyError (-231) and 
 * acceleration parameter will not be set.\n
 * Power PMAC command string sent = "Motor[<axis index>].JogTa"
 *
 * @param axis - Index of axis.
 * @param acceleration - Acceleration of the axis
 * @return If acceleration of the axis is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisGetAcceleration(int axis, double& acceleration){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisGetAcceleration";
    debugPrint_ppmaccomm("%s called", functionName);

    char cmd[128] = "";
    sprintf( cmd, "Motor[%d].JogTa\n", axis);
    
    std::string reply;
    int ret = this->writeRead(cmd,reply);
    if (ret != PPMACcontrolNoError)
        return ret;

    std::istringstream stream(reply);
    double dval;
    stream >> dval;
    if (stream.fail())
    {
        //failed to convert to double value
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    acceleration = dval;
    return PPMACcontrolNoError;
}

/**
 * @brief Set acceleration of an axis.
 * 
 * Power PMAC command string sent = "Motor[<axis index>].JogTa=<new acceleration>"
 *
 * @param axis - Index of axis.
 * @param acceleration - New acceleration of the axis to be set
 * @return If the new acceleration of the axis is set successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisSetAcceleration(int axis, double acceleration){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisSetAcceleration";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "Motor[%d].JogTa=%f\n", axis, acceleration);
  
    return writeRead(cmd);
}

/**
 * @brief Get deadband of an axis.
 * 
 * If 'inf' or 'nan' is received from the Power PMAC, 
 * this function will return PPMACcontrolPMACUnexpectedReplyError (-231) and 
 * the deadband parameter will not be set.\n
 * Power PMAC command string sent = "Motor[<axis index>].Servo.OutDbOn"
 *
 * @param axis - Index of axis.
 * @param deadband - Deadband of the axis
 * @return If deadband of the axis is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisGetDeadband(int axis, double& deadband){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisGetDeadband";
    debugPrint_ppmaccomm("%s called", functionName);
  
    char cmd[128] = "";
    sprintf( cmd, "Motor[%d].Servo.OutDbOn\n", axis);
    
    std::string reply;
    int ret = this->writeRead(cmd,reply);
    if (ret != PPMACcontrolNoError)
        return ret;

    std::istringstream stream(reply);
    double dval;
    stream >> dval;
    if (stream.fail())
    {
        //failed to convert to double value
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    deadband = dval;
    return PPMACcontrolNoError;
}

/**
 * @brief Set deadband of an axis.
 * 
 * Power PMAC command string sent = "Motor[<axis index>].Servo.OutDbOn=<new deadband>"
 *
 * @param axis - Index of axis.
 * @param deadband - New deadband of the axis to be set
 * @return If the new deadband of the axis is set successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisSetDeadband(int axis, double deadband){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisSetDeadband";
    debugPrint_ppmaccomm("%s called", functionName);
    char cmd[128] = {0};
    sprintf( cmd, "Motor[%d].Servo.OutDbOn=%f\n", axis, deadband);
  
    return writeRead(cmd);
}

/**
 * @brief Get software limits of an axis.
 * 
 * If 'inf' or 'nan' is received from the Power PMAC, 
 * this function will return PPMACcontrolPMACUnexpectedReplyError (-231) and 
 * the maxpos and minpos parameters will not be set. The maxpos and the minpos 
 * parameters are set only if both values are received successfully from Power PMAC.\n
 * Power PMAC command string sent = "Motor[<axis index>].MaxPos Motor[<axis index>].MinPos"
 *
 * @param axis - Index of axis.
 * @param maxpos - Maximum software limit
 * @param minpos - Minimum software limit
 * @return If both limit positions are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisGetSoftwareLimits(int axis, double& maxpos, double& minpos){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisGetSoftwareLimits";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = "";
    sprintf( cmd, "Motor[%d].MaxPos Motor[%d].MinPos\n", axis, axis);
    
    std::string reply;
    int ret = this->writeRead(cmd,reply);
    if (ret != PPMACcontrolNoError)
        return ret;

    std::vector<std::string> ss;
    ret = PowerPMACcontrol::splitit(reply, "\n", ss);
    if (ret != PPMACcontrolNoError)
        return PPMACcontrolPMACUnexpectedReplyError;
    if (ss.size() != 2)
        return PPMACcontrolPMACUnexpectedReplyError;
    
    double maxd = 0.0;
    double mind = 0.0;
    std::istringstream stream_max(ss.at(0));
    stream_max >> maxd;
    if (stream_max.fail())
    {
        //failed to convert to double value
        return PPMACcontrolPMACUnexpectedReplyError;
    }

    std::istringstream stream_min(ss.at(1));
    stream_min >> mind;
    if (stream_min.fail())
    {
        //failed to convert to double value
        return PPMACcontrolPMACUnexpectedReplyError;
    }

    maxpos = maxd;
    minpos = mind;
    return PPMACcontrolNoError;
}

/**
 * @brief Set software limits of an axis.
 * 
 * Power PMAC command string sent = "Motor[<axis index>].MaxPos=<maxpos> Motor[<axis index>].MinPos=<minpos>"
 *
 * @param axis - Index of axis.
 * @param maxpos - New maximum software limit
 * @param minpos - New minimum software limit
 * @return If the new software limits are set successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisSetSoftwareLimits(int axis, double maxpos, double minpos){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisSetSoftwareLimits";
    debugPrint_ppmaccomm("%s called", functionName);
    char cmd[128] = {0};
    sprintf( cmd, "Motor[%d].MaxPos=%f Motor[%d].MinPos=%f\n", axis, maxpos, axis, minpos);
  
    return writeRead(cmd);
}


/**
 * @brief Get current position of an axis.
 * 
 * If 'inf' or 'nan' is received from the Power PMAC, 
 * this function will return PPMACcontrolPMACUnexpectedReplyError (-231) and 
 * the position parameter will not be set. \n
 * Power PMAC command string sent = "#<axis index>p"
 *
 * @param axis - index of axis
 * @param position - Current position of the axis
 * @return If the current position is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisGetCurrentPosition(int axis, double& position){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisGetCurrentPosition";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = "";
    sprintf( cmd, "#%dp\n", axis);
    
    std::string reply;
    int ret = this->writeRead(cmd,reply);
    if (ret != PPMACcontrolNoError)
        return ret;

    std::istringstream stream(reply);
    double dval;
    stream >> dval;
    if (stream.fail())
    {
        //failed to convert to double value
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    position = dval;
    return PPMACcontrolNoError;
    
}

/**
 * @brief Move axis to a specified position
 * 
 * Power PMAC command string sent = "#<axis index>j=<position>"
 *
 * @param axis - Index of axis.
 * @param position - New position
 * @return If the new position is set successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisMoveAbs(int axis, double position){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisMoveAbs";
    debugPrint_ppmaccomm("%s called", functionName);

    char cmd[128] = {0};
    sprintf( cmd, "#%dj=%.2f\n", axis, position);
  
    return writeRead(cmd);
}

/**
 * @brief Move axis by a specified position
 * 
 * Power PMAC command string sent = "#<axis index>j^=<position>"
 *
 * @param axis - Index of axis.
 * @param relposition - Relative position
 * @return If the new position is set successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisMoveRel(int axis, double relposition){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisMoveRel";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dj^%.2f\n", axis, relposition);
  
    return writeRead(cmd);
}

/**
 * @brief Move axis forward
 * 
 * Power PMAC command string sent = "#<axis index>j+"
 *
 * @param axis - Index of axis.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisMovePositive(int axis){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisMovePositive";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dj+\n", axis);
  
    return writeRead(cmd);
}

/**
 * @brief Move axis backward
 *
 * Power PMAC command string sent = "#<axis index>j-"
 * 
 * @param axis - Index of axis.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisMoveNegative(int axis){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisMoveNegative";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dj-\n", axis);
  
    return writeRead(cmd);
}


/**
 * @brief Execute homing procedure on the specified axis
 * 
 * Power PMAC command string sent = "#<axis index>hm"
 *
 * @param axis - Index of axis.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisHome(int axis){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisHome";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dhm\n", axis);
  
    return writeRead(cmd);
}

/**
 * @brief Stop current movement of the specified axis
 *
 * Power PMAC command string sent = "#<axis index>j/"
 * 
 * @param axis - Index of axis.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisStop(int axis){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisStop";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dj/\n", axis);
  
    return writeRead(cmd);
}

/**
 * @brief Stop current movement of the specified axis
 * 
 * Power PMAC command string sent = "#<axis index>k"
 *
 * @param axis - Index of axis.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axisAbort(int axis){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axisAbort";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%dk\n", axis);
  
    return writeRead(cmd);
}

/**
 * @brief Enable a PLC program
 *
 * Power PMAC command string sent = "enable plc <plc number>"
 * 
 * @param plcnum - PLC number.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_enablePlc(int plcnum){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_enablePlc";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "enable plc %d\n", plcnum);
  
    return writeRead(cmd);
}

/**
 * @brief Disable a PLC program
 * 
 * Power PMAC command string sent = "disable plc <plc number>"
 *
 * @param plcnum - PLC number.
 * @return If the command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_disablePlc(int plcnum){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_disablePlc";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "disable plc %d\n", plcnum);
  
    return writeRead(cmd);
}


/**
 * @brief Get status of a PLC. 
 * 
 * Power PMAC command string sent = "Plc[<plc number>].Active Plc[<plc number>].Running"
 *
 * @param plcnum - PLC number
 * @param active - The active state of the PLC
 * @param running - The running state of the PLC
 * @return If the PLC states are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_plcState(int plcnum, bool& active, bool& running){
static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_plcState";
    debugPrint_ppmaccomm("%s called", functionName);
    char cmd[128] = {0};
    sprintf( cmd, "Plc[%d].Active Plc[%d].Running\n", plcnum, plcnum);
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    int int_active = 0;
    int int_running = 0;
    int n = sscanf( reply.c_str(), "%d\n%d", &int_active, &int_running);
    if (n < 2)
    {
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    if (int_active==0)
        active = false;
    else if (int_active == 1)
        active = true;
    else
        return PPMACcontrolPMACUnexpectedReplyError;
    
    if (int_running==0)
        running = false;
    else if (int_running == 1)
        running = true;
    else
        return PPMACcontrolPMACUnexpectedReplyError;
    
    return PPMACcontrolNoError;
    
}

/**
 * @brief Get status of a motion program associated with the specified coordinate system. 
 * 
 * Power PMAC command string sent = "Coord[<cs number>].ProgActive Coord[<cs number>].ProgRunning"
 *
 * @param ncoord - Coordinate system number
 * @param active - The active state of the motion program
 * @param running - The running state of the motion program
 * @return If the program states are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_mprogState(int ncoord, bool& active, bool& running){
static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_mprogState";
    debugPrint_ppmaccomm("%s called", functionName);
    char cmd[128] = {0};
    sprintf( cmd, "Coord[%d].ProgActive Coord[%d].ProgRunning\n", ncoord, ncoord);
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    int int_active = 0;
    int int_running = 0;
    int n = sscanf( reply.c_str(), "%d\n%d", &int_active, &int_running);
    if (n < 2)
    {
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    if (int_active==0)
        active = false;
    else if (int_active == 1)
        active = true;
    else
        return PPMACcontrolPMACUnexpectedReplyError;
    
    if (int_running==0)
        running = false;
    else if (int_running == 1)
        running = true;
    else
        return PPMACcontrolPMACUnexpectedReplyError;
    
    return PPMACcontrolNoError;
    
}

/**
 * @brief Run a motion program associated with the specified coordinate system
 * 
 * Power PMAC command string sent = "&<cs number>r"
 *
 * @param ncoord - Coordinate system number
 * @return If the command to run the motion program is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_runMprog(int ncoord){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_runMprog";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "&%dr\n", ncoord);
  
    return writeRead(cmd);
}

/**
 * @brief Abort a motion program associated with the specified coordinate system
 * 
 * Power PMAC command string sent = "&<cs number>a"
 *
 * @param ncoord - Coordinate system number
 * @return If the command to abort the motion program is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_abortMprog(int ncoord){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_abortMprog";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "&%da\n", ncoord);
  
    return writeRead(cmd);
}

/**
 * @brief Reset the control application in Power PMAC
 * 
 * Power PMAC command string sent = "$$$"
 *
 * @return If the reset command is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_reset(){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_reset";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "$$$\n");
  
    int ret = writeRead(cmd);
    if (ret == PPMACcontrolSSHDriverErrorReadTimeout)
    {
        //Read timeout is expected when $$$ command succeeded
        return PPMACcontrolNoError;    //Timeout is expected 
    }
    return ret;

}

/**
 * @brief Stop (abrupt) all axis
 * 
 * Power PMAC command string sent = "#*k"
 *
 * @return If the command to stop all axis is sent to Power PMAC successfully, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_stopAllAxes(){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_stopAllAxes";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#*k\n");
  
    return writeRead(cmd);
}

/**
 * @brief Get global status
 *
 * Power PMAC command string sent = "?"
 * 
 * @param status - Global status
 * @return If the global status is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getGlobalStatus(uint32_t& status){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_getGlobalStatus";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "?\n");
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    if (reply.length() != 9)
    {
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    char cstatus[16] = "";
    reply.copy(cstatus, 8, 1);
    if (sscanf(cstatus, "%8x", &status) != 1 )
        return PPMACcontrolPMACUnexpectedReplyError;
    return PPMACcontrolNoError;
    
}

/**
 * @brief Get status of the specified motor
 *
 * Power PMAC command string sent = "#<axis index>?"
 * 
 * @param motor - Motor number
 * @param status - Status of the motor
 * @return If the motor status is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getMotorStatus(int motor, uint64_t& status){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_getMotorStatus";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "#%d?\n", motor);
    
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    if (reply.length() != 17)
    {
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    char cstatus_h[16] = "";
    char cstatus_l[16] = "";
    
    reply.copy(cstatus_h, 8, 1);
    reply.copy(cstatus_l, 8, 9);
    
    uint32_t h;
    uint32_t l;
    
    if (sscanf(cstatus_h, "%8x", &h) != 1 )
        return PPMACcontrolPMACUnexpectedReplyError;
    
    if (sscanf(cstatus_l, "%8x", &l) != 1 )
        return PPMACcontrolPMACUnexpectedReplyError;
    
    status = (((uint64_t)h)<<32) | ((uint64_t)l);

    return PPMACcontrolNoError;
    
}

/**
 * @brief Get status of the specified coordinate system
 * 
 * Power PMAC command string sent = "&<cs number>?"
 *
 * @param Cs - Coordinate system number
 * @param status - Status of the coordinate system
 * @return If the coordinate system status is successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getCoordStatus(int Cs, uint64_t& status){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_getCoordStatus";
    debugPrint_ppmaccomm("%s called", functionName);
    
    char cmd[128] = {0};
    sprintf( cmd, "&%d?\n", Cs);
    
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    if (reply.length() != 17)
    {
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    char cstatus_h[16] = "";
    char cstatus_l[16] = "";
    
    reply.copy(cstatus_h, 8, 1);
    reply.copy(cstatus_l, 8, 9);
    
    uint32_t h;
    uint32_t l;
    
    if (sscanf(cstatus_h, "%8x", &h) != 1 )
        return PPMACcontrolPMACUnexpectedReplyError;
    
    if (sscanf(cstatus_l, "%8x", &l) != 1 )
        return PPMACcontrolPMACUnexpectedReplyError;
    
    status = (((uint64_t)h)<<32) | ((uint64_t)l);

    return PPMACcontrolNoError;
    
}

/**
 * @brief Get velocities of multiple axes.
 * 
 * The velocities of axes between 
 * the specified first axis number and last axis number will be 
 * added to the parameter velocities. If 'inf' or 'nan' is received from the Power PMAC, 
 * this function will return PPMACcontrolPMACUnexpectedReplyError (-231) and 
 * the velocities parameter will be empty. \n
 * The maximum number of axes is 32. If the number of axes is larger than 32, this function will return PPMACcontrolInvalidParamError (-242) 
 * Power PMAC command string sent = "Motor[<first axis index>].JogSpeed Motor[<first axis index + 1>].JogSpeed ... Motor[<last axis index>].JogSpeed"
 *
 * @param firstAxis - The number of first axis
 * @param lastAxis - The number of lasst axis
 * @param velocities - Velocities of the axes between the first axis and the last axis.
 * This parameter is cleared first then velocities of the axes are added.
 * @return If the velocities are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolOutOfOrderError (-233)
 *      - PPMACcontrolSemaphoreTimeoutError (-239)
 *      - PPMACcontrolSemaphoreError (-240)
 *      - PPMACcontrolSemaphoreReleaseError (-241)
 *      - PPMACcontrolInvalidParamError (-242)
 */
int PowerPMACcontrol::PowerPMACcontrol_axesGetVelocities(int firstAxis, int lastAxis, std::vector<double>& velocities){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axesGetVelocities";
    debugPrint_ppmaccomm("%s called", functionName);
    velocities.clear();
    
    if (firstAxis > lastAxis)
    {
        return PPMACcontrolOutOfOrderError;
    }
    
    if ( (lastAxis - firstAxis + 1) > MAX_ITEM_NUM )
    {
        debugPrint_ppmaccomm("%s : Too many axes, %d", functionName, (lastAxis - firstAxis + 1));
        return PPMACcontrolInvalidParamError;
    }
    
    std::string comstring="";
    for (int i=firstAxis; i<=lastAxis; i++ )
    {
        char part[128] = {0};
        sprintf(part, "Motor[%d].JogSpeed ", i);
        comstring += part;
    }
    comstring+="\n";
    
    std::string reply;
    debugPrint_ppmaccomm("comstring.length() is %d\n", comstring.length());
    int ret = writeRead(comstring.c_str(), reply);
    if (ret != PPMACcontrolNoError)
        return ret;    
    
    //Check to see if there are correct number of items in the return string
    int axis_nums = lastAxis-firstAxis+1;
    std::vector<std::string> statusstrings;
    
    ret = PowerPMACcontrol::splitit(reply, " \n\r", statusstrings);
    
    // Check if it was split correctly
    if (ret != PPMACcontrolNoError)
        return PPMACcontrolPMACUnexpectedReplyError;
    // Check if there is correct number of status
    if ((int)statusstrings.size() != axis_nums)
        return PPMACcontrolPMACUnexpectedReplyError;
    
    try{
        for (size_t i=0; i<statusstrings.size(); i++)
        {

            double d;
            std::string axisstatus = statusstrings.at(i);
            std::istringstream axisstream(axisstatus);
            
            axisstream >> d;
            if (axisstream.fail())
            {
                //failed to convert to double value
                velocities.clear();
                return PPMACcontrolPMACUnexpectedReplyError;
            }
            velocities.push_back(d);
        }
    }
    catch(...)
    {
        debugPrint_ppmaccomm("%s : Error trying to get Multiple Motor Velocities", functionName);
        velocities.clear();
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    
    return PPMACcontrolNoError;
}

/**
 * @brief Get position of multiple axes.
 * 
 * The position of axes between 
 * the specified first axis number and last axis number will be 
 * added to the parameter positions.
 * If 'inf' or 'nan' is received from the Power PMAC, 
 * this function will return PPMACcontrolPMACUnexpectedReplyError (-231) and 
 * the positions parameter will be empty. \n
 * Power PMAC command string sent = "#<first axis index>..<last axis index>p"
 *
 * @param firstAxis - The number of first axis
 * @param lastAxis - The number of last axis
 * @param positions - Positions of the axes between the first axis and the last axis.
 * This parameter is cleared first then positions of the axes are added.
 * @return If the positions are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_axesGetCurrentPositions(int firstAxis, int lastAxis, std::vector<double>& positions){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_axesGetCurrentPositions";
    debugPrint_ppmaccomm("%s called", functionName);
    positions.clear();
   
    char cmd[128] = {0};
    sprintf( cmd, "#%d..%dp\n", firstAxis, lastAxis);
 
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;    
    
    //Check to see if there are correct number of items in the return string
    int axis_nums = lastAxis-firstAxis+1;
    std::vector<std::string> positionstrings;
    
    ret = PowerPMACcontrol::splitit(reply, " \n\r", positionstrings);
    
    // Check if it was split correctly
    if (ret != PPMACcontrolNoError)
        return PPMACcontrolPMACUnexpectedReplyError;
    // Check if there is correct number of status
    if ((int)positionstrings.size() != axis_nums)
        return PPMACcontrolPMACUnexpectedReplyError;
    
    try{
        for (size_t i=0; i<positionstrings.size(); i++)
        {
            double d;
            std::string pos = positionstrings.at(i);
            std::istringstream posstream(pos);
            posstream >> d;
            if (posstream.fail())
            {
                //failed to convert to double value
                posstream.clear();
                return PPMACcontrolPMACUnexpectedReplyError;
            }
            positions.push_back(d);
        }
    }
    catch(...)
    {
        debugPrint_ppmaccomm("%s : Error trying to get Axes Current Positions", functionName);
        positions.clear();
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    
    return PPMACcontrolNoError;
}

/**
 * @brief Get status of multiple motors.
 * 
 * The status of motors between 
 * the specified first motor number and last motor number will be 
 * added to the parameter status.\n
 * Power PMAC command string sent = "#<first motor index>..<last motor index>?"
 *
 * @param firstMotor - The number of first motor
 * @param lastMotor - The number of last motor
 * @param status - Status of the motors between the first motor number and the last motor number.
 * This parameter is cleared first then status of motors are added.
 * @return If the status are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getMultiMotorStatus(int firstMotor, int lastMotor, std::vector<uint64_t>& status){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_getMultiMotorStatus";
    debugPrint_ppmaccomm("%s called", functionName);
    status.clear();
    char cmd[128] = {0};
    sprintf( cmd, "#%d..%d?\n", firstMotor, lastMotor);
    
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    
    //Check to see if there are correct number of items in the return string
    int axis_nums = lastMotor-firstMotor+1;
    std::vector<std::string> statusstrings;
    
    ret = PowerPMACcontrol::splitit(reply, " ", statusstrings);
    if (ret != PPMACcontrolNoError)
        return PPMACcontrolPMACUnexpectedReplyError;
    if ((int)statusstrings.size() != axis_nums)
        return PPMACcontrolPMACUnexpectedReplyError;
    
    try{
        for (size_t i=0; i<statusstrings.size(); i++)
        {
            std::string axisstatus = statusstrings.at(i);
            if (axisstatus.length() != 17)
            {
                return PPMACcontrolPMACUnexpectedReplyError;
            }
            char cstatus_h[16] = "";
            char cstatus_l[16] = "";

            axisstatus.copy(cstatus_h, 8, 1);
            axisstatus.copy(cstatus_l, 8, 9);

            uint32_t h;
            uint32_t l;

            if (sscanf(cstatus_h, "%8x", &h) != 1 )
                return PPMACcontrolPMACUnexpectedReplyError;

            if (sscanf(cstatus_l, "%8x", &l) != 1 )
                return PPMACcontrolPMACUnexpectedReplyError;

            uint64_t uintstatus = (((uint64_t)h)<<32) | ((uint64_t)l);
            status.push_back(uintstatus);
        }
    }
    catch(...)
    {
        debugPrint_ppmaccomm("%s : Error trying to get Multiple Motor Status", functionName);
        status.clear();
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    return PPMACcontrolNoError;
    
}

/**
 * @brief Get status of multiple coordinate systems.
 * 
 * The status of coordinate system between 
 * the specified first coordinate system number and last coordinate system number will be 
 * added to the parameter status.\n
 * Power PMAC command string sent = "&<first cs number>..<last cs number>?"
 *
 * @param firstCs - The number of first coordinate system
 * @param lastCs - The number of last coordinate system
 * @param status - Status of the coordinate systems between the specified first 
 * and the last coordinate system number.
 * This parameter is cleared first then status of coordinate systems are added.
 * @return If the status are successfully received, 
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getMultiCoordStatus(int firstCs, int lastCs, std::vector<uint64_t>& status){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_getMultiCoordtatus";
    debugPrint_ppmaccomm("%s called", functionName);
    status.clear();
    char cmd[128] = {0};
    sprintf( cmd, "&%d..%d?\n", firstCs, lastCs);
    
    
    std::string reply;
    int ret = writeRead(cmd, reply);
    if (ret != PPMACcontrolNoError)
        return ret;
    
    //Check to see if there are correct number of items in the return string
    int axis_nums = lastCs-firstCs+1;
    std::vector<std::string> statusstrings;
    
    ret = PowerPMACcontrol::splitit(reply, " ", statusstrings);
    if (ret != PPMACcontrolNoError)
        return PPMACcontrolPMACUnexpectedReplyError;
    if ((int)statusstrings.size() != axis_nums)
        return PPMACcontrolPMACUnexpectedReplyError;
    
    try{
        for (size_t i=0; i<statusstrings.size(); i++)
        {
            std::string coordstatus = statusstrings.at(i);
            if (coordstatus.length() != 17)
            {
                return PPMACcontrolPMACUnexpectedReplyError;
            }
            char cstatus_h[16] = "";
            char cstatus_l[16] = "";

            coordstatus.copy(cstatus_h, 8, 1);
            coordstatus.copy(cstatus_l, 8, 9);

            uint32_t h;
            uint32_t l;

            if (sscanf(cstatus_h, "%8x", &h) != 1 )
                return PPMACcontrolPMACUnexpectedReplyError;

            if (sscanf(cstatus_l, "%8x", &l) != 1 )
                return PPMACcontrolPMACUnexpectedReplyError;

            uint64_t uintstatus = (((uint64_t)h)<<32) | ((uint64_t)l);
            status.push_back(uintstatus);
        }
    }
    catch(...)
    {
        debugPrint_ppmaccomm("%s : Error trying to get Multiple Motor Status", functionName);
        status.clear();
        return PPMACcontrolPMACUnexpectedReplyError;
    }
    return PPMACcontrolNoError;
    
}


/**
 * @brief Remote download a motion/plc program. 
 * 
 * This function expects 'open' command is
 * written in the file at the beginning of the program. This function 
 * always send 'close' command before returning if the function has attempted to write
 * to Power PMAC. \n
 * Comments in the source program can only be in the form of line beginning "//". 
 * It does not support '/' followed by '*' style comments.
 *
 * @param filepath - Path to the program file to be downloaded to Power PMAC.
 * @return If the program file is successfully downloaded,
 * PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113) 
 *      - PPMACcontrolPMACUnexpectedReplyError (-231)
 *      - PPMACcontrolFileOpenError (-234)
 *      - PPMACcontrolFileReadError = (-235)
 *      - PPMACcontrolProgramCloseError = (-236)  
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_progDownload(std::string filepath){
    static const char *functionName = "PowerPMACcontrol::PowerPMACcontrol_progDownload";
    if (this->connected == 0)
    {
        debugPrint_ppmaccomm("%s : PMAC is not connected", functionName);
        return PPMACcontrolNoSSHDriverSet;
    }
    std::ifstream progfile;
    progfile.open(filepath.c_str());
    std::string line;
    std::string towrite;
    std::string reply;
    int ret = 0;
    bool written = false;
    if (progfile.is_open())
    {
#ifdef WIN32
		DWORD dwWaitResult = WaitForSingleObject(this->ghSemaphore, SEMAPHORE_WAIT_MSEC);
        if (dwWaitResult == WAIT_OBJECT_0)

#else
        struct timespec ts = getAbsTimeout(SEMAPHORE_WAIT_MSEC);
        int sem_return = sem_timedwait(&sem_writeRead, &ts);
        if (sem_return == 0)
#endif	
       {
			getline(progfile, line);
			while (!progfile.eof())
			//while (progfile.good())
			{
				if (progfile.fail())
				{
					//Error while reading 
					ret = PPMACcontrolFileReadError;
					break;
				}
				else
				{
					towrite = line;
					getline(progfile,line);
					size_t length = towrite.length();
					if (length > 0)
					{

						const char * carray = towrite.c_str();
						if (carray[length-1] != '\n')
						{
							debugPrint_ppmaccomm("%s : adding cr to %s\n", functionName, towrite.c_str());
							towrite = towrite + "\n";

						}
						written = true;
						ret = writeRead_WithoutSemaphore(towrite.c_str(), reply);
						if (ret != PPMACcontrolNoError)
						{
							break;
						}
					}
				}
			}
            if (written)
            {
                //Always call 'close' command
                int ret2 = writeRead_WithoutSemaphore("close\n",reply);
                if ( ret2 != PPMACcontrolNoError)
                {
                    debugPrint_ppmaccomm("%s : Error while writing 'close'. error number %d\n", functionName, ret2);
                    ret = PPMACcontrolProgramCloseError;
                }
            }
#ifdef WIN32
            if (!ReleaseSemaphore(ghSemaphore, 1, NULL))
            {
                debugPrint_ppmaccomm("%s : Error releasing Semaphore\n",functionName);
                ret = PPMACcontrolSemaphoreReleaseError;
            }
#else
            if (sem_post(&sem_writeRead)!=0)    //Releasing semaphore
            {
                debugPrint_ppmaccomm("%s : Error releasing Semaphore\n",functionName);
                ret = PPMACcontrolSemaphoreReleaseError;
            }
            else
            {
                debugPrint_ppmaccomm("%s : Released Semaphore\n",functionName);
            }
#endif
       }
#ifdef WIN32
        else if (dwWaitResult == WAIT_TIMEOUT)
		{
            debugPrint_ppmaccomm("%s : Semaphore timed out\n",functionName);
			ret = PPMACcontrolSemaphoreTimeoutError;
		}
		else
		{
			debugPrint_ppmaccomm("%s : Semaphore error (%d)\n",functionName, dwWaitResult);
			ret = PPMACcontrolSemaphoreError;
		}
#else
        else    //Semaphore error
        {
            if (errno == ETIMEDOUT)
            {
                debugPrint_ppmaccomm("%s : Semaphore timed out\n",functionName);
                ret = PPMACcontrolSemaphoreTimeoutError;
            }
            else
            {	
                debugPrint_ppmaccomm("%s : Semaphore error (%d)\n",functionName, errno);
                ret = PPMACcontrolSemaphoreError;
            }
        }
#endif
        progfile.close();
    }
    else    //Program file open failed.
    {
        debugPrint_ppmaccomm("%s : unable to open the file %s\n", functionName, filepath.c_str());
        return PPMACcontrolFileOpenError;
    }
    if (ret < 0 && ret > -100 ) //This is a pmac error
    {
        debugPrint_ppmaccomm("%s : Error while sending program. error number %d\n", functionName, ret);
    }
    
    return ret;
}
/**
 * @brief Write data to the connected SSH channel and read the reply. 
 * Caller of this function must obtain semaphore before calling this function.
 * 
 * A timeout should be specified in milliseconds.
 * @param cmd - The string buffer to be written.
 * @param response - The response read from the SSH channel
 * @param timeout - A timeout in ms for the write. The default value is 1000 and may be updated using PowerPMACcontrol_setTimeout.
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 */
int PowerPMACcontrol::writeRead_WithoutSemaphore(const char *cmd, std::string& response, int timeout){
    static const char *functionName = "PowerPMACcontrol::writeRead(char, string, int)";
    debugPrint_ppmaccomm("%s writing %s\n", functionName, cmd);
    if (this->connected == 0)
    {
        debugPrint_ppmaccomm("%s : PMAC is not connected", functionName);
        return PPMACcontrolNoSSHDriverSet;
    }
    // If no timeout specified, use the common value
    if (timeout == TIMEOUT_NOT_SPECIFIED)
    {
    	timeout = common_timeout_ms;
    }

    size_t bytes = 0;
	int return_num = PPMACcontrolNoError;
	char buff[5120] = "";
    int ret = this->PowerPMACcontrol_write(cmd, strlen(cmd), &bytes, timeout);
    if (ret != PPMACcontrolNoError)
    {
        debugPrint_ppmaccomm("%s : Failed to write to powerPmac command (%s)\n", functionName, cmd);
        return_num = ret;
    }
    else
    {
        ret = this->PowerPMACcontrol_read(buff, 5120, &bytes, 0x06, timeout);
        if (ret != PPMACcontrolNoError)
        {
            debugPrint_ppmaccomm("%s : Failed to read from powerPmac\n", functionName);
            return_num=ret;
        }
        else
        {
            debugPrint_ppmaccomm("%s : The reply from PowerPMAC for %s is [%s]\n", functionName,cmd, buff);
            std::string trimmed = trim_right_copy(std::string(buff));
            response = trimmed;
            int pmac_err_num = PowerPMACcontrol::check_PowerPMAC_error( trimmed );
            if ( pmac_err_num != 0 )
            {
                return_num = (-1)*pmac_err_num;
            }
        }
    }
	return return_num;
}


/**
 * @brief Write data to the connected SSH channel and read the reply.
 * 
 * A timeout should be specified in milliseconds.
 * @param cmd - The string buffer to be written.
 * @param response - The response read from the SSH channel
 * @param timeout - A timeout in ms for the write. The default value is 1000 and may be updated using PowerPMACcontrol_setTimeout.
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::writeRead(const char *cmd, std::string& response, int timeout){
    static const char *functionName = "PowerPMACcontrol::writeRead(char, string, int)";
    debugPrint_ppmaccomm("%s writing %s\n", functionName, cmd);
    if (this->connected == 0)
    {
        debugPrint_ppmaccomm("%s : PMAC is not connected", functionName);
        return PPMACcontrolNoSSHDriverSet;
    }
    // If no timeout specified, use the common value
    if (timeout == TIMEOUT_NOT_SPECIFIED)
    {
    	timeout = common_timeout_ms;
    }

    size_t bytes = 0;
    
    //Get semaphore
#ifdef WIN32    
	DWORD dwWaitResult = WaitForSingleObject(this->ghSemaphore, SEMAPHORE_WAIT_MSEC);
#else
    struct timespec ts = getAbsTimeout(SEMAPHORE_WAIT_MSEC);
    int sem_return = sem_timedwait(&sem_writeRead, &ts);
#endif
	int return_num = PPMACcontrolNoError;   //initialisation
	char buff[5120] = "";
    
    //Check semaphore error
#ifdef WIN32 
	if (dwWaitResult == WAIT_OBJECT_0)
#else
    if (sem_return == 0)    
#endif
    {   //Semaphore OK
		int ret = this->PowerPMACcontrol_write(cmd, strlen(cmd), &bytes, timeout);
		if (ret != PPMACcontrolNoError)
		{
			debugPrint_ppmaccomm("%s : Failed to write to powerPmac command (%s)\n", functionName, cmd);
			return_num = ret;
		}
		else
		{
			ret = this->PowerPMACcontrol_read(buff, 5120, &bytes, 0x06, timeout);
			if (ret != PPMACcontrolNoError)
			{
				debugPrint_ppmaccomm("%s : Failed to read from powerPmac\n", functionName);
				return_num=ret;
			}
			else
			{
				debugPrint_ppmaccomm("%s : The reply from PowerPMAC for %s is [%s]\n", functionName,cmd, buff);
				std::string trimmed = trim_right_copy(std::string(buff));
				response = trimmed;
				int pmac_err_num = PowerPMACcontrol::check_PowerPMAC_error( trimmed );
				if ( pmac_err_num != 0 )
				{
					return_num = (-1)*pmac_err_num;
				}
			}
		}
        
        //Releasing the semaphore
#ifdef WIN32 
		if (!ReleaseSemaphore(ghSemaphore, 1, NULL))
		{
			debugPrint_ppmaccomm("%s : Error releasing Semaphore\n",functionName);
            return_num = PPMACcontrolSemaphoreReleaseError;
		}
        else
        {
            debugPrint_ppmaccomm("%s : Released Semaphore\n",functionName);
        }
#else
        if (sem_post(&sem_writeRead)!=0)
        {
            debugPrint_ppmaccomm("%s : Error releasing Semaphore\n",functionName);
            return_num = PPMACcontrolSemaphoreReleaseError;
        }
        else
        {
            debugPrint_ppmaccomm("%s : Released Semaphore\n",functionName);
        }
#endif
		return return_num;
	}
#ifdef WIN32 
	else if (dwWaitResult == WAIT_TIMEOUT)
	{
        debugPrint_ppmaccomm("%s : Semaphore timed out\n",functionName);
        return PPMACcontrolSemaphoreTimeoutError;
	}	
    else
	{
        debugPrint_ppmaccomm("%s : Error obtaining a Semaphore (%d)\n",functionName, dwWaitResult);
        return PPMACcontrolSemaphoreError;
	}
#else
    else
	{
        if (errno == ETIMEDOUT)
        {
			debugPrint_ppmaccomm("%s : Semaphore timed out\n",functionName);
			return PPMACcontrolSemaphoreTimeoutError;
        }
        else
        {	
            debugPrint_ppmaccomm("%s : Semaphore error %d\n",functionName, errno);
			return PPMACcontrolSemaphoreError;
        }
	}
#endif
}

/**
 * @brief Write data to the connected SSH channel and read the reply. 
 * 
 * A timeout should be specified in milliseconds.
 * @param cmd - The string buffer to be written.
 * @param timeout - A timeout in ms for the write. The default value is 1000 and may be updated using PowerPMACcontrol_setTimeout.
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::writeRead(const char *cmd, int timeout){
    static const char *functionName = "PowerPMACcontrol::writeRead(char*, int)";
    debugPrint_ppmaccomm("%s called", functionName);
    // If no timeout specified, use the common value
    if (timeout == TIMEOUT_NOT_SPECIFIED)
    {
    	timeout = common_timeout_ms;
    }
    std::string reply = "";
    return this->writeRead(cmd, reply, timeout);
}


/**
 * @brief Send a command to the connected SSH channel and read the reply. 
 * 
 * If command string is zero length, this function will not send the 
 * command and return PPMACcontrolNoError(0).\n
 * Power PMAC command string sent = "<command>"
 * 
 * @param command - The string buffer to be written.
 * @param reply - The reply from the SSH channel.
 * @return If successful, PPMACcontrolNoError(0) is returned. If not, 
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError (0)
 *      - Error reported from Power PMAC (-1 to -99) -1*(Power PMAC error number)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_sendCommand(const std::string command, std::string& reply){
    static const char *functionName = "PowerPMACcontrol::writeRead(char*, int)";
    debugPrint_ppmaccomm("%s called", functionName);
    std::string cmd = command;
    int length = cmd.length();
    if (length > 0)
    {
        if ( cmd.at(length-1) != '\n' )
        {
            cmd = cmd + "\n";//Add the new line
        }
        return this->writeRead(cmd.c_str(), reply);
    }
    else    //Don't send a command of 0 length. Return no error.
        return PPMACcontrolNoError;
}

/**
 * @brief Checks if a string has a "error #". 
 * 
 * This function can be used to check a reply string from the Power PMAC if it reports any error. 
 * 
 * @param s - The string to be checked.
 * @return Error number found in the string. If no error is found in the string, 0.
 */
int PowerPMACcontrol::check_PowerPMAC_error(const std::string s){
    static const char *functionName = "PowerPMACcontrol::check_PowerPMAC_error";
    
    
    std::string error_str("error #");
    size_t index = s.find(error_str);
    if (index == std::string::npos)
    {
        debugPrint_ppmaccomm("%s : No error\n", functionName);
        return 0;       //No error
    }
    
    //get error number
    std::string s2 = s.substr(index + error_str.length());
    size_t index2 = s2.find(":");      //get index of ':'
    
    if (index2 == std::string::npos)
    {
        debugPrint_ppmaccomm("%s : No error (couldn't find ':').\n", functionName);
        return 0;       //No error
    }
    
    std::string numstring = s2.substr(0, index2);
    debugPrint_ppmaccomm("%s : numstring is %s\n", functionName, numstring.c_str());

    int error_number;
    
    std::istringstream(numstring) >> error_number;
   
    return error_number;
    
}


/**
 * @brief Split a string by a specified separator. 
 * 
 * The split strings are set in the strings parameter. 
 * 
 * @param s - The string to be split.
 * @param separator - Separator to be used to split the string
 * @param strings - String items after the input string s is split
 * @return If successful, PPMACcontrolNoError(0), if not, PPMACcontrolSplitterError(-237).
 */
int PowerPMACcontrol::splitit(std::string s, std::string separator, std::vector<std::string> &strings){
    static const char *functionName = "PowerPMACcontrol::splitit";
    if (strings.size() > 0)
        strings.clear();
    
    if (s.length() < 1)
        return PPMACcontrolNoError;
    
    size_t start_index;
    size_t end_index;
    
    start_index = s.find_first_not_of(separator);
    end_index = s.find_first_of(separator, start_index);
    debugPrint_ppmaccomm("%s : start_index = %d\n", functionName, start_index);
    debugPrint_ppmaccomm("%s : end_index = %d\n", functionName, end_index);
    try{
    while(start_index != std::string::npos)
    {
        if (end_index != std::string::npos)
        {
            debugPrint_ppmaccomm("%s : item (%s) found\n", functionName, s.substr(start_index, end_index-start_index).c_str());
            strings.push_back(s.substr(start_index, end_index-start_index));
        }
        else
        {
            strings.push_back(s.substr(start_index, std::string::npos));
            debugPrint_ppmaccomm("%s : item (%s) found\n", functionName, s.substr(start_index, end_index-start_index).c_str());
            break;
        }
        start_index = s.find_first_not_of(separator, end_index);
        end_index = s.find_first_of(separator, start_index);
        
    }
    }
    catch(...)
    {
        strings.clear();
        return PPMACcontrolSplitterError;
    }
    return PPMACcontrolNoError;
}

/**
 * @brief Get the current CPU operational temperature
 *
 * Power PMAC command string sent = "Sys.CpuTemp"
 * @param temperature - The temperature in Celsius will be written to this parameter
 * @return If temperature is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getCPUTemperature(double& temperature){
	//static const char *functionName = "PowerPMACcontrol_getCPUTemperature";
	static const char *functionName = __FUNCTION__;
    debugPrint_ppmaccomm("%s called\n", functionName);

    // Send command and read response
    return this->PowerPMACcontrol_getVariable("Sys.CpuTemp", temperature);

}

/**
 * @brief Get the time from power-on to present
 *
 * Power PMAC command string sent = "Sys.Time"
 * @param runningTime - The time in seconds since power-on will be written to this parameter.
 * @return If value is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getRunningTime(double& runningTime){
	static const char *functionName = "PowerPMACcontrol_getRunningTime";
    debugPrint_ppmaccomm("%s called\n", functionName);

    // Send command and read response
    return this->PowerPMACcontrol_getVariable("Sys.Time", runningTime);

}

/**
 * @brief Get CPU usage percentage
 *
 * @param CPUUsage - The averaged fraction of available CPU time used by all PMAC tasks in percent will be written to this parameter.
 * @return If value is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getCPUUsage(double& CPUUsage){
    TaskCalculator tasks(this);
    CPUUsage = tasks.getCPUUsageByPmacTasks();

    return tasks.getErrorStatus();
}

/**
 * @brief Get the percentage of total CPU time used by phase tasks
 *
 * @param phaseTaskUsage - The averaged fraction of total CPU time used by phase tasks in percent will be written to this parameter
 * @return If value is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getPhaseTaskUsage(double& phaseTaskUsage){
	static const char *functionName = "PowerPMACcontrol_getPhaseTime";
    debugPrint_ppmaccomm("%s called\n", functionName);

    TaskCalculator tasks(this);
    phaseTaskUsage = tasks.getPhaseTaskUsage();

    return tasks.getErrorStatus();

}

/**
 * @brief Get the percentage of total CPU time used by servo tasks
 *
 * @param servoTaskUsage - The averaged fraction of total CPU time used by servo tasks in percent will be written to this parameter
 * @return If value is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getServoTaskUsage(double& servoTaskUsage){
	static const char *functionName = "PowerPMACcontrol_getServoTime";
    debugPrint_ppmaccomm("%s called\n", functionName);

    TaskCalculator tasks(this);
    servoTaskUsage = tasks.getServoTaskUsage();

    return tasks.getErrorStatus();

}

/**
 * @brief Get the percentage of total CPU time used by real-time interrupt tasks
 *
 * @param rtIntTaskUsage - The averaged fraction of total CPU time used by real-time interrupt tasks in percent will be written to this parameter
 * @return If value is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getRtIntTaskUsage(double& rtIntTaskUsage){
	static const char *functionName = "PowerPMACcontrol_getRtIntTime";
    debugPrint_ppmaccomm("%s called\n", functionName);

    TaskCalculator tasks(this);
    rtIntTaskUsage = tasks.getRtIntTaskUsage();

    return tasks.getErrorStatus();
}

/**
 * @brief Get the percentage of total CPU time used by background task scans
 *
 * @param bgTaskUsage - The averaged fraction of total CPU time used by background task scans in percent will be written to this parameter
 * @return If value is successfully received, PPMACcontrolNoError(0) is returned. If not,
 * minus value is returned. Possible error codes are :
 *      - PPMACcontrolNoError(0)
 *      - PPMACcontrolNoSSHDriverSet (-230)
 *      - PPMACcontrolSSHDriverError (-102)
 *      - PPMACcontrolSSHDriverErrorNoconn (-104)
 *      - PPMACcontrolSSHDriverErrorNobytes (-103)
 *      - PPMACcontrolSSHDriverErrorReadTimeout (-112)
 *      - PPMACcontrolSSHDriverErrorWriteTimeout (-113)
 *      - PPMACcontrolSemaphoreTimeoutError = (-239)
 *      - PPMACcontrolSemaphoreError = (-240)
 *      - PPMACcontrolSemaphoreReleaseError = (-241)
 */
int PowerPMACcontrol::PowerPMACcontrol_getBgTaskUsage(double& bgTaskUsage){
	static const char *functionName = "PowerPMACcontrol_getBgTime";
    debugPrint_ppmaccomm("%s called\n", functionName);

    TaskCalculator tasks(this);
    bgTaskUsage = tasks.getBgTaskUsage();

    return tasks.getErrorStatus();
}
/**
 * @brief Perform calculations for the CPU usage by different PMAC tasks.
 *
 * Calculations adapted from source code for Power PMAC IDE Task Manager, provided by Delta Tau
 *
 * @param parent_in Pointer to the calling PowerPMACControl class, needed to request the required information from the Power PMAC
 */
PowerPMACcontrol::TaskCalculator::TaskCalculator(PowerPMACcontrol * parent_in)
{
	myParent = parent_in;

	// Initialise members
	Sys_FltrPhaseTime = 0;
	Sys_FltrServoTime = 0;
	Sys_FltrRtIntTime = 0;
	Sys_FltrBgTime = 0;
	Sys_BgSleepTime = 0;
	Sys_PhaseDeltaTime = 0;
	Sys_ServoDeltaTime = 0;
	Sys_RtIntDeltaTime = 0;
	Sys_BgDeltaTime = 0;

	phaseTaskUsage = 0;
	servoTaskUsage = 0;
	rtIntTaskUsage = 0;
	bgTaskUsage = 0;
	CPUUsageByPmacTasks = 0;

	errorStatus = 0;

	// Build single request string
	std::string request_string = "Sys.FltrPhaseTime "
			"Sys.FltrServoTime "
			"Sys.FltrRtIntTime "
			"Sys.FltrBgTime "
			"Sys.BgSleepTime "
			"Sys.PhaseDeltaTime "
			"Sys.ServoDeltaTime "
			"Sys.RtIntDeltaTime "
			"Sys.BgDeltaTime\n";

	// Send command and read response
	std::string reply;
	errorStatus = myParent->writeRead(request_string.c_str(),reply);

	// Only proceed with calculations if no error
	if (errorStatus == PPMACcontrolNoError)
	{
		std::stringstream extract(reply);

		// Extract responses
		extract >> Sys_FltrPhaseTime;
		extract >> Sys_FltrServoTime;
		extract >> Sys_FltrRtIntTime;
		extract >> Sys_FltrBgTime;
		extract >> Sys_BgSleepTime;
		extract >> Sys_PhaseDeltaTime;
		extract >> Sys_ServoDeltaTime;
		extract >> Sys_RtIntDeltaTime;
		extract >> Sys_BgDeltaTime;

		// Phase task time
		phaseTaskTime_usec = Sys_FltrPhaseTime;

		// Servo task time
		servoTaskTime_usec = Sys_FltrServoTime
				- ( (int) (Sys_FltrServoTime / Sys_PhaseDeltaTime) + 1 ) * phaseTaskTime_usec;
		if (servoTaskTime_usec > 0.0)
		{
			last_positive_servoTaskTime = servoTaskTime_usec;
		}
		else
		{
			servoTaskTime_usec = last_positive_servoTaskTime;
		}

		// RT Interrupt Task Time
		rtIntTaskTime_usec = Sys_FltrRtIntTime
				- ( (int)(Sys_FltrRtIntTime / Sys_PhaseDeltaTime) + 1 ) * phaseTaskTime_usec
				- ( (int)(Sys_FltrRtIntTime / Sys_ServoDeltaTime) + 1 ) * servoTaskTime_usec;

		if (rtIntTaskTime_usec > 0.0)
		{
			last_positive_rtIntTaskTime = rtIntTaskTime_usec;
		}
		else
		{
			rtIntTaskTime_usec = last_positive_rtIntTaskTime;
		}

		// Background Task Time
		double temp_bgDeltaTime = Sys_FltrBgTime + Sys_FltrRtIntTime;
		bgTaskTime_usec = temp_bgDeltaTime;

		double difference_bg_rti = ((int)(temp_bgDeltaTime / Sys_RtIntDeltaTime) + 1)
				* rtIntTaskTime_usec;

		if ( bgTaskTime_usec > difference_bg_rti)
		{
			bgTaskTime_usec = bgTaskTime_usec - difference_bg_rti;
		}

		double difference_bg_servo
				= ((int)(temp_bgDeltaTime / Sys_ServoDeltaTime) + 1) * servoTaskTime_usec;

		if ( bgTaskTime_usec > difference_bg_servo)
		{
			bgTaskTime_usec = bgTaskTime_usec - difference_bg_servo;
		}

		double difference_bg_phase
				= ((int)(temp_bgDeltaTime / Sys_PhaseDeltaTime) + 1) * phaseTaskTime_usec;

		if (bgTaskTime_usec > difference_bg_phase)
		{
			bgTaskTime_usec = bgTaskTime_usec - difference_bg_phase;
		}

		// If Sys.BgSleepTime is set to 0, it means use a value of 1000 us.
		if (Sys_BgSleepTime == 0)
		{
			Sys_BgSleepTime = 1000;
		}
		double temp_overallTime_usec = Sys_FltrRtIntTime + Sys_FltrBgTime + Sys_BgSleepTime;

		// Phase task usage %
		phaseTaskUsage
			= ((int)(temp_overallTime_usec / Sys_PhaseDeltaTime) + 1)
			* phaseTaskTime_usec / temp_overallTime_usec * 100;

		// Servo task usage %
		servoTaskUsage
			= ((int)(temp_overallTime_usec / Sys_ServoDeltaTime) + 1)
			* servoTaskTime_usec / temp_overallTime_usec * 100;

		// RT interrupt task usage
		rtIntTaskUsage
			= ((int)(temp_overallTime_usec / Sys_RtIntDeltaTime) + 1)
			* rtIntTaskTime_usec / temp_overallTime_usec * 100;

		// Background task usage
		bgTaskUsage
			= bgTaskTime_usec / temp_overallTime_usec * 100;

		// Total CPU usage therefore
		CPUUsageByPmacTasks
			= phaseTaskUsage + servoTaskUsage + rtIntTaskUsage + bgTaskUsage;
	}
}

}
