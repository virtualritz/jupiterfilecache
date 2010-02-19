# this makefile is to be used with gmake
include ../commonrules.mk

SRC.dir = src/
BIN.dir = bin/
LIB.dir = ../bin/

SOURCES = \
	test.cpp

INCLUDES = \
	-Iinclude \
	-I../include \
	-I$(RND)/include/boost-1.34.1 \
	-I$(RND)/include

LDFLAGS = \
	-L$(RND)/lib/$(OSname) \
	-L$(LIB.dir) \
	-l$(LIB)



DEFINES_optimized =  -DNDEBUG
DEFINES_debug =  -DDEBUG

OBJ.dir = obj/
CPPOBJ  = $(patsubst %.cpp,$(OBJ.dir)%.o,$(filter %.cpp,$(SOURCES)))

CFLAGS_ = -O2 -fPIC
CFLAGS_optimized = $(CFLAGS_) -march=pentium4 -O3
CFLAGS_debug = -fPIC -g -O0 -gstabs+

CFLAGS  = $(INCLUDES) $(CFLAGS_$(COMPILE_OPTION))
CXX     = g++

DEFINES = -DLINUX -DUNIX -DLINUX_64 -DBits64_ $(DEFINES_$(COMPILE_OPTION))

BINARY = $(BIN.dir)test

$(OBJ.dir)%.o: $(SRC.dir)%.cpp
	@echo $@
	@if [ ! -d "$(OBJ.dir)" ]; then mkdir -p "$(OBJ.dir)"; fi
	@$(CXX) -c $(CFLAGS) $(DEFINES) -o $@ $< -Fo$@

$(BINARY): $(CPPOBJ) $(LIB.dir)/$(GENERICLIBNAME)
	@echo ________________________________________________________________________________
	@echo Creating $@
	@if [ ! -d "$(BIN.dir)" ]; then mkdir -p "$(BIN.dir)"; fi
	@$(CXX) $(CFLAGS) $(CPPOBJ) -L$(BIN.dir) $(LDFLAGS) -o $@
	@strip --strip-all $@

test: $(BINARY)
	@rm bin/runtest.sh
	@echo "#!/bin/sh" >> bin/runtest.sh
	@echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../bin" >> bin/runtest.sh
	@echo "bin/test" >> bin/runtest.sh
	@chmod 755 bin/runtest.sh
	@bin/runtest.sh


all: $(BINARY)

clean:
	@-rm -rf $(OBJ.dir)*.o $(BIN.dir)*
