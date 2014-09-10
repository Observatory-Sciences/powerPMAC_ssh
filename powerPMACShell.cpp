#include "libssh2Driver.h"
#include "PowerPMACcontrol.h"
#include <iostream>

#define IPADDR "192.168.0.48"
#define USER   "root"
#define PASSW  "deltatau"

using namespace PowerPMACcontrol_ns;
int main(int argc, char *argv[])
{
    PowerPMACcontrol *ppmaccomm = new PowerPMACcontrol();
    int ret = ppmaccomm->PowerPMACcontrol_connect(IPADDR, USER , PASSW);
    
    if (ret != 0)
    {
      printf("Error connecting to power pmac  at %s. exit:\n", IPADDR);
      return 0;
    }
    else
      printf("Connected to Power PMAC OK at %s\n", IPADDR);
    
    
#ifdef WIN32
    Sleep(1000);
#else
     sleep(1);
#endif
     
  int c;
  int counter = 0;
  char cmd[1024];

  counter = 0;
  printf("   -- Power PMAC Shell --      \n    Type \"quit\" to end\n");
  do {
    counter = 0;
    printf("ppmac> ");
    do {
      c = getchar();
      cmd[counter] = c;
      counter++;
    } while(c != '\n');
    // Append the terminator
    cmd[counter] = '\0';
    if (strcmp(cmd, "quit\n")){
      std::string command(cmd);
      std::string reply = "";
      int ret = ppmaccomm->PowerPMACcontrol_sendCommand(command, reply);
 	if (ret == PowerPMACcontrol::PPMACcontrolNoError)
      {
        printf("Reply from Power PMAC : [%s]\n", reply.c_str());
      }
      else
      {
          printf("Error from Power PMAC read/write: error number %d\n", ret);
      }
    }
    // Quit if we get a quit
  } while(strcmp(cmd, "quit\n"));
  
  delete ppmaccomm;
  
  return 0;

}
