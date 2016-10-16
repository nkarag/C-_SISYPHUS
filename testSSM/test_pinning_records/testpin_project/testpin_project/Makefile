# ---------------------------------------------------------------- #
# -- Copyright (c) 1999, Knowledge and Database Systems Laboratory #
# -- National Technical University of Athens (NTUA).               # 
# ---------------------------------------------------------------- #

#
# Makefile for testpin,
#
# Author: Nikos Karayannidis



# Modify the following line as appropriate to point to the place where
# Shore is installed
SHORE = /usr/local/shore2

#DISKRW = $(SHORE)/bin/diskrw
DISKRW = $(SHORE)/installed/bin/diskrw

# Applications used

RM = rm -f


# Modify the following as desired to control compilation options
# NOTE:  -lnsl is only required for Solaris
CC = /usr/local/shore2/bin/g++

CCFLAGS = -fPIC -g -fexceptions -ftemplate-depth-25 -DDEBUGGING

INCLUDE = -I$(SHORE)/installed/include                  \
          -I$(SHORE)/installed/include/sm               \
          -I$(SHORE)/installed/include/sthread          \
          -I$(SHORE)/installed/include/fc               \
          -I$(SHORE)/installed/include/common           \
          -I$(SHORE)/build/src/smlayer/common		\
          -I$(SHORE)/build/src/smlayer/sm		\
          -I$(SHORE)/build/src/smlayer/sthread          \
          -I$(SHORE)/include/g++	
	
LIBPATH = -L$(SHORE)/lib  -L$(SHORE)/installed/lib

LIBS =  -lSM -lCOMMON -lSTHREAD -lFC #-lnsl  -lsocket

COMPILE = $(CC) -c $(CCFLAGS) $(INCLUDE)
LINK = $(CC) $(CCFLAGS) $(INCLUDE) -o

SERVER = testpin

CONFIG_FILE  = config

LOG_FILE_DIR = log.ssph

DEVICE_NAME  = device.ssph

TARGET = $(SERVER)

OBJ_FILES_SRV = SsmStartUpThread.o		\
		SystemManager.o			\
		TestPinClass.o			\
		testpin.o			

DEPENDENCIES_FILE = Makefile.Dependencies

# Default compilation rule for C++ files. 
# Object files depend on corresponding header and source files.

%.o: %.h %.C
	$(COMPILE) $*.C

%.o: %.C
	$(COMPILE) $*.C



all :  $(TARGET) $(CONFIG_FILE) $(LOG_FILE_DIR) usage

usage:
	@echo 
	@echo '****************************************************************'	
	@echo '* To run testpin Server give: ./testpin [-i]                   *'
	@echo '* -i: initialize database (necessary the FIRST time!)          *'
	@echo '*                                                              *'
	@echo '* "server" is an alias for "testpin server"                    *'
	@echo '****************************************************************'
	@echo

# Automatically generate the dependencies using
# the -MM option of gcc

depend:
	$(COMPILE) -MM *.C > $(DEPENDENCIES_FILE)
 
# Top Level Dependencies

$(SERVER) : $(OBJ_FILES_SRV)
	$(LINK) $(SERVER) $(OBJ_FILES_SRV) $(LIBPATH) $(LIBS)

$(CLIENT) : $(OBJ_FILES_CLN)
	$(LINK) $(CLIENT) $(OBJ_FILES_CLN) $(LIBPATH) $(LIBS)

# Include the automatically generated dependencies

include $(DEPENDENCIES_FILE)

# Log and Config File

$(LOG_FILE_DIR):
	mkdir $@

$(CONFIG_FILE): $(DISKRW) exampleconfig
	sed -e "s,DISKRW,$(DISKRW)," exampleconfig > $(CONFIG_FILE)

# Cleanup

clean :
	$(RM) core *.o *~

distclean : clean 
	$(RM) $(SERVER) $(CLIENT) $(CONFIG_FILE)
	$(RM) $(DEVICE_NAME)
	$(RM) -r $(LOG_FILE_DIR)

.PHONY : depend usage clean distclean
