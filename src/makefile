TAF_PATH = /home/zhanghong/Project/Project/tool/src/test_frame/tcinclude
JSON_PATH = /home/xucuiping/3rd_party/sdk/json-c
INCLUDE=-I. -I$(TAF_PATH) -I$(JSON_PATH)/include

LIBS= -lpthread $(TAF_PATH)/lib_taf.a  $(JSON_PATH)/lib/libjson-c.a

CC=gcc
CXX=g++
GCCVER := $(shell $(CC) -dumpversion | awk -F. '{ print $$1"."$$2}' )

CFLAGS=-g -O2 -fPIC -Wall -pipe -fno-ident -D_GNU_SOURCE -D_REENTRANT -DTIXML_USE_STL -Wno-deprecated

TARGET = iot_load_test

BINARY = $(patsubst %.cpp, obj/%.o, $(wildcard *.cpp))

all:$(TARGET)

$(TARGET):$(BINARY)
	$(CXX) $? -o $@ $(CFLAGS) $(INCLUDE) $(LIBS)
	cp $(TARGET) ../bin/

obj/%.o:%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) $(INCLUDE)

clean:
	@rm -f $(TARGET) $(BINARY)

all32:
	make -e ARCH=32
