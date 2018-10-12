PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

TARGET = libgenepp.so
SRCS = $(wildcard *.cpp) 				#wildcard : get all files in current directory
OBJS = gene_tsp.c #$(patsubst %cpp,%o,$(SRCS)) 	#patsubst : replace

$(warning  $(SRCS)) #print
$(warning  $(OBJS))

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
	TARGET = libgene.so
endif

CFLAGS += -shared -fPIC -I../
CFLAGS += -DDBG_PRINT

all: $(TARGET)

%.o:	$(PROJECT_ROOT)%.c
	$(CXX) -c $(CFLAGS) -o $@ $<
	
#%.o:	$(PROJECT_ROOT)%.c
#	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

libgene.so:	$(OBJS)
	$(CC)  $(CFLAGS) -o $@ $^ -L.. -lkmp -lssl
	mv $@ ..
	
libgenepp.so: $(OBJS)
	$(CC)  $(CFLAGS) -o $@ $^ -L.. -lkmp -lssl
	mv $@ ..
	
#libbpe.a:	$(OBJS)
#	ar crv $@ $^
	
clean:
	rm -fr *.so *.o