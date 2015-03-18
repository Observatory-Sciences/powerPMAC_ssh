#
# Makefile to build the Power PMAC Communications library and test applications
#

CPP=g++
INCLUDE_DIRS=-I.\
	-I/usr/local/include

LIB_DIRS=-L. -L/usr/local/lib

CXXFLAGS=-D_REENTRANT -fpic -Wall $(INCLUDE_DIRS)
LFLAGS=$(LIB_DIRS) -lPowerPMACcontrol -lssh2 -lrt 

LIB_OBJS=libssh2Driver.o PowerPMACcontrol.o

INSTALL_DIR=/usr/local

.SUFFIXES: .cpp .o
.cpp.o:
	$(CPP) $(CXXFLAGS) -c $<;


all: multi_thread_test powerPMACShell testPowerPMACcontrolLib isConnected_test libPowerPMACcontrol.a libPowerPMACcontrol.so docs

libPowerPMACcontrol.a: $(LIB_OBJS)
	ar rcs libPowerPMACcontrol.a $(LIB_OBJS)

libPowerPMACcontrol.so: $(LIB_OBJS)	
	$(CPP) -shared -Wl,-soname,libPowerPMACcontrol.so \
	-o libPowerPMACcontrol.so -pthread \
	$(LIB_OBJS)

powerPMACShell: powerPMACShell.o argParser.o libPowerPMACcontrol.a $(LIB_OBJS)
	$(CPP) powerPMACShell.o argParser.o -o powerPMACShell $(LFLAGS)
	
testPowerPMACcontrolLib: testPowerPMACcontrolLib.o argParser.o libPowerPMACcontrol.a $(LIB_OBJS)
	$(CPP) testPowerPMACcontrolLib.o argParser.o -o testPowerPMACcontrolLib $(LFLAGS)	
	
isConnected_test: isConnected_test.o argParser.o libPowerPMACcontrol.a $(LIB_OBJS)
	$(CPP) isConnected_test.o argParser.o -o isConnected_test $(LFLAGS)	
	
multi_thread_test: multi_thread_test.o libPowerPMACcontrol.a $(LIB_OBJS)
	$(CPP) multi_thread_test.o -o multi_thread_test $(LFLAGS)
	
release: $(wildcard *.h) $(wildcard *.cpp) Doxyfile
	zip -r PowerPMACcontrol $(wildcard *.h) $(wildcard *.cpp) Doxyfile libssh2 msvc -x "*/.svn/*"
	./makeReleaseTar

install: $(wildcard *.h) libPowerPMACcontrol.a libPowerPMACcontrol.so
	install -D -m 0644  $(wildcard *.h) $(INSTALL_DIR)/include
	install -D -m 0644  libPowerPMACcontrol.a libPowerPMACcontrol.so $(INSTALL_DIR)/lib

clean:
	/bin/rm -f *.o *.a *.so core powerPMACShell isConnected_test multi_thread_test testPowerPMACcontrolLib *.zip *.tar.gz
	/bin/rm -rf html

.PHONY: docs
docs:
	doxygen Doxyfile


