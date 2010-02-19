# this makefile is to be used with gmake
include commonrules.mk

FCVERSION = $(shell tr -d \"\\"\\"\" < src/filecache.version)

SRC.dir = src/
BIN.dir = bin/

LIBNAME = lib$(LIB)-$(FCVERSION).so
LIBBINARY = $(BIN.dir)$(LIBNAME)
LIBBINARYLINK = $(BIN.dir)lib$(LIB).so

SOURCES = \
	filecache.cpp

INCLUDES = \
	-Iinclude \
	-I$(RND)/include/boost-1.34.1 \
	-I$(RND)/include

DEFINES_optimized =  -DNDEBUG
DEFINES_debug =  -DDEBUG

LIB.dir = lib/
OBJ.dir = obj/
CPPOBJ  = $(patsubst %.cpp,$(OBJ.dir)%.o,$(filter %.cpp,$(SOURCES)))

CFLAGS_ = -O2 -fPIC
CFLAGS_optimized = $(CFLAGS_) -march=pentium4 -O3
CFLAGS_debug = -fPIC -g -O0 -gstabs+

CFLAGS  = $(INCLUDES) $(CFLAGS_$(COMPILE_OPTION)) -shared
CXX     = g++

LDFLAGS = -L$(RND)/lib/$(OSname) -lboost_filesystem
DEFINES = -DLINUX -DUNIX -DLINUX_64 -DBits64_ $(DEFINES_$(COMPILE_OPTION))


#all: $(LIBBINARY) regression/bin/test swig/bin/_filecache.so rslplugin/bin/filecache.so
all: $(LIBBINARYLINK) swig/bin/_filecache.so rslplugin/bin/filecache.so

swig/bin/_filecache.so: $(LIBBINARYLINK)
	@cd swig; make

# regression/bin/test: $(LIBBINARY)
#	@cd regression; make

rslplugin/bin/filecache.so: $(LIBBINARYLINK)
	@cd rslplugin; make

$(OBJ.dir)%.o: $(SRC.dir)%.cpp
	@echo $@
	@if [ ! -d "$(OBJ.dir)" ]; then mkdir -p "$(OBJ.dir)"; fi
	@$(CXX) -c $(CFLAGS) $(DEFINES) -o $@ $< -Fo$@

$(LIBBINARY): $(CPPOBJ)
	@echo ________________________________________________________________________________
	@echo Creating $@
	@if [ ! -d "$(BIN.dir)" ]; then mkdir -p "$(BIN.dir)"; fi
	@$(CXX) $(CFLAGS) $(CPPOBJ) -L$(BIN.dir) $(LDFLAGS) -o $@
	@strip --strip-all $@

$(LIBBINARYLINK): $(LIBBINARY)
	rm $@ &>/dev/null
	ln -s $(LIBBINARY) $@
	
# test:
#	@cd regression; make test


clean:
	@cd regression; make clean
	@cd swig; make clean
	@cd rslplugin; make clean
	@-rm -rf $(OBJ.dir)*.o $(BIN.dir)*
