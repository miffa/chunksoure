# A sample Makefile for building Google Test and using it in user
# tests.  Please tweak it to suit your environment and project.  You
# may want to move it to your project's root directory.
#
# SYNOPSIS:
#
#   make [all]  - makes everything.
#   make TARGET - makes the given target.
#   make clean  - removes all files generated by make.

# Please tweak the following variable definitions as needed by your
# project, except GTEST_HEADERS, which you can use in your own targets
# but shouldn't modify.

##################################### Variable Definition #########################################
SHELL = /bin/sh

# Where to find src code.
THELIB_DIR = ../thelib
#THELIB_SRC=$(THELIB_DIR)/src/netio
THELIB_SRC=$(THELIB_DIR)/src
#THLIB_SRC_1=$(THELIB_DIR)/src/p2p
THELIB_INC=$(THELIB_DIR)/include

THIRD_PARTY_INC=../../3rdparty/include 

COMMON_DIR=../common
COMMON_INC=$(COMMON_DIR)/include
COMMON_SRC=$(COMMON_DIR)/src

RTMP_DIR=./librtmp

SRC_DIR = .
BIN_DIR = .
DIST_DIR = ../dist
UTIL_DIR = ../util
BUILD_DIR = ../build
THIRD_PARTY_LIBS =../../3rdparty

# All code path, make depend path
VPATH = $(JEEP_DIR):$(SRC_DIR):$(RECORD_DIR):$(VOICE_DIR)

# Lib for link -L
LIB_PATH = . 

# Lib for link -l
LIBS = pthread ssl lua boost_date_time

STATIC_LIB = # $(THIRD_PARTY_LIBS)/lib/json/libjson_linux-gcc-4.1.2_libmt.a\
 	$(THIRD_PARTY_LIBS)/lib/mysql/mysql/libmysqlclient_r.a \
	$(THIRD_PARTY_LIBS)/lib/glog/libglog.a \
	$(THIRD_PARTY_LIBS)/lib/boost/libboost_date_time.a \
	$(THIRD_PARTY_LIBS)/lib/boost/libboost_system.a \
	/usr/lib64/libcurl.a \
	/usr/lib64/libidn.a \
	$(THIRD_PARTY_LIBS)/lib/libmemcached/libmemcached.a

# Lib for preprocessor -I
INC_PATH = $(SRC_DIR) $(COMMON_INC) $(RTMP_DIR) $(THELIB_INC) $(THIRD_PARTY_INC)
	#/usr/local/include

PLATFORM_DEFINES = \
	-DLINUX \
	-DLITTLE_ENDIAN_BYTE_ALIGNED \
	-DNET_EPOLL \
	-DHAS_SAFE_LOGGER \
	-DLINUX64 \
	-DNO_CRYPTO 
	#-D_DEBUG 
	#-D_FILE_DEBUG_ 


# Flags passed to the preprocessor.
CPPFLAGS += $(addprefix -I,$(INC_PATH))

# Flags passed to the C compiler
SVN_REV := $(shell svnversion -n .)
SVNDEV := -D'SVN_REV="$(SVN_REV)"'
CFLAGS += -Wall -g $(PLATFORM_DEFINES) $(SVNDEV)
#CFLAGS += -Wall -O2 -g -DLINUX64 -D__DEBUG__ 
# CSTD = -std=c99
#############################################################################
OUTPUT = $(BIN_DIR)/chunksource

# House-keeping build targets.
CPP_EXT := cpp
C_EXT := c

RTMP_SOURCE := $(shell find $(RTMP_DIR) -type f -name "*.$(C_EXT)")
RTMP_OBJECT := $(patsubst $(RTMP_DIR)/%, $(BUILD_DIR)/%, $(RTMP_SOURCE:.$(C_EXT)=.o))

C_SOURCE := $(RTMP_SOURCE)
C_OBJECT := $(RTMP_OBJECT)

COMMON_SOURCE := $(shell find $(COMMON_SRC)   -type f  -name "*.$(CPP_EXT)")
COMMON_OBJECT := $(patsubst $(COMMON_SRC)/%, $(BUILD_DIR)/%, $(COMMON_SOURCE:.$(CPP_EXT)=.o))

THELIB_SOURCE := $(shell find $(THELIB_SRC)   -type f -name "*.$(CPP_EXT)")
THELIB_OBJECT := $(patsubst $(THELIB_SRC)/%, $(BUILD_DIR)/%, $(THELIB_SOURCE:.$(CPP_EXT)=.o))

#THELIB_SOURCE_1 := $(shell find $(THELIB_SRC_1)   -type f -name "*.$(CPP_EXT)")
#THELIB_OBJECT_1 := $(patsubst $(THELIB_SRC_1)/%, $(BUILD_DIR)/%, $(THELIB_SOURCE_1:.$(CPP_EXT)=.o))

CPP_SOURCE := $(shell find $(SRC_DIR)  -path "$(RTMP_DIR)*" -a -prune -type f -o -name "*.$(CPP_EXT)") 
CPP_OBJECT := $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(CPP_SOURCE:.$(CPP_EXT)=.o))

SOURCES := $(C_SOURCE) $(COMMON_SOURCE) $(THELIB_SOURCE) $(THELIB_SOURCE_1) $(CPP_SOURCE)
OBJECTS := $(C_OBJECT) $(COMMON_OBJECT) $(THELIB_OBJECT) $(THELIB_OBJECT_1) $(CPP_OBJECT)

###############################################################################
all : $(OUTPUT)

$(BUILD_DIR)/%.o: $(RTMP_DIR)/%.c 
	@mkdir -p $(shell dirname $@) 
	gcc $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(COMMON_SRC)/%.cpp 
	@mkdir -p $(shell dirname $@) 
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(THELIB_SRC)/%.cpp 
	@mkdir -p $(shell dirname $@) 
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

#$(BUILD_DIR)/%.o: $(THELIB_SRC_1)/%.cpp 
#	@mkdir -p $(shell dirname $@) 
#	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp 
	@mkdir -p $(shell dirname $@) 
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OUTPUT): $(OBJECTS) 
	$(CXX) -lrt -o $@ $^ $(addprefix -L,$(LIB_PATH)) $(addprefix -l,$(LIBS)) $(STATIC_LIB)


.PHONY : clean
clean : 
	rm -f $(OUTPUT) $(OBJECTS)

dist : $(OUTPUT) 
	tar -zcvf $(DIST_DIR)/statserver.$(subst :,_,$(SVN_REV)).tar.gz \
        statserver \
        ip.txt conf ver \
        logrun.pl \
        $(UTIL_DIR)/*.sh


