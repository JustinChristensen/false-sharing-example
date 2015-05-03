#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#define MS_TO_NS 1000000

int main(int argc, char *argv[])
{
    uint64_t start, elapsed;
    mach_timebase_info_data_t timebase_info;
    double timebase_constant;

    mach_timebase_info(&timebase_info);

    timebase_constant = timebase_info.numer / timebase_info.denom;

    if (argc < 2) {
        fprintf(stderr, "Usage: program time_in_milliseconds\n");
        exit(EXIT_FAILURE);
    }

    long ms = strtol(argv[1], NULL, 10);
    unsigned long seconds = ms / 1000;
    ms %= 1000;

    printf("requested time: %ld secs, %ld ms\n", seconds, ms);

    const struct timespec wait = {
        .tv_sec = seconds,
        .tv_nsec = ms * MS_TO_NS
    };

    struct timespec remaining;

    start = mach_absolute_time();

    if (nanosleep(&wait, &remaining) != 0) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    elapsed = (mach_absolute_time() - start) * timebase_constant;

    printf("sec: %llu, ns: %llu\n", elapsed / 1000000000, elapsed % 1000000000);

    return EXIT_SUCCESS;
}
