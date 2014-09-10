/* Might need to define WIN32 for MinGW */
#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#ifndef sshDriver_H
#define sshDriver_H

#include <libssh2.h>

#ifdef WIN32
# include <winsock2.h>
# include <ws2tcpip.h> 
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <time.h>
# include <unistd.h>
# include <arpa/inet.h>
#endif

#include <stdio.h>

typedef enum e_SSHDriverStatus
{
  SSHDriverSuccess,
  SSHDriverError,           /* SSH Generic error */
  SSHDriverErrorNobytes,    /* SSH Zero bytes written */
  SSHDriverErrorNoconn,     /* SSH Not connected */
  SSHDriverErrorPassword,   /* SSH Authentication by password failed */
  SSHDriverErrorPty,        /* SSH Failed requesting dumb pty */
  SSHDriverErrorPublicKey,  /* SSH Authentication by public key failed */
  SSHDriverErrorShell,      /* SSH Unable to request shell on allocated pty\ */
  SSHDriverErrorSockfail,   /* SSH socket failed to connect */
  SSHDriverErrorSshInit,    /* libssh2 initialization failed */
  SSHDriverErrorSshSession, /* libssh2 failed to create a session instance */
  SSHDriverErrorReadTimeout,  /* SSH read timed out */
  SSHDriverErrorWriteTimeout, /* SSH write Timed out */
  SSHDriverErrorUnknownHost,   /* Host unknown */
  SSHDriverErrorInvalidParameter   /* Parameter Invalid */
} SSHDriverStatus;

/**
 * The SSHDriver class provides a wrapper around the libssh2 library.
 * It takes out some of the complexity of creating SSH connections and
 * provides a simple read/write/flush interface.  Setting up a connection
 * can be configured with a host name/IP, username and optional password.
 *
 * @author Alan Greer (ajg@observatorysciences.co.uk)
 */
class SSHDriver {

  public:
    SSHDriver(const char *host);
    SSHDriverStatus setUsername(const char *username);
    SSHDriverStatus setPassword(const char *password);
    SSHDriverStatus setPort(const char *port);
    SSHDriverStatus connectSSH();
    SSHDriverStatus flush();
    SSHDriverStatus write(const char *buffer, size_t bufferSize, size_t *bytesWritten, int timeout);
    SSHDriverStatus read(char *buffer, size_t bufferSize, size_t *bytesRead, int readTerm, int timeout);
    SSHDriverStatus disconnectSSH();
    virtual ~SSHDriver();

  private:
    int sock_;
    int auth_pw_;
    int connected_;
    struct sockaddr_in sin_;
    LIBSSH2_SESSION *session_;
    LIBSSH2_CHANNEL *channel_;
    char host_[256];
    char username_[256];
    char password_[256];
    char port_[256];
    off_t got_;

    SSHDriverStatus setBlocking(int blocking);
    double SSHDriverCurrentTimeSecs ();

};


#endif


