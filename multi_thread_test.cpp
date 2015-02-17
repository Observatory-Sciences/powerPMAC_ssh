#include "libssh2Driver.h"
#include "PowerPMACcontrol.h"
#include <iostream>
#include <pthread.h>

#define IPADDR "192.168.0.48"
#define USER   "root"
#define PASSW  "deltatau"

using namespace PowerPMACcontrol_ns;

void *getVersion(void *arg);
void *getCurrentPosition(void *arg);
void *getBuffer(void *arg);
void *downloadPrograms(void *arg);

PowerPMACcontrol *ppmaccomm;
int main(int argc, char *argv[])
{


    ppmaccomm = new PowerPMACcontrol();
    
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
     
     pthread_t thread1, thread2, thread3, thread4;
     pthread_create(&thread4, NULL, &downloadPrograms, NULL);
     sleep(0.2);
     pthread_create(&thread1, NULL, &getVersion, NULL);
     sleep(0.2);
     pthread_create(&thread2, NULL, &getCurrentPosition, NULL);
     sleep(0.2);
     pthread_create(&thread3, NULL, &getBuffer, NULL);

     
     
     pthread_join(thread1, NULL);
     pthread_join(thread2, NULL);
     pthread_join(thread3, NULL);
     pthread_join(thread4, NULL);
     
     return 0;
     
  /*int c;
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
  //printf("after delete\n");
  return 0;*/

}

void *getVersion(void *arg)
{
    std::string s;
    int ret = 0;
    for(int i=0; i<100; i++)
    {
        ret = ppmaccomm->PowerPMACcontrol_getVers(s);
        if (ret != 0)
        {
            printf("getVersion error %d\n", ret);
        }
        else
            printf("                                       getVersion OK\n");
    }
    return 0;
}
void *getCurrentPosition(void *arg)
{
    double d;
    int ret = 0;
    for(int i=0; i<100; i++)
    {
        ret = ppmaccomm->PowerPMACcontrol_axisGetCurrentPosition(1,d);
        if (ret != 0)
        {
            printf("getCurrentPosition error %d\n", ret);
        }
        else
            printf("                                       getCurrentPosition OK\n");
    }
    return 0;
}
void *getBuffer(void *arg)
{
    std::vector<std::string> sv;
    int ii;
    int ret = 0;
    for(int i=0; i<100; i++)
    {
        ret = ppmaccomm->PowerPMACcontrol_getProgNames(ii, sv);
        if (ret != 0)
        {
            printf("getBuffer error %d\n", ret);
        }
        else
            printf("                                       getBuffer OK\n");
    }
    return 0;
}

void *downloadPrograms(void *arg)
{
    std::string s = "plc7.plc";
    int ret = ppmaccomm->PowerPMACcontrol_progDownload(s);
    if (ret != 0)
    {
        printf("download plc7.plc error %d\n", ret);
    }
    else
        printf("                                       download plc7.plc  OK\n");
    
    s = "prog2.pmc";
    ret = ppmaccomm->PowerPMACcontrol_progDownload(s);
    if (ret != 0)
    {
        printf("download prog2.pmc  error %d\n", ret);
    }
    else
        printf("                                       download prog2.pmc  OK\n");
    
    s = "prog1.pmc";
    ret = ppmaccomm->PowerPMACcontrol_progDownload(s);
    if (ret != 0)
    {
        printf("download prog1.pmc  error %d\n", ret);
    }
    else
        printf("                                       download prog1.pmc  OK\n");

    return 0;
}
