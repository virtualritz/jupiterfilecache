# this makefile is to be used with gmake
include ../commonrules.mk

SRC.dir = src/
INC.dir = include/
BIN.dir = bin/
INT.dir = interface/
LIB.dir = ../bin/

SOURCES = \
	filecache_wrap.cpp

INTERFACES = \
	filecache.i

INCLUDES = \
	-I$(INC.dir) \
	-I../include \
	-I/usr/include/python2.6

LIBS = \
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
SFLAGS  = -c++ -python
SWIG    = swig

DEFINES = -DLINUX -DUNIX -DLINUX_64 -DBits64_ $(DEFINES_$(COMPILE_OPTION))

BINARY = $(BIN.dir)_filecache.so



$(OBJ.dir)%.o: $(SRC.dir)%.cpp $(SRC.dir)filecache_wrap.cxx
	@echo $@
	@if [ ! -d "$(OBJ.dir)" ]; then mkdir -p "$(OBJ.dir)"; fi
	@$(CXX) -c $(CFLAGS) $(DEFINES) -o $@ $< -Fo$@

$(BINARY): $(CPPOBJ) $(LIB.dir)/$(GENERICLIBNAME)
	@echo ________________________________________________________________________________
	@echo Creating $@
	@if [ ! -d "$(BIN.dir)" ]; then mkdir -p "$(BIN.dir)"; fi
	@$(CXX) -shared $(CFLAGS) $(CPPOBJ) -L$(BIN.dir) $(LIBS) -o $@
	@strip --strip-all $@

$(SRC.dir)filecache_wrap.cxx: $(INT.dir)filecache.i
	@echo $@
	@if [ ! -d "$(BIN.dir)" ]; then mkdir -p "$(BIN.dir)"; fi
	@$(SWIG) $(SFLAGS) -outdir $(BIN.dir) $<
	@if [ ! -d "$(INC.dir)" ]; then mkdir -p "$(INC.dir)"; fi
	@mv $(INT.dir)filecache_wrap.cxx $(INC.dir)filecache_wrap.cxx

all: $(BINARY)

clean:
	@-rm -rf $(OBJ.dir)*.o $(BIN.dir)*
	@-rm -rf $(INC.dir)*.cxx
