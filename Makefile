PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = kmp.o md5.o
OBJS_LIB = libmd5.o libkmp.o
OBJS_SERVER = kmp_server.o

BUILD_MODE = run

ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -O2
else
	#$(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif

all:	libkmp.so kmp kmp_server

kmp:	$(OBJS)
	$(CXX) -o $@ $^ -L. -lkmp -lcrypto -lssl
	
libkmp.so:	$(OBJS_LIB)
	$(CXX) -shared -fPIC -o $@ $^	
	
kmp_server:	$(OBJS_SERVER)
	$(CXX) -o $@ $^ -L. -lpthread -lkmp -lcrypto -lssl

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
	
libmd5.o:	md5.cpp
	$(CXX) -c $(CFLAGS)  -fPIC $(CXXFLAGS) $(CPPFLAGS) -o $@ $<
libkmp.o:	libkmp.cpp
	$(CXX) -c $(CFLAGS) -std=c++11 -DKMP_SO  -fPIC $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr kmp libkmp.so kmp_server $(OBJS) $(OBJS_LIB) $(OBJS_SERVER)
