# this makefile is to be used with gmake
include ../commonrules.mk

DSOVERSION = $(shell tr -d \"\\"\\"\" < src/rslfilecache.version)

SRC.dir = src/
BIN.dir = bin/
LIB.dir = ../bin/

DSONAME = $(LIB)-$(DSOVERSION).so
DSOBINARY = $(BIN.dir)$(DSONAME)

SOURCES = \
	filecache.cpp

INCLUDES = \
	-I../include \
	-I$(RMANTREE)/include

LDFLAGS = \
	-L$(LIB.dir) \
	-l$(LIB)

DEFINES_optimized =  -DNDEBUG
DEFINES_debug =  -DDEBUG

OBJ.dir = obj/
CPPOBJ  = $(patsubst %.cpp,$(OBJ.dir)%.o,$(filter %.cpp,$(SOURCES)))

CFLAGS_ = -O2 -fPIC
CFLAGS_optimized = $(CFLAGS_) -march=pentium4 -O3
CFLAGS_debug = -fPIC -g -O0 -gstabs+

CFLAGS  = $(INCLUDES) $(CFLAGS_$(COMPILE_OPTION)) -shared
CXX     = g++

DEFINES = -DLINUX -DUNIX -DLINUX_64 -DBits64_ $(DEFINES_$(COMPILE_OPTION))

all: $(DSOBINARY)

$(OBJ.dir)%.o: $(SRC.dir)%.cpp
	@echo $@
	@if [ ! -d "$(OBJ.dir)" ]; then mkdir -p "$(OBJ.dir)"; fi
	@$(CXX) -c $(CFLAGS) $(DEFINES) -o $@ $< -Fo$@

$(DSOBINARY): $(CPPOBJ)
	@echo ________________________________________________________________________________
	@echo Creating $@
	@if [ ! -d "$(BIN.dir)" ]; then mkdir -p "$(BIN.dir)"; fi
	$(CXX) -shared $(CFLAGS) $(CPPOBJ) $(LDFLAGS) -o $@
	@strip --strip-all $@
	@cd $(BIN.dir); rm $(LIB).so &>/dev/null; ln -s $(DSONAME) $(LIB).so

test:
	@cd regression; make test


clean:
	@-rm -rf $(OBJ.dir)*.o $(BIN.dir)*
