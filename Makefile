CFLAGS += -D_GNU_SOURCE
CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes -g

PROGRAMS = sgv2_inq sgv2_dd sgv4_inq sgv4_dd sgv4_bench smp_rep_manufacturer sgv4_xdwriteread

all: $(PROGRAMS)

sgv2_dd: sgv2_dd.o
	$(CC) $^ -o $@

sgv2_inq: sgv2_inq.o
	$(CC) $^ -o $@

sgv4_inq: sgv4_inq.o libbsg.o
	$(CC) $^ -o $@

sgv4_dd: sgv4_dd.o libbsg.o
	$(CC) $^ -o $@

sgv4_bench: sgv4_bench.o libbsg.o
	$(CC) $^ -o $@

sgv4_xdwriteread: sgv4_xdwriteread.o libbsg.o
	$(CC) $^ -o $@

smp_rep_manufacturer: smp_rep_manufacturer.o libbsg.o libsmp.o
	$(CC) $^ -o $@

clean:
	rm -f *.o $(PROGRAMS)
