CC = gcc
CFLAGS=-g -fPIC -Wall -pipe -fno-ident -D_GNU_SOURCE -D_REENTRANT -DTIXML_USE_STL

INCLUDE=-I. 
LIBS=

BINARY = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
TARGET = lib_taf.a

all: ${TARGET}

${TARGET}: $(BINARY) 
	ar -r $@ $^
	
.cpp.o:
	$(CC) ${CFLAGS} $(INCLUDE) -c $^
	
clean:
	rm -f ${BINARY} ${TARGET}
	
all32:
	make -e ARCH=32
