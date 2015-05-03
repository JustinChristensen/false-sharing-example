#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mach/clock.h>
#include <mach/mach.h>

#define MS_TO_NS 1000000

int main(int argc, char *argv[])
{
    mach_port_t host_port = mach_host_self();    // host message queue
    clock_serv_t clock_port;                // clock service message queue
    mach_timespec_t start, end;
    kern_return_t kr;                       // kernel return code

    if (argc < 2) {
        fprintf(stderr, "Usage: program time_in_milliseconds\n");
        exit(EXIT_FAILURE);
    }

    long ms = strtol(argv[1], NULL, 10);
    unsigned long seconds = ms / 1000;
    ms %= 1000;

    printf("requested time: %ld secs, %ld ms\n", seconds, ms);

    // get the host clock service
    kr = host_get_clock_service(host_port, REALTIME_CLOCK, &clock_port);

    mach_port_deallocate(mach_task_self(), host_port);

    if (kr == KERN_SUCCESS) {
        kr = clock_get_time(clock_port, &start);

        if (kr == KERN_SUCCESS) {
            const struct timespec wait = {
                .tv_sec = seconds,
                .tv_nsec = ms * MS_TO_NS
            };

            struct timespec remaining;

            if (nanosleep(&wait, &remaining) != 0) {
                perror(NULL);
                exit(EXIT_FAILURE);
            }

            kr = clock_get_time(clock_port, &end);

            if (kr == KERN_SUCCESS) {
                mach_port_deallocate(mach_task_self(), clock_port);
                SUB_MACH_TIMESPEC(&end, &start);
                printf("secs: %u, ns: %d\n", end.tv_sec, end.tv_nsec);
            }
            else {
                fprintf(stderr, "kern failure: %s\n", mach_error_string(kr));
            }
        }
        else {
            fprintf(stderr, "kern failure: %s\n", mach_error_string(kr));
        }
    }
    else {
        fprintf(stderr, "kern failure: %s\n", mach_error_string(kr));
    }

    return EXIT_SUCCESS;
}
