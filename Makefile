PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS_LIBKMP = libkmp.o #libmd5.o 
OBJS_LIBTEST = libtest.o
#OBJS_LIBTEST_PROTECT = libtestp.o
OBJS_LIBTEST_PROTECT_PRINT = libtestpp.o
OBJS = kmp.o md5.o ptp.o
OBJS_SERVER = kmp_server.o ptp.o

BUILD_MODE = run

ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -O2
else
	#$(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif
 
ifeq ($(KMP), 0)
	CFLAGS += -DKMP_DISABLE
endif

ifeq ($(DBG), 1)
	CFLAGS += -DDBG_PRINT
endif

all:	libkmp.so  libtest.so  libtestpp.so  kmp  kmp_server

libkmp.so:	$(OBJS_LIBKMP)
	$(CC) -shared -fPIC -o $@ $^	
	
#libkmp.a:	$(OBJS_LIBKMP)	
#	ar crv $@ $^
	
libtest.so:	$(OBJS_LIBTEST)
	$(CXX)  -shared -fPIC -o $@ $^  -L. -lssl -lkmp
	
#libtestp.so:	$(OBJS_LIBTEST_PROTECT)
#	$(CXX)  -shared -fPIC -o $@ $^  -L. -lssl -lkmp	
libtestpp.so:	$(OBJS_LIBTEST_PROTECT_PRINT)
	$(CXX)  -shared -fPIC -o $@ $^  -L. -lssl -lkmp
	
kmp:	$(OBJS)
	$(CXX) -o $@ $^ -L. -lcrypto -lssl -lkmp -ldl
	
kmp_server:	$(OBJS_SERVER)
	$(CXX) -o $@ $^ -L. -lpthread -lcrypto -lssl -lkmp -lbzip2pp -lbpepp -ldl

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
	
#libmd5.o:	md5.cpp
	#$(CXX) -c $(CFLAGS)  -fPIC $(CXXFLAGS) $(CPPFLAGS) -o $@ $<
libkmp.o:	libkmp.c
	$(CC) -c $(CFLAGS) -DKMP_SO -shared -fPIC $(CXXFLAGS) $(CPPFLAGS) -o $@ $<
libtest.o:	libtest.cpp
	$(CXX) -c -DKMP_DISABLE -DDBG_PRINT -std=c++11 -shared -fPIC $(CFLAGS)  $(CXXFLAGS) $(CPPFLAGS) -o $@ $<
libtestpp.o: libtest.cpp
	$(CXX) -c -DDBG_PRINT -std=c++11 -shared -fPIC $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<
	
clean:
	rm -fr libkmp.so  libkmp.a  libtest.so  libtestpp.so  kmp  kmp_server
	rm -f $(OBJS_LIBTEST) $(OBJS_LIBTEST_PROTECT_PRINT) $(OBJS_LIBKMP) $(OBJS) $(OBJS_SERVER)
