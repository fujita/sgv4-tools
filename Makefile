KERNELRELEASE ?= $(shell uname -r)
KSRC ?= /lib/modules/$(KERNELRELEASE)/build

CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes -g -I$(KSRC)/include

PROGRAMS = sgv4_dd sgv4_bench smp_rep_manufacturer

all: $(PROGRAMS)

sgv4_dd: sgv4_dd.o libbsg.o
	$(CC) $^ -o $@

sgv4_bench: sgv4_bench.o libbsg.o
	$(CC) $^ -o $@

smp_rep_manufacturer: smp_rep_manufacturer.o libbsg.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGRAMS)
