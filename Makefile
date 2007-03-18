KERNELRELEASE ?= $(shell uname -r)
KSRC ?= /lib/modules/$(KERNELRELEASE)/build

CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes -g -I$(KSRC)/include

PROGRAMS = sgv4_dd

sgv4_dd: sgv4_dd.o libbsg.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGRAMS)
