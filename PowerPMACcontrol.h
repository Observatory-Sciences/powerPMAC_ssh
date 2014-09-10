/* 
 * File:   PowerPMACcontrol.h
 * Author: aya
 *
 * Created on 04 March 2013, 12:08
 */



/** 
 * @file PowerPMACcontrol.h
 * @brief Header file for the PowerPMACcontrol class 
 * 
 * This file contains declaration of the PowerPMACcontrol class, 
 * its functions and member variables. Integer values for error codes 
 * are defined in this file. The error codes are used for a return value 
 * in most of the class member functions.
 */



#ifndef POWERPMACCONTROL_H
#define	POWERPMACCONTROL_H

#include <string>
#include "libssh2Driver.h"
#include <vector>

/* Some versions of MS Visual Studio don't have stdint.h,
   so define uint32_t and uint64_t here */
#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif


#ifndef WIN32
#include <semaphore.h>
#endif


/* Windows DLL: Declaration for methods to be exported */ 
#ifdef DLL_CONFIG
#define DLLDECL __declspec(dllexport)
#else
#define DLLDECL
#endif

namespace PowerPMACcontrol_ns
{

/*
 * Remove trailing delimiters from the string and returns it.
 * param s - String to be trimmed.
 * param delimiters - List of delimiters to be removed from the string. 
 * If it is not specified, the default value is "\r\n" is used..
 * return Trimmed string
 */    
inline std::string trim_right_copy(
                const std::string& s, const std::string& delimiters = "\r\n")
{
    size_t t = s.find_last_not_of( delimiters );
    if (t != std::string::npos)
        return s.substr(0, t + 1);
    else
        return s.substr(0, 0);
}

#ifndef WIN32
inline struct timespec getAbsTimeout( long milliseconds)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
        
    //printf( "the diff is %ld, the second is %ld, the nanoseconds is %ld\n",milliseconds, ts.tv_sec, ts.tv_nsec);
    
    
    
    int more_seconds = 0;
    long ms = milliseconds;

    while (ms > 1000)
    {
        more_seconds += 1;
        ms -= 1000;
    }
    
    long ns = ts.tv_nsec + ms*1000000;
    if (ns > 1000000000)
    {
        more_seconds += 1;
        ns -= 1000000000;
    }
    
    ts.tv_nsec = ns;
    ts.tv_sec += more_seconds;
    
    return ts;
    
}


#endif

/**
 * The PowerPMACcontrol class manages the communication with the Power PMAC.
 * Setting up a connection can be configured with a host name/IP, 
 * username and optional password.
 *
 * @author Aya Yoshimura (aya@observatorysciences.co.uk)
 */
class PowerPMACcontrol {
public:
    DLLDECL PowerPMACcontrol();
    DLLDECL virtual ~PowerPMACcontrol();
    
   DLLDECL int PowerPMACcontrol_connect(const char *host, const char *user, const char *pwd, const char *port="22");
   DLLDECL int PowerPMACcontrol_disconnect();
   DLLDECL bool PowerPMACcontrol_isConnected();
   DLLDECL int PowerPMACcontrol_sendCommand(const std::string command, std::string& reply);


    //PowerPMAC Controller oriented functions
   DLLDECL int PowerPMACcontrol_getVers(std::string &vers);
   DLLDECL int PowerPMACcontrol_getGlobalStatus(uint32_t& status);
   DLLDECL int PowerPMACcontrol_getProgNames(int& num, std::vector<std::string>& names);
   DLLDECL int PowerPMACcontrol_getVariable(const std::string name, float& value);
   DLLDECL int PowerPMACcontrol_setVariable(const std::string name, float value);
   DLLDECL int PowerPMACcontrol_enablePlc(int plcnum);
   DLLDECL int PowerPMACcontrol_disablePlc(int plcnum);
   DLLDECL int PowerPMACcontrol_plcState(int plcnum, bool& active, bool& running);
   DLLDECL int PowerPMACcontrol_mprogState(int ncoord, bool& active, bool& running);
   DLLDECL int PowerPMACcontrol_runMprog(int ncoord);
   DLLDECL int PowerPMACcontrol_abortMprog(int ncoord);
   DLLDECL int PowerPMACcontrol_reset();
   DLLDECL int PowerPMACcontrol_stopAllAxes();
   DLLDECL int PowerPMACcontrol_progDownload(std::string filepath);
    
    //Axis oriented functions
   DLLDECL int PowerPMACcontrol_getMotorStatus(int motor, uint64_t& status);
   DLLDECL int PowerPMACcontrol_getMultiMotorStatus(int firstMotor, int lastMotor, std::vector<uint64_t>& status);
   DLLDECL int PowerPMACcontrol_getCoordStatus(int Cs, uint64_t& status);
   DLLDECL int PowerPMACcontrol_getMultiCoordStatus(int firstCs, int lastCs, std::vector<uint64_t>& status);
   DLLDECL int PowerPMACcontrol_motorPowered(int mnum, bool& powered);
   DLLDECL int PowerPMACcontrol_axisGetVelocity(int axis, double& velocity);
   DLLDECL int PowerPMACcontrol_axesGetVelocities(int firstAxis, int lastAxis, std::vector<double>& velocities);
   DLLDECL int PowerPMACcontrol_axisSetVelocity(int axis, double velocity);
   DLLDECL int PowerPMACcontrol_axisGetAcceleration(int axis, double& acceleration);
   DLLDECL int PowerPMACcontrol_axisSetAcceleration(int axis, double acceleration);
   DLLDECL int PowerPMACcontrol_axisGetSoftwareLimits(int axis, double& maxpos, double& minpos);
   DLLDECL int PowerPMACcontrol_axisSetSoftwareLimits(int axis, double maxpos, double minpos);    
   DLLDECL int PowerPMACcontrol_axisGetDeadband(int axis, double& deadband);
   DLLDECL int PowerPMACcontrol_axisSetDeadband(int axis, double deadband);
   DLLDECL int PowerPMACcontrol_axisMoveAbs(int axis, double position);
   DLLDECL int PowerPMACcontrol_axisMoveRel(int axis, double relposition);
   DLLDECL int PowerPMACcontrol_axisMovePositive(int axis);
   DLLDECL int PowerPMACcontrol_axisMoveNegative(int axis);
   DLLDECL int PowerPMACcontrol_axisGetCurrentPosition(int axis, double& position);
   DLLDECL int PowerPMACcontrol_axesGetCurrentPositions(int firstAxis, int lastAxis, std::vector<double>& positions);
   DLLDECL int PowerPMACcontrol_axisDefCurrentPos(int axis, double newpos);
   DLLDECL int PowerPMACcontrol_axisStop(int axis);
   DLLDECL int PowerPMACcontrol_axisAbort(int axis);
   DLLDECL int PowerPMACcontrol_axisHome(int axis);
    
    
    DLLDECL static const int  PPMACcontrolNoError = 0;                         ///< No error 
    //-1 to -99 are reserved for PMAC error
    DLLDECL static const int  PPMACcontrolError = -101;                         ///< PowerPMACcontrol Generic error 
    DLLDECL static const int  PPMACcontrolSSHDriverError = -102;                ///< SSHDriver error : SSH Generic error 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorNobytes = -103;         ///< SSHDriver error : SSH Zero bytes written 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorNoconn = -104;          ///< SSHDriver error : SSH Not connected 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorPassword = -105;        ///< SSHDriver error : SSH Authentication by password failed 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorPty = -106;             ///< SSHDriver error : SSH Failed requesting dumb pty 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorPublicKey = -107;       ///< SSHDriver error : SSH Authentication by public key failed 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorShell = -108;           ///< SSHDriver error : SSH Unable to request shell on allocated pty 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorSockfail = -109;        ///< SSHDriver error : SSH socket failed to connect 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorSshInit = -110;         ///< SSHDriver error : libssh2 initialization failed 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorSshSession = -111;      ///< SSHDriver error : libssh2 failed to create a session instance 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorReadTimeout = -112;     ///< SSHDriver error : SSH read timed out 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorWriteTimeout = -113;    ///< SSHDriver error : SSH write Timed out 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorUnknownHost = -114;     ///< SSHDriver error : Host unknown 
    DLLDECL static const int  PPMACcontrolSSHDriverErrorInvalidParameter = -115;     ///< SSHDriver error : Invalid Parameter
    DLLDECL static const int  PPMACcontrolNoSSHDriverSet = -230;                ///< SSH Driver hasn't been setup            
    DLLDECL static const int  PPMACcontrolPMACUnexpectedReplyError = -231;      ///< Unexpected reply from Power PMAC 
    DLLDECL static const int  PPMACcontrolSoftwareError = -232;                 ///< Unexpected software error, such as failed to allocate memory  
    DLLDECL static const int  PPMACcontrolOutOfOrderError = -233;               ///< Parameters to a function are in a wrong order
    DLLDECL static const int  PPMACcontrolFileOpenError = -234;                 ///< Error opening a file
    DLLDECL static const int  PPMACcontrolFileReadError = -235;                 ///< Error reading a file
    DLLDECL static const int  PPMACcontrolProgramCloseError = -236;             ///< Error sending 'close' command to Power PMAC
    DLLDECL static const int  PPMACcontrolSplitterError = -237;                 ///< Error while splitting a string
    DLLDECL static const int  PPMACcontrolUnexpectedParamError = -238;          ///< Unexpected parameter is found for a function
	DLLDECL static const int  PPMACcontrolSemaphoreTimeoutError = -239;         ///< Timeout error while waiting for a semaphore
	DLLDECL static const int  PPMACcontrolSemaphoreError = -240;				///< Error while waiting for a semaphore
	DLLDECL static const int  PPMACcontrolSemaphoreReleaseError = -241;			///< Error releasing a semaphore
    DLLDECL static const int  PPMACcontrolInvalidParamError = -242;			///< Invalid parameter
    DLLDECL static const int  PPMACcontrolInvalidHostNameError = -243;			///< Invalid host name
    DLLDECL static const int  PPMACcontrolInvalidUserNameError = -244;			///< Invalid user name
    DLLDECL static const int  PPMACcontrolInvalidPasswordError = -245;			///< Invalid password
    DLLDECL static const int  PPMACcontrolInvalidPortError = -246;			///< Invalid port number

private:
    SSHDriver *sshdriver;
    int connected;

    static int splitit(std::string s, std::string separator, std::vector<std::string> &strings);
    static int check_PowerPMAC_error(const std::string s);
    
    int PowerPMACcontrol_write(const char *buffer, size_t bufferSize, size_t *bytesWritten, int timeout);
    int PowerPMACcontrol_read(char *buffer, size_t bufferSize, size_t *bytesRead, int readTerm, int timeout);
    int writeRead(const char *cmd, int timeout = 1000);
    int writeRead(const char *cmd, std::string& response, int timeout = 1000);
	int writeRead_WithoutSemaphore(const char *cmd, std::string& response, int timeout = 1000);
    
    static const long SEMAPHORE_WAIT_MSEC = 200L;
    static const int MAX_ITEM_NUM = 32;
    
#ifdef WIN32
	HANDLE ghSemaphore;
#else
	sem_t sem_writeRead;
#endif
};

}
#endif	/* POWERPMACCONTROL_H */

