#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_TRACE_FILES 3
#define TEAM_NUMBER "12" // Team 12 per your setup

typedef struct {
    int cache_size_kb;                    // -s: 8 to 8192 KB
    int block_size_bytes;                 // -b: 4, 8, 16, 32, 64 bytes
    int associativity;                    // -a: 1, 2, 4, 8, 16
    char replacement_policy[8];           // -r: RR or RND
    int phys_memory_mb;                   // -p: 128 to 4096 MB
    double os_mem_percent;                // -u: 0 to 100%
    int time_slice;                       // -n: 1 to max (-1 for MAX)
    char trace_files[MAX_TRACE_FILES][256]; // -f: 1 to 3 trace files
    int trace_file_count;
    int max_milestone;                    // -m: 1, 2, or 3
} SimConfig;

static inline void print_usage(const char *prog_name) {
    printf("Usage: %s -s <cache_size_kb> -b <block_size> -a <associativity> "
           "-r <rr|rnd> -p <phys_mem_mb> -u <os_percent> -n <time_slice> "
           "-f <trace1.trc> [-f <trace2.trc> -f <trace3.trc>] [-m <1|2|3>]\n", prog_name);
}

static inline bool is_power_of_two(int x) {
    return x > 0 && (x & (x - 1)) == 0;
}

static inline bool parse_arguments(int argc, char *argv[], SimConfig *config) {
    int opt;
    memset(config, 0, sizeof(SimConfig));
    config->max_milestone = 1;

    while ((opt = getopt(argc, argv, "s:b:a:r:p:u:n:f:m:")) != -1) {
        switch (opt) {
            case 's': config->cache_size_kb = atoi(optarg); break;
            case 'b': config->block_size_bytes = atoi(optarg); break;
            case 'a': config->associativity = atoi(optarg); break;
            case 'r':
                strncpy(config->replacement_policy, optarg, sizeof(config->replacement_policy) - 1);
                config->replacement_policy[sizeof(config->replacement_policy) - 1] = '\0';
                break;
            case 'p': config->phys_memory_mb = atoi(optarg); break;
            case 'u': config->os_mem_percent = atof(optarg); break;
            case 'n': config->time_slice = atoi(optarg); break;
            case 'f':
                if (config->trace_file_count < MAX_TRACE_FILES) {
                    strncpy(config->trace_files[config->trace_file_count], optarg, 255);
                    config->trace_files[config->trace_file_count][255] = '\0';
                    config->trace_file_count++;
                } else {
                    fprintf(stderr, "Error: Maximum of %d trace files exceeded.\n", MAX_TRACE_FILES);
                    return false;
                }
                break;
            case 'm':
                config->max_milestone = atoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                return false;
        }
    }

    if (config->trace_file_count < 1) {
        fprintf(stderr, "Error: At least one trace file (-f) must be specified.\n");
        return false;
    }
    if (config->cache_size_kb < 8 || config->cache_size_kb > 8192 || !is_power_of_two(config->cache_size_kb)) {
        fprintf(stderr, "Error: Cache size (-s) must be a power of 2 between 8 and 8192 KB.\n");
        return false;
    }
    // Updated block size validation to permit 4 through 64 (all powers of 2)
    if (config->block_size_bytes < 4 || config->block_size_bytes > 64 || !is_power_of_two(config->block_size_bytes)) {
        fprintf(stderr, "Error: Block size (-b) must be 4, 8, 16, 32, or 64 bytes.\n");
        return false;
    }
    if (config->associativity < 1 || config->associativity > 16 || !is_power_of_two(config->associativity)) {
        fprintf(stderr, "Error: Associativity (-a) must be 1, 2, 4, 8, or 16.\n");
        return false;
    }
    if (strcasecmp(config->replacement_policy, "RR") != 0 &&
        strcasecmp(config->replacement_policy, "RND") != 0) {
        fprintf(stderr, "Error: Replacement policy (-r) must be RR or RND.\n");
        return false;
    }
    if (config->phys_memory_mb < 128 || config->phys_memory_mb > 4096 || !is_power_of_two(config->phys_memory_mb)) {
        fprintf(stderr, "Error: Physical memory (-p) must be a power of 2 between 128 and 4096 MB.\n");
        return false;
    }
    if (config->os_mem_percent < 0.0 || config->os_mem_percent > 100.0) {
        fprintf(stderr, "Error: OS memory percent (-u) must be between 0 and 100.\n");
        return false;
    }

    return true;
}

#endif