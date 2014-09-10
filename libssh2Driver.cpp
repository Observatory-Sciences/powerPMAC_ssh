/********************************************
 *  sshDriver.cpp
 * 
 *  SSH wrapper class for the libssh2 library
 *  This class provides standard read/write
 *  and flush methods for an ssh connection.
 * 
 *  Alan Greer
 *  21 Jan 2013
 * 
 ********************************************/

#include "libssh2Driver.h"
#include <string.h>

/*
 * Uncomment the DEBUG define and recompile for lots of
 * driver debug messages.
 */
/* #define DEBUG 1 */

#ifdef DEBUG
#define debugPrint printf
#else
void debugPrint(...){}
#endif

/**
 * Constructor for the SSH driver.  Accepts a host name or IP
 * address.  The class will attempt to resolve the name to an
 * IP address before connecting.  Initializes internal variables.
 *  struct timespec stime, ctime;
  double stimesecs, ctimesecs, time_at_timeout;
 * @param host - Host name/IP to attempt a connection with.
 */
SSHDriver::SSHDriver(const char *host)
{
  static const char *functionName = "SSHDriver::SSHDriver";
  debugPrint("%s : Method called\n", functionName);

  // Initialize internal SSH parameters
  auth_pw_ = 0;
  got_ = 0;
  connected_ = 0;
  // Username and password currently set to empty strings
  strncpy(username_, "", 256);
  strncpy(password_, "", 256);
  // Store the host address
  strncpy(host_, host, 256);
  
  //set the default port number 
  strncpy(port_, "22", 256);
  
  debugPrint("SSHDriver using libssh2 version: %s \n", libssh2_version(0));
}

/**
 * Setup the username for the connection.  Obviously the
 * username must exist on the device running the SSH
 * server.
 *
 * @param username - Username for the SSH connection.
 * @return - Success(SSHDriverSuccess) or failure(SSHDriverErrorInvalidParameter).
 */
SSHDriverStatus SSHDriver::setUsername(const char *username)
{
  static const char *functionName = "SSHDriver::setUsername";
  debugPrint("%s : Method called with user name %s\n", functionName, username);
  
  size_t ss = strlen(username);
  if (ss > 255)
      return SSHDriverErrorInvalidParameter;    //Too long user name

  // Store the username
  strncpy(username_, username, 256);    //256 is the length of char array

  return SSHDriverSuccess;
}

/**
 * Setup the password for the username on this connection.
 * A password does not need to be entered.  If it is not then
 * key based authorization will be attempted.
 *
 * @param password - Password for the SSH connection.
 * @return - Success(SSHDriverSuccess) or failure(SSHDriverErrorInvalidParameter).
 */
SSHDriverStatus SSHDriver::setPassword(const char *password)
{
  static const char *functionName = "SSHDriver::setPassword";
  debugPrint("%s : Method called with password %s\n", functionName, password);

  size_t ss = strlen(password);
  if (ss > 255)
      return SSHDriverErrorInvalidParameter;    //Too long password
  // Store the password
  strncpy(password_, password, 256);

  // Set the password authentication to on
  auth_pw_ = 1;

  return SSHDriverSuccess;
}

/**
 * Setup the port for the connection.
 * If it is not called, the default port of 22 will be used.
 *
 * @param port - Port to be used for the SSH connection.
 * @return - Success(SSHDriverSuccess) or failure(SSHDriverErrorInvalidParameter).
 */
SSHDriverStatus SSHDriver::setPort(const char *port)
{
  static const char *functionName = "SSHDriver::setPort";
  debugPrint("%s : Method called\n", functionName);

  size_t ss = strlen(port);
  if (ss > 255)
      return SSHDriverErrorInvalidParameter;    //Too long port
  // Store the password
  strncpy(port_, port, 256);

  return SSHDriverSuccess;
}

/**
 * Attempt to create a connection and authorize the username
 * with the password (or by keys).  Once the connection has
 * been established a dumb terminal is created and an attempt
 * to read the initial welcome lines is made.
 *
 * @return - Success or failure.
 */
SSHDriverStatus SSHDriver::connectSSH()
{
  int rc;
  int i, nerr;
  static const char *functionName = "SSHDriver::connect";
  struct addrinfo *res;
  
  debugPrint("%s : Method called\n", functionName);

#ifdef WIN32
  WSADATA wsadata;

  WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

   if ( (nerr = getaddrinfo(host_, port_, NULL, &res)) != 0)
   {
      debugPrint("%s : libssh2 unknown host %s port %s\n", functionName, host_, port_);
      return SSHDriverErrorUnknownHost;
   }

  debugPrint("%s : String host address (%s)\n", functionName, host_);

  rc = libssh2_init(0);
  if (rc != 0) {
    debugPrint("%s : libssh2 initialization failed, error code (%d)\n", functionName, rc);
    return SSHDriverErrorSshInit;
  }

  // Create the socket neccessary for the connection
  sock_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  if (connect(sock_, res->ai_addr, res->ai_addrlen) != 0){
    debugPrint("%s : socket failed to connect!\n", functionName);
    return SSHDriverErrorSockfail;
  }

  // Create a session instance
  session_ = libssh2_session_init();
  if(!session_){
    debugPrint("%s : libssh2 failed to create a session instance\n", functionName);
    return SSHDriverErrorSshSession;
  }

  // Start up the session. This will trade welcome banners, exchange keys,
  // and setup crypto, compression, and MAC layers
  rc = libssh2_session_handshake(session_, sock_);
  if(rc){
    debugPrint("%s : libssh2 failure establishing SSH session: %d\n", functionName, rc);
    return SSHDriverErrorSshSession;
  }

  // Here we now have a connection that will need to be closed
  connected_ = 1;

  // At this point the connection hasn't yet authenticated.  The first thing to do
  // is check the hostkey's fingerprint against the known hosts.
  const char *fingerprint = libssh2_hostkey_hash(session_, LIBSSH2_HOSTKEY_HASH_SHA1);
  debugPrint("%s : SSH fingerprint: ", functionName);
  for(i = 0; i < 20; i++) {
    debugPrint("%02X ", (unsigned char)fingerprint[i]);
  }
  debugPrint("\n");

  if (auth_pw_ == 1){
    // Authenticate via password
    if (libssh2_userauth_password(session_, username_, password_)) {
      debugPrint("%s : SSH authentication by password failed.\n", functionName);
      disconnectSSH();
      return SSHDriverErrorPassword;
    } else {
      debugPrint("%s : SSH authentication by password worked.\n", functionName);
    }
  } else {
    /* Or by public key */
    char rsapubbuff[256];
    char rsabuff[256];
    sprintf(rsapubbuff, "/home/%s/.ssh/id_rsa.pub", username_);
    sprintf(rsabuff, "/home/%s/.ssh/id_rsa", username_);
    if (libssh2_userauth_publickey_fromfile(session_, username_, rsapubbuff, rsabuff, password_)){
      debugPrint("%s : SSH authentication by public key failed\n", functionName);
      disconnectSSH();
      return SSHDriverErrorPublicKey;
    }
  }

  // Open the channel for read/write
  channel_ = libssh2_channel_open_session(session_);
  debugPrint("%s : SSH channel opened\n", functionName);

  // Request a terminal with 'dumb' terminal emulation
  // See /etc/termcap for more options
  if (libssh2_channel_request_pty(channel_, "dumb")){
    debugPrint("%s : Failed requesting dumb pty\n", functionName);
    disconnectSSH();
    return SSHDriverErrorPty;
  }

  // Open a SHELL on that pty
  if (libssh2_channel_shell(channel_)) {
    debugPrint("%s : Unable to request shell on allocated pty\n", functionName);
    disconnectSSH();
    return SSHDriverErrorShell;
  }

  setBlocking(0);

  // Here we should wait for the initial welcome line
  char buffer[1024];
  size_t bytes = 0;
  read(buffer, 1024, &bytes, '\n', 1000);
  // And the command line
  read(buffer, 1024, &bytes, 0x20, 1000);
  debugPrint("%s : Connection ready...\n", functionName);

  return SSHDriverSuccess;
}

/**
 * Set the connection to a blocking or non-blocking connection.
 *
 * @param blocking - 0 for non-blocking or 1 for blocking.
 * @return - Success or failure.
 */
SSHDriverStatus SSHDriver::setBlocking(int blocking)
{
  static const char *functionName = "SSHDriver::setBlocking";
  debugPrint("%s : Method called\n", functionName);

  // Make the channel blocking or non-blocking
  libssh2_channel_set_blocking(channel_, blocking);
  debugPrint("%s : Set blocking value to %d\n", functionName, blocking);
  return SSHDriverSuccess;
}

/**
 * Flush the connection as best as possible.
 *
 * @return - Success or failure.
 */
SSHDriverStatus SSHDriver::flush()
{
  char buff[2048];
  static const char *functionName = "SSHDriver::flush";
  debugPrint("%s : Method called\n", functionName);

  if (connected_ == 0){
    debugPrint("%s : Not connected\n", functionName);
    return SSHDriverErrorNoconn;
  }

  ssize_t rc = libssh2_channel_flush_ex(channel_, 0);
  rc |= libssh2_channel_flush_ex(channel_, 1);
  rc |= libssh2_channel_flush_ex(channel_, 2);
  rc = libssh2_channel_read(channel_, buff, 2048);
  if (rc < 0){
    return SSHDriverErrorNoconn;
  }
  return SSHDriverSuccess;
}

/**
 * Write data to the connected channel.  A timeout should be
 * specified in milliseconds.
 *
 * @param buffer - The string buffer to be written.
 * @param bufferSize - The number of bytes to write.
 * @param bytesWritten - The number of bytes that were written.
 * @param timeout - A timeout in ms for the write.
 * @return - Success(SSHDriverSuccess) if it succeeds to write. The possible error numbers are
 *      - SSHDriverErrorNoconn if there is no connection. 
 *      - SSHDriverErrorInvalidParameter if bufferSize is too large.
 *      - SSHDriverErrorNobytes if no bytes were written.
 *      - SSHDriverErrorWriteTimeout if timeout occurs
 */
SSHDriverStatus SSHDriver::write(const char *buffer, size_t bufferSize, size_t *bytesWritten, int timeout)
{
  static const int CHAR_SIZE = 5120;
  char input[CHAR_SIZE];
  static const char *functionName = "SSHDriver::write";
  double stimesecs, ctimesecs, time_at_timeout;
  
  debugPrint("%s : Method called\n", functionName);
  *bytesWritten = 0;

  if (connected_ == 0){
    debugPrint("%s : Not connected\n", functionName);
    return SSHDriverErrorNoconn;
  }

  if (bufferSize > (CHAR_SIZE-1))
  {
      debugPrint("%s : Buffer size too large\n", functionName);
      return SSHDriverErrorInvalidParameter;
  }
  
  strncpy(input, buffer, bufferSize);
  input[bufferSize] = 0;
  flush();
  debugPrint("%s : Writing => %s\n", functionName, input);

  stimesecs = SSHDriverCurrentTimeSecs ();
  time_at_timeout = stimesecs + timeout/1000.0;
  ssize_t rc = libssh2_channel_write(channel_, buffer, bufferSize);
  if (rc > 0){
    debugPrint("%s : %d bytes written\n", functionName, rc);
    *bytesWritten = rc;
  } else {
    debugPrint("%s : No bytes were written, libssh2 error (%d)\n", functionName, rc);
    bytesWritten = 0;
    return SSHDriverErrorNobytes;
  }

  // Now we need to read back the same number of bytes, to remove the written string from the buffer
  int bytesToRead = *bytesWritten;
  int bytes = 0;
  char buff[CHAR_SIZE];
  rc = 0;
  int crCount = 0;
    
  // Count the number of \n characters sent
  for (int index = 0; index < (int)*bytesWritten; index++){
    if (buffer[index] == '\n'){
      // CR, need to read back 1 extra character
      crCount++;
    }
  }
  ctimesecs = 0.0;
  bytesToRead += crCount;
  while ((bytesToRead > 0) && (ctimesecs < time_at_timeout)){
    rc = libssh2_channel_read(channel_, &buff[bytes], bytesToRead);
    if (rc > 0){
      bytes+=rc;
      bytesToRead-=rc;
    }
    if (bytesToRead > 0){
      ctimesecs = SSHDriverCurrentTimeSecs ();
    }
  }

  buff[bytes] = '\0';
  debugPrint("%s : Bytes =>\n", functionName);
  for (int j = 0; j < bytes; j++){
    debugPrint("[%d] ", buff[j]);
  }
  debugPrint("\n");

  ctimesecs = SSHDriverCurrentTimeSecs ();
  debugPrint("%s : Time taken for write => %ld ms\n", functionName, (long)((ctimesecs - stimesecs) * 1000) );
  if (ctimesecs >= time_at_timeout){
    return SSHDriverErrorWriteTimeout;
  }

  return SSHDriverSuccess;
}

/**
 * Read data from the connected channel.  A timeout should be
 * specified in milliseconds.  The read method will continue to
 * read data from the channel until either the specified 
 * terminator is read or the timeout is reached.
 *
 * @param buffer - A string buffer to hold the read data.
 * @param bufferSize - The maximum number of bytes to read.
 * @param bytesWritten - The number of bytes that have been read.
 * @param readTerm - A terminator to use as a check for EOM (End Of Message).
 * @param timeout - A timeout in ms for the read.
 * @return - Success or failure.
 */
SSHDriverStatus SSHDriver::read(char *buffer, size_t bufferSize, size_t *bytesRead, int readTerm, int timeout)
{
  static const char *functionName = "SSHDriver::read";
  double stimesecs, ctimesecs, time_at_timeout;  
  
  debugPrint("%s : Method called\n", functionName);
  debugPrint("%s : Read terminator %d\n", functionName, readTerm);

  if (connected_ == 0){
    debugPrint("%s : Not connected\n", functionName);
    return SSHDriverErrorNoconn;
  }

  ssize_t rc = 0;
  int matched = 0;
  int matchedindex = 0;
  int lastCount = 0;
  *bytesRead = 0;
  ctimesecs = 0.0;
  
  stimesecs = SSHDriverCurrentTimeSecs ();
  time_at_timeout = stimesecs + timeout/1000.0;
  while ((matched == 0) && (ctimesecs < time_at_timeout)){
    rc = libssh2_channel_read(channel_, &buffer[*bytesRead], (bufferSize-*bytesRead));
    if (rc > 0){
      *bytesRead+=rc;
    }
    for (int index = lastCount; index < (int)*bytesRead; index++){
      // Match against output terminator
      if (buffer[index] == readTerm){
        matched = 1;
        matchedindex = index;
      }
    }
    lastCount = *bytesRead;
    if (matched == 0){
      ctimesecs = SSHDriverCurrentTimeSecs ();
    }
  }

  buffer[matchedindex] = '\0';
  debugPrint("%s : Bytes =>\n", functionName);
  for (int j = 0; j < lastCount; j++){
    debugPrint("[%d] ", buffer[j]);
  }
  debugPrint("\n");
  debugPrint("%s : Matched %d\n", functionName, matched);

  *bytesRead = lastCount;
  debugPrint("%s : Line => %s", functionName, buffer);

  ctimesecs = SSHDriverCurrentTimeSecs ();
  debugPrint("%s : Time taken for read => %ld ms\n", functionName,  (long)((ctimesecs - stimesecs) * 1000) );
  if (ctimesecs >= time_at_timeout){
    return SSHDriverErrorReadTimeout;
  }

  return SSHDriverSuccess;
}

/**
 * Close the connection.
 *
 * @return - Success or failure.
 */
SSHDriverStatus SSHDriver::disconnectSSH()
{
  static const char *functionName = "SSHDriver::disconnect";
  debugPrint("%s : Method called\n", functionName);

  if (connected_ == 1){
    connected_ = 0;
    libssh2_session_disconnect(session_, "Normal Shutdown");
    libssh2_session_free(session_);

#ifdef WIN32
    closesocket(sock_);
#else
    close(sock_);
#endif
    debugPrint("%s : Completed disconnect\n", functionName);

    libssh2_exit();
  
  } else {
    debugPrint("%s : Connection was never established\n", functionName);
  }
  return SSHDriverSuccess;
}

/**
 * Destructor, cleanup.
 */
SSHDriver::~SSHDriver()
{
  static const char *functionName = "SSHDriver::~SSHDriver";
  debugPrint("%s : Method called\n", functionName);
}

double SSHDriver::SSHDriverCurrentTimeSecs ()
{
#ifdef WIN32
  static BOOL initialized = FALSE;
  LARGE_INTEGER timeNow;
  static LARGE_INTEGER proc_freq;

  if (! initialized)
  {
    if (QueryPerformanceFrequency(&proc_freq)!=TRUE) printf("QueryPerformanceFrequency() failed\n");
    initialized = TRUE;
  }
  QueryPerformanceCounter(&timeNow);
  return((double) (((double)timeNow.QuadPart)/((double)proc_freq.QuadPart)));
#else
  struct timespec timeNow;
  clock_gettime(CLOCK_MONOTONIC, &timeNow);
  return((double)(timeNow.tv_sec + timeNow.tv_nsec/1E9));
#endif
}


