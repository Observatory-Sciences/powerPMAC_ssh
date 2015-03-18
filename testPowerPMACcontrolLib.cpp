#include <iostream>
#include <string>
using namespace std;
#include "PowerPMACcontrol.h"
#include "argParser.h"

void testController (const char * u_ipaddr, const char * u_user, const char * u_passw, const char * u_port, bool u_nominus2);
void testAxis(const char * u_ipaddr, const char * u_user, const char * u_passw, const char * u_port, bool u_nominus2);
void printStringVec(const vector<std::string> & strvec);
void checkPMACerror(int estatus);
string StringToUpper(string strToConvert);
void printStringVec(const vector<std::string> & strvec);
void printUint64Vec(const vector<uint64_t> & ivec);
void printDoubleVec(const vector<double> & dvec);

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

    int i = 0; 
  
  while (i < 1 || i > 2)
  {
    cout << "Enter 1 for Controller functions, 2 for Axis functions" << endl;
    cin >> i;
  }
  if (i ==1) testController(u_ipaddr.c_str(), u_user.c_str(), u_passw.c_str(), u_port.c_str(), u_nominus2);
    else testAxis(u_ipaddr.c_str(), u_user.c_str(), u_passw.c_str(), u_port.c_str(), u_nominus2);

  
  return 0;
}

void testController (const char * u_ipaddr, const char * u_user, const char * u_passw, const char * u_port, bool u_nominus2)
  {
  int i, estatus;
  std::string reply, prompt;
  
  i = -1;
  PowerPMACcontrol *ppmaccomm = new PowerPMACcontrol();
  estatus = ppmaccomm->PowerPMACcontrol_connect( u_ipaddr, u_user , u_passw, u_port, u_nominus2);
  if (estatus != 0)
  {
    printf("Error connecting to power pmac. exit:\n");
    return;
  }
  #ifdef WIN32
    Sleep(1000);
  #else
    sleep(1);
  #endif 
  
  while (i !=0)
  {
      printf( "\033[2J" );
      cout << "*** Select from the list below ***" << endl << endl << endl;
      cout << "1.  getVers(std::string &vers)"<< endl;
      cout << "2.  getGlobalStatus(uint32_t& status)"<< endl;
      cout << "3.  getVariable(const std::string name, float& value)"<< endl;
      cout << "4.  setVariable(const std::string name, float value)"<< endl;
      cout << "5.  reset()"<< endl;
      cout << "6.  stopAllAxes()"<< endl;
      cout << "" << endl;
      cout << "7.  getProgNames(int& num, std::vector<std::string>& names)"<< endl;
      cout << "8.  progDownload(std::string filepath)"<< endl;
      cout << "9.  plcState(int plcnum, bool& active, bool& running)"<< endl;
      cout << "10. enablePlc(int plcnum)"<< endl;
      cout << "11. disablePlc(int plcnum)"<< endl;
      cout << "12. mprogState(int ncoord, bool& active, bool& running)"<< endl;
      cout << "13. runMprog(int ncoord)"<< endl;
      cout << "14. abortMprog(int ncoord)"<< endl;
      cout << "15. sendCommand(const std::string command, std::string& reply)"<< endl;
      cout << "" << endl;
      cout << "16. getPhaseTaskUsage(double&)" << endl;
      cout << "17. getServoTaskUsage(double&)" << endl;
      cout << "18. getRtIntTaskUsage(double&)" << endl;
      cout << "19. getBgTaskUsage(double&)" << endl;
      cout << "20. getCPUUsage(double&)" << endl;
      cout << "21. getCPUTemperature(double&)" << endl;
      cout << "22. getRunningTime(double&)" << endl;
      cout << "23. isConnected()" << endl;
      cout << "Please enter your selection (0 to exit) : ";
      
      string str;
      cin >> str;
      i = -1;
      sscanf(str.c_str(), "%d", &i);

      if (i == 0) break;

      switch(i)
      {
         case 1:
               estatus = ppmaccomm->PowerPMACcontrol_getVers(reply);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                   cout << "Power PMAC firmware Version is: "<< reply << endl;               
               break;
         case 2:
               {
               uint32_t gstatus;
               estatus = ppmaccomm->PowerPMACcontrol_getGlobalStatus(gstatus);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                   cout << "Global Status is: "<< gstatus << endl;
               break;
               }
         case 3:
               {
			   float varvalfloat;
				  double varvaldouble;
				  int varvalint;
				  unsigned int varvaluint;
				  std::string varvalstring;
				  std::string varnam;

				  cout << "Input variable name: ";
				  cin >> varnam;
				  cout << endl;
				  estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvalfloat);
				  if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
					  checkPMACerror(estatus);
				  else
					  cout << "The (float) value of " << varnam << " is: " << varvalfloat << endl;

				  estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvaldouble);
				  if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
				   checkPMACerror(estatus);
				  else
				   cout << "The (double) value of " << varnam << " is: " << varvaldouble << endl;

				  estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvalint);
			   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
				   checkPMACerror(estatus);
			   else
				   cout << "The (int) value of " << varnam << " is: " << varvalint << endl;

			   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvaluint);
			   			   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
			   				   checkPMACerror(estatus);
			   			   else
			   				   cout << "The (uint) value of " << varnam << " is: " << varvalint << endl;

			   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvalstring);
			   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
				   checkPMACerror(estatus);
			   else
				   cout << "The (string) value of " << varnam << " is: " << varvalstring << endl;
			   break;
               }
         case 4:
               {
               float varvalfloat;
               double varvaldouble;
               int varvalint;
               unsigned int varvaluint;
               std::string varvalstring;
               std::string varnam;

               cout << "Input variable name: ";
               cin >> varnam;
               cout << endl;
               enum {FLOAT=0, DOUBLE=1, INT=2, UINT=3, STRING=4};
               int w = -1;
               while (w <0 || w >4)
               {
            	   cout << "Choose data type to write: float=" << FLOAT << "; double=" << DOUBLE << "; int=" << INT << "; uint=" << UINT << "; string=" << STRING << endl;
            	   cin >> w;
               }
               cout << "Input new value: ";
               switch (w){
               case FLOAT:
            	   cin >> varvalfloat;
                   cout << endl;
                   estatus = ppmaccomm->PowerPMACcontrol_setVariable(varnam, varvalfloat);
                   checkPMACerror(estatus);
                   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvalfloat);
                   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
					  checkPMACerror(estatus);
				  else
					  cout << "The value of " << varnam << " is now: " << varvalfloat << endl;
            	   break;
               case DOUBLE:
            	   cin >> varvaldouble;
                   cout << endl;
                   estatus = ppmaccomm->PowerPMACcontrol_setVariable(varnam, varvaldouble);
                   checkPMACerror(estatus);
                   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvaldouble);
                   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
					  checkPMACerror(estatus);
				  else
					  cout << "The value of " << varnam << " is now: " << varvaldouble << endl;
            	   break;
               case INT:
            	   cin >> varvalint;
                   cout << endl;
                   estatus = ppmaccomm->PowerPMACcontrol_setVariable(varnam, varvalint);
                   checkPMACerror(estatus);
                   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvalint);
                   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
					  checkPMACerror(estatus);
				  else
					  cout << "The value of " << varnam << " is now: " << varvalint << endl;
            	   break;
               case UINT:
            	   cin >> varvaluint;
                   cout << endl;
                   estatus = ppmaccomm->PowerPMACcontrol_setVariable(varnam, varvaluint);
                   checkPMACerror(estatus);
                   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvaluint);
                   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
					  checkPMACerror(estatus);
				  else
					  cout << "The value of " << varnam << " is now: " << varvaluint << endl;
            	   break;
               case STRING:
            	   cin >> varvalstring;
                   cout << endl;
                   estatus = ppmaccomm->PowerPMACcontrol_setVariable(varnam, varvalstring);
                   checkPMACerror(estatus);
                   estatus = ppmaccomm->PowerPMACcontrol_getVariable(varnam, varvalstring);
                   if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
					  checkPMACerror(estatus);
				  else
					  cout << "The value of " << varnam << " is now: " << varvalstring << endl;
            	   break;
               }
               cout << endl;
               break;
               }
         case 5:
               {
               char reply = ' ';
               cout << "Are you sure you want to reset the Power PMAC (current settings will be lost)?" << endl;
               while (toupper(reply) != 'Y' && toupper(reply) != 'N')
                 {
                 cin >> reply;
                 if (toupper(reply) != 'Y' && toupper(reply) != 'N')
                     cout << "Answer Y or N" <<endl;
                 }
               if (toupper(reply) == 'Y') ppmaccomm->PowerPMACcontrol_reset();
               break;
               }
         case 6:
               cout << "Stopping all axes" << endl;
               ppmaccomm->PowerPMACcontrol_stopAllAxes();
               break;
         case 7:
               {
               int num;
               std::vector<std::string> prognames;
               estatus = ppmaccomm->PowerPMACcontrol_getProgNames(num, prognames);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
                 checkPMACerror(estatus);
               else
                 {
                 if (num > 0)
                   {
                   cout << num <<" programs found, as follows:" << endl;              
                   printStringVec(prognames);
                   }
                 else
                   cout << "No programs found" << endl;              
                 }
               break;
               }
         case 8:
               {
                 std::string fpath;     
                 cout << "Input program file path: ";
                 cin >> fpath;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_progDownload(fpath);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolFileOpenError) 
                     cout << "Error: failed to open file " << fpath << endl;
                 break;
               }
          case 9:
               {
                 int plcnum;
                 bool active, running;
                 std::string acstr, runstr;
                 cout << "Input PLC program number: ";
                 cin >> plcnum;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_plcState(plcnum, active, running);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                 {
                     if (active)
                         acstr = "Active";
                     else
                         acstr = "Inactive";
                     if (running)
                         runstr = "Running";
                     else
                         runstr = "Not running";
                     cout << "PLC program " << plcnum << " is " << acstr << " and " << runstr << endl;
                 }
                 break;
               }
          case 10:
               {
                 int plcnum;
                 cout << "Input PLC program number: ";
                 cin >> plcnum;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_enablePlc(plcnum);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                     cout << "PLC program " << plcnum << " is now enabled" << endl;
                 break;
               }
          case 11:
               {
                 int plcnum;
                 cout << "Input PLC program number: ";
                 cin >> plcnum;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_disablePlc(plcnum);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                     cout << "PLC program " << plcnum << " is now disabled" << endl;
                 break;
               }
          case 12:
               {
                 int mpnum;
                 bool active, running;
                 std::string acstr, runstr;
                 cout << "Input Motion program number: ";
                 cin >> mpnum;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_mprogState(mpnum, active, running);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                 {
                     if (active)
                         acstr = "Active";
                     else
                         acstr = "Inactive";
                     if (running)
                         runstr = "Running";
                     else
                         runstr = "Not running";
                     cout << "Motion program " << mpnum << " is " << acstr << " and " << runstr << endl;
                 }
                 break;
               }
          case 13:
               {
                 int mpnum;
                 cout << "input Motion program number: ";
                 cin >> mpnum;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_runMprog(mpnum);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                     cout << "Motion program " << mpnum << " is now running" << endl;
                 break;
               }
          case 14:
               {
                 int mpnum;
                 cout << "input Motion program number: ";
                 cin >> mpnum;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_abortMprog(mpnum);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                     cout << "Motion program " << mpnum << " has been aborted" << endl;
                 break;
               }
          case 15:
               {
                 std::string command;;
                 cout << "input Power PMAC command string: ";
                 cin >> command;
                 cout << endl;
                 estatus = ppmaccomm->PowerPMACcontrol_sendCommand(command, reply);
                 checkPMACerror(estatus);
                 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                     {
                     cout << "Command " << command << """ was sent OK" << endl;
                     if (reply.length() > 0)
                       cout << "Reply received was: " << reply << endl;
                     else
                       cout << "No reply received" << endl;
                     }
                 break;
               }
          	  case 16: /* getPhaseTaskUsage */
          	  {
          		  double d;
          		estatus = ppmaccomm->PowerPMACcontrol_getPhaseTaskUsage(d);
          		                 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "Phase task usage: " << d << "%" << endl;
					 }
          		  break;
          	  }
          	case 17: /* getServoTaskUsage */
			  {
				  double d;
				estatus = ppmaccomm->PowerPMACcontrol_getServoTaskUsage(d);
								 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "Servo task usage: " << d << "%" << endl;
					 }
				  break;
			  }
          	case 18:
			  {
				  double d;
				estatus = ppmaccomm->PowerPMACcontrol_getRtIntTaskUsage(d);
								 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "Real-time interrupt task usage: " << d << "%" << endl;
					 }
				  break;
			  }
          	case 19: /* getBgTaskUsage */
			  {
				  double d;
				estatus = ppmaccomm->PowerPMACcontrol_getBgTaskUsage(d);
								 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "Background task usage: " << d << "%" << endl;
					 }
				  break;
			  }
          	case 20: /* getCPUUsage */
			  {
				  double d;
				estatus = ppmaccomm->PowerPMACcontrol_getCPUUsage(d);
								 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "Total CPU usage by PMAC tasks: " << d << "%" << endl;
					 }
				  break;
			  }
          	case 21: /* getCPUTemperature */
			  {
				  double d;
				estatus = ppmaccomm->PowerPMACcontrol_getCPUTemperature(d);
								 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "CPU temperature: " << d << " degrees C" << endl;
					 }
				  break;
			  }
          	case 22: /* getRunningTime */
			  {
				  double d;
				estatus = ppmaccomm->PowerPMACcontrol_getRunningTime(d);
								 checkPMACerror(estatus);
				 if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
					 {
					 cout << "Running time: " << d << " seconds" << endl;
					 }
				  break;
			  }
          	case 23:
          	{
          		bool status = ppmaccomm->PowerPMACcontrol_isConnected();
          		if (status == true)
          			cout << "Connection OK" << endl;
          		else
          			cout << "Connection problem" << endl;
          		break;
          	}
             default:
             	cout << "Input value '" << str << "' was not recognised as a test case." << endl;
             break;
             } /* end switch */
      cout << endl;
      cout << "Press ENTER only to continue>";
      string s;
      getline( cin, s );
      while (getline( cin, s ) && !s.empty())
      cout << "Press only the ENTER key>";
      }
    ppmaccomm->PowerPMACcontrol_disconnect();
    delete ppmaccomm;
    return;
}  /* end testController() */
  
void testAxis(const char * u_ipaddr, const char * u_user, const char * u_passw, const char * u_port, bool u_nominus2)
{
  int i, estatus;
  
  i = -1;
  PowerPMACcontrol *ppmaccomm = new PowerPMACcontrol();
  estatus = ppmaccomm->PowerPMACcontrol_connect( u_ipaddr, u_user , u_passw, u_port, u_nominus2);
  if (estatus != 0)
  {
    printf("Error connecting to power pmac. exit:\n");
    return;
  }
  #ifdef WIN32
    Sleep(1000);
  #else
    sleep(1);
  #endif 
  
  while (i != 0)
  {
      printf( "\033[2J" );
      cout << "*** Select from the list below ***" << endl << endl << endl;
      cout << "1.  getMotorStatus(int motor, uint64_t& status)"<< endl;
      cout << "2.  getMultiMotorStatus(int firstMotor, int lastMotor, std::vector<uint64_t>& status)"<< endl;
      cout << "3.  getCoordStatus(int Cs, uint64_t& status)"<< endl;
      cout << "4.  getMultiCoordStatus(int firstCs, int lastCs, std::vector<uint64_t>& status)"<< endl;
      cout << "5.  motorPowered(int mnum, bool& powered)"<< endl;
      cout << "6.  axisGetVelocity(int axis, double& velocity)"<< endl;
      cout << "7.  axesGetVelocities(int firstAxis, int lastAxis, std::vector<double>& velocities)"<< endl;
      cout << "8.  axisSetVelocity(int axis, double velocity)"<< endl;
      cout << "9.  axisGetAcceleration(int axis, double& acceleration)"<< endl;
      cout << "10. axisSetAcceleration(int axis, double acceleration)"<< endl;
      cout << "11. axisGetSoftwareLimits(int axis, double& maxpos, double& minpos)"<< endl;
      cout << "12. axisSetSoftwareLimits(int axis, double maxpos, double minpos)"<< endl;    
      cout << "13. axisGetDeadband(int axis, double& deadband)"<< endl;
      cout << "14. axisSetDeadband(int axis, double deadband)"<< endl;
      cout << "15  axisMoveAbs(int axis, double position)"<< endl;
      cout << "16. axisMoveRel(int axis, double relposition)"<< endl;
      cout << "17. axisMovePositive(int axis)"<< endl;
      cout << "18. axisMoveNegative(int axis)"<< endl;
      cout << "19. axisGetCurrentPosition(int axis, double& position)"<< endl;
      cout << "20. axesGetCurrentPositions(int firstAxis, int lastAxis, std::vector<double>& positions)"<< endl;
      cout << "21. axisDefCurrentPos(int axis, double newpos)"<< endl;
      cout << "22. axisStop(int axis)"<< endl;
      cout << "23. axisAbort(int axis)"<< endl;
      cout << "24. axisHome(int axis)"<< endl;
      cout << "Please enter your selection: (0 to exit) ";
       
      string str;
      cin >> str;
      i = -1;
      sscanf(str.c_str(), "%d", &i);
      
      if (i == 0) break;

      switch(i)
      {
         case 1:
               {
               uint64_t mstatus;
               int mnum;
               cout << "Input axis number: ";
               cin >> mnum;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_getMotorStatus(mnum, mstatus);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << mnum << " status is: ";
                   cout << hex << uppercase << mstatus;
                   cout << nouppercase << dec << endl;
                 }
               break;
               }
         case 2:
               {
               std::vector<uint64_t> mstatus;
               int fnum, lnum;
               cout << "Input first and last axes numbers: ";
               cin >> fnum >> lnum;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_getMultiMotorStatus(fnum, lnum, mstatus);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Status for Axis " << fnum << " to Axis " << lnum << " is: " << endl;
                   printUint64Vec(mstatus);
                 }
               break;
               }
         case 3:
               {
               uint64_t cstatus;
               int csnum;
               cout << "Input CS number: ";
               cin >> csnum;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_getCoordStatus(csnum, cstatus);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "CS " << csnum << " status is: ";
                   cout << hex << uppercase << cstatus;
                   cout << nouppercase << dec << endl;
                 }
               break;
               }
         case 4:
               {
               std::vector<uint64_t> cstatus;
               int fnum, lnum;
               cout << "Input first and last CS numbers: ";
               cin >> fnum >> lnum;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_getMultiCoordStatus(fnum, lnum, cstatus);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Status for CS " << fnum << " to CS " << lnum << " is: " << endl;
                   printUint64Vec(cstatus);
                 }
               break;
               }
         case 5:
               {
                  int mnum;
                  bool powered;
                  std::string powstr;
                  cout << "Input motor number: ";
                  cin >> mnum;
                  cout << endl;
                  estatus = ppmaccomm->PowerPMACcontrol_motorPowered(mnum, powered);
                  checkPMACerror(estatus);
                  if (estatus == PowerPMACcontrol::PPMACcontrolNoError)
                  {
                     if (powered)
                         powstr = "Powered";
                     else
                         powstr = "Not Powered";
                     cout << "Motor " << mnum << " is " << powstr << endl;
                  }
                  break;
                }
         case 6:
               {
               double velocity;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisGetVelocity(axis, velocity);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " velocity is: ";
                   cout << velocity << endl;
                 }
               break;
               }
         case 7:
               {
               std::vector<double> velocities;
               int fnum, lnum;
               cout << "Input first and last axes numbers: ";
               cin >> fnum >> lnum;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axesGetVelocities(fnum, lnum, velocities);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Velocities for Axis " << fnum << " to Axis " << lnum << " are: " << endl;
                   printDoubleVec(velocities);
                 }
               break;
               }
         case 8:
               {
               double newvel;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               cout << "Input desired velocity: ";
               cin >> newvel;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisSetVelocity(axis, newvel);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " velocity has been set to ";
                   cout << newvel << endl;
                 }
               break;
               }
          case 9:
               {
               double accel;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisGetAcceleration(axis, accel);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " acceleration is: ";
                   cout << accel << endl;
                 }
               break;
               }
          case 10:
               {
               double newaccel;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               cout << "Input desired acceleration: ";
               cin >> newaccel;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisSetAcceleration(axis, newaccel);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " acceleration has been set to ";
                   cout << newaccel << endl;
                 }
               break;
               }
          case 11:
               {
               double maxpos, minpos;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisGetSoftwareLimits(axis, maxpos, minpos);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " Max and Min SW limits are: ";
                   cout << maxpos << " " << minpos << endl;
                 }
               break;
               }
          case 12:
               {
               double maxpos, minpos;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               cout << "Input new maximum and minimum positions: ";
               cin >> maxpos >> minpos;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisSetSoftwareLimits(axis, maxpos, minpos);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " Max and Min SW limits set to ";
                   cout << maxpos <<" " << minpos << endl;
                 }
               break;
               }
          case 13:
               {
               double deadband;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisGetDeadband(axis, deadband);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " Deadband is: ";
                   cout << deadband << endl;
                 }
               break;
               }
          case 14:
               {
               double deadband;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               cout << "Input desired deadband: ";
               cin >> deadband;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisSetDeadband(axis, deadband);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " Deadband has been set to ";
                   cout << deadband << endl;
                 }
               break;
               }
          case 15:
               {
               double abspos;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               cout << "Input desired absolute position: ";
               cin >> abspos;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisMoveAbs(axis, abspos);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " has been requested to move to ";
                   cout << abspos << endl;
                 }
               break;
               }
          case 16:
               {
               double relpos;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               cout << "Input desired relative movement: ";
               cin >> relpos;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisMoveRel(axis, relpos);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " has been requested to move by ";
                   cout << relpos << endl;
                 }
               break;
               }
          case 17:
               {
               int axis;
               cout << "Input axis number for positive move: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisMovePositive(axis);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " has had positive movement requested";
                 }
               break;
               }
          case 18:
               {
               int axis;
               cout << "Input axis number for negative move: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisMoveNegative(axis);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " has had negative movement requested";
                 }
               break;
               }
          case 19:
               {
               double position;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisGetCurrentPosition(axis, position);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " current position is: ";
                   cout << position << endl;
                 }
               break;
               }
         case 20:
               {
               std::vector<double> positions;
               int fnum, lnum;
               cout << "Input first and last axes numbers: ";
               cin >> fnum >> lnum;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axesGetCurrentPositions(fnum, lnum, positions);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Current positions for Axis " << fnum << " to Axis " << lnum << " are: " << endl;
                   printDoubleVec(positions);
                 }
               break;
               }
         case 21:
               {
               double oldpos, newpos;
               int axis;
               cout << "Input axis number: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisGetCurrentPosition(axis, oldpos);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
               {
                   checkPMACerror(estatus);
                   return;
               }
               else
                 {
                   cout << "Axis " << axis << " current position is: ";
                   cout << oldpos << endl;
                 }
               cout << "Input new value for current position: ";
               cin >> newpos;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisAbort(axis);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError)
               {
                   cout << "Error on axis abort " << axis << endl;
                   checkPMACerror(estatus);
                   return;
               }
               estatus = ppmaccomm->PowerPMACcontrol_axisDefCurrentPos(axis, newpos);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               else
                 {
                   cout << "Axis " << axis << " current position redefined as ";
                   cout << newpos << endl;
                 }
               break;
               }
         case 22:
               {
               int axis;
               cout << "Input number of axis to be stopped: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisStop(axis);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               break;
               }
         case 23:
               {
               int axis;
               cout << "Input number of axis to be aborted: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisAbort(axis);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               break;
               }
         case 24:
               {
               int axis;
               cout << "Input number of axis to be Homed: ";
               cin >> axis;
               cout << endl;
               estatus = ppmaccomm->PowerPMACcontrol_axisHome(axis);
               if (estatus != PowerPMACcontrol::PPMACcontrolNoError) 
                   checkPMACerror(estatus);
               break;
               }
         default:
             	cout << "Input value '" << str << "' was not recognised as a test case." << endl;
             break;
      } /* End switch */
      cout << endl;
      cout << "Press ENTER only to continue>";
      string s;
      getline( cin, s );
      while (getline( cin, s ) && !s.empty())
      cout << "Press only the ENTER key>";
   } /* End while */
  ppmaccomm->PowerPMACcontrol_disconnect();
  delete ppmaccomm;
  return;
}

void printStringVec(const vector<std::string> & strvec)
{
    vector<std::string>::const_iterator it;
    
    for(it=strvec.begin(); it!=strvec.end(); ++it)
        cout << *it << endl;
}

void printUint64Vec(const vector<uint64_t> & ivec)
{
    vector<uint64_t>::const_iterator it;
    
    for(it=ivec.begin(); it!=ivec.end(); ++it)
    {
        cout << hex << uppercase << *it;
        cout << nouppercase << dec << endl;
    }
}

void printDoubleVec(const vector<double> & dvec)
{
    vector<double>::const_iterator it;
    
    for(it=dvec.begin(); it!=dvec.end(); ++it)
        cout << *it << endl;
}

 void checkPMACerror(int estatus)
 {
     if (estatus != 0)
         cout << "PMAC call returned error " << estatus << endl;
 }
