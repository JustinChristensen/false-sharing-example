#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

#define CL_MEMORY_BLOCK_SIZE 1024
#define CL_THREAD_COUNT 2
#define CL_LIMIT 500000000
#define CL_VERSION "1.0.0"
#define CL_BASE_OFFSET 0

#define NS_TO_MSF 1000000.0

struct args {
    long offset;
    long limit;
};

struct instructions {
    const char *description;
    const char *default_value;
};

static int debug_mode = false;

static const struct option options[] = {
    { "offset", required_argument, NULL, 'o' },
    { "limit", required_argument, NULL, 'l' },
    { "debug", no_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 },
};

static const struct instructions instructions[] = {
    { "Offset in memory thread #2 should begin incrementing from", "0" },
    { "Limit (exclusive)", "512" },
    { "Debug mode", NULL },
    { "Print usage instructions and copyright information", NULL },
    { "Print version information", NULL  },
    { NULL, NULL }
};

static void print_usage()
{
    const char *row_format = "  -%-c, --%-11s  %-60s";
    const char *default_val_format = "  [default: %s]";

    printf("Usage: %s [options]\n\n", "false_sharing");

    for (int i = 0, len = sizeof options / sizeof *options - 1; i < len; i++) {
        printf(row_format, options[i].val, options[i].name, instructions[i].description);

        if (instructions[i].default_value != NULL) {
            printf(default_val_format, instructions[i].default_value);
        }

        printf("\n");
    }

    printf("\n");
}

static struct args *parse_args(int *argc, char **argv[])
{
#define ARGS_EXHAUSTED -1
    static struct args args = {
        .offset = 0,
        .limit = CL_MEMORY_BLOCK_SIZE / 2
    };

    long offset, limit;
    int c;
    bool exit = false;

    while (!exit && (c = getopt_long(*argc, *argv, "hdvl:o:", options, NULL)) != ARGS_EXHAUSTED) {
        switch (c) {
            case 'o':
                offset = strtol(optarg, NULL, 10);

                if (errno) {
                    perror("error: ");
                    exit = true;
                }
                else {
                    args.offset = offset;
                }
                break;
            case 'l':
                limit = strtol(optarg, NULL, 10);

                if (errno) {
                    perror("error: ");
                    exit = true;
                }
                else {
                    args.limit = limit;
                }
                break;
            case 'd':
                debug_mode = true;
                break;
            case 'v':
                printf("%s\n", CL_VERSION);
                exit = true;
                break;
            case 'h':
            case '?':
            case ':':
            default:
                print_usage();
                exit = true;
                break;
        }
    }

    *argv += optind;
    *argc -= optind;

    return !exit ? &args : NULL;
#undef ARGS_EXHAUSTED
}

static void print_thread_preamble(pthread_t thread_ptr)
{
    int r;
    uint64_t thread_id;

    if ((r = pthread_threadid_np(thread_ptr, &thread_id)) == 0) {
        printf("%-20s %llu\n", "entering thread:", thread_id);
    }
    else {
        fprintf(stderr, "%s\n", strerror(r));
    }
}

static void print_thread_epilogue(pthread_t thread_ptr)
{
    int r;
    uint64_t thread_id;

    if ((r = pthread_threadid_np(thread_ptr, &thread_id)) == 0) {
        printf("%-20s %llu\n", "exiting thread:", thread_id);
    }
    else {
        fprintf(stderr, "%s\n", strerror(r));
    }
}

static void *increment(void *args)
{
    int *n = args;
    pthread_t self = pthread_self();

    if (debug_mode)
        print_thread_preamble(self);

    for (int i = 0; i < CL_LIMIT; i++)
        ++*n;

    if (debug_mode)
        print_thread_epilogue(self);

    return EXIT_SUCCESS;
}

static pthread_t create_thread(int *arg)
{
    pthread_t thread_ptr;
    int r;

    if ((r = pthread_create(&thread_ptr, NULL, increment, arg)) != 0) {
        fprintf(stderr, "%s\n", strerror(r));
    }

    return thread_ptr;
}

int main(int argc, char *argv[])
{
    int exit_status = EXIT_SUCCESS;
    struct args *args = parse_args(&argc, &argv);
    mach_timebase_info_data_t timebase_info;
    double timebase_coefficient;

    mach_timebase_info(&timebase_info);

    timebase_coefficient = timebase_info.numer / timebase_info.denom;

    if (args != NULL) {
        int *block = calloc(CL_MEMORY_BLOCK_SIZE, sizeof *block);
        int x, y, offset_limit;

        if (block != NULL) {
            for (x = CL_BASE_OFFSET, y = args->offset, offset_limit = args->limit; y < offset_limit; y++) {
                pthread_t threads[CL_THREAD_COUNT];
                uint64_t start, elapsed;

                start = mach_absolute_time();

                // create both threads: x, y = y + 1
                threads[0] = create_thread(&block[x]);
                threads[1] = create_thread(&block[y]);

                // join both threads
                pthread_join(threads[0], NULL);
                pthread_join(threads[1], NULL);

                printf("%f\n", timebase_coefficient);

                elapsed = (mach_absolute_time() - start) * timebase_coefficient;

                printf("x: %d, y: %d, elapsed: %f ms\n", x, y, elapsed / NS_TO_MSF);

                // reset x and y to 0
                block[x] = 0;
                block[y] = 0;
            }

            free(block);
        }
        else {
            fprintf(stderr, "error: %s", "could not allocate memory for the block");
            exit_status = EXIT_FAILURE;
        }
    }

    return exit_status;
}
