PROGRAM := false_sharing_test
MACH_TIMESPEC := timespec_test
MACH_ABSOLUTE := absolute_test
CFLAGS := -O0
LDFLAGS :=

all: $(PROGRAM) $(MACH_TIMESPEC) $(MACH_ABSOLUTE)

$(PROGRAM): false_sharing.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $+

$(MACH_TIMESPEC): mach_timespec.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $+

$(MACH_ABSOLUTE): absolute.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $+

clean:
	-rm -rf $(PROGRAM) $(MACH_ABSOLUTE) $(MACH_TIMESPEC) *.dSYM

.PHONY: all clean
