#ifndef MILESTONE1_H
#define MILESTONE1_H

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include "config.h"

static inline void run_milestone_1(const SimConfig *config) {
    long long cache_size_bytes = (long long)config->cache_size_kb * 1024LL;
    long long total_blocks = cache_size_bytes / config->block_size_bytes;
    long long total_rows = total_blocks / config->associativity;

    int index_size = (int)round(log2((double)total_rows));
    int offset_size = (int)round(log2((double)config->block_size_bytes));

    long long phys_mem_bytes = (long long)config->phys_memory_mb * 1024LL * 1024LL;
    int phys_mem_bits = (int)round(log2((double)phys_mem_bytes));

    int tag_size = phys_mem_bits - index_size - offset_size;

    int overhead_bits_per_block = tag_size + 1;
    long long overhead_size_bytes = (total_blocks * overhead_bits_per_block) / 8LL;

    long long impl_mem_size_bytes = cache_size_bytes + overhead_size_bytes;
    double impl_mem_size_kb = (double)impl_mem_size_bytes / 1024.0;
    double cost = impl_mem_size_kb * 0.07;

    long long phys_pages = phys_mem_bytes / 4096LL;
    double system_fraction = config->os_mem_percent / 100.0;
    long long system_pages = (long long)llround((double)phys_pages * system_fraction);

    int phys_page_bits = (int)round(log2((double)phys_pages));
    int pte_size_bits = 1 + phys_page_bits;

    long long page_table_ram_bytes = (524288LL * config->trace_file_count * pte_size_bits) / 8LL;

    const char *rep_policy = (strcasecmp(config->replacement_policy, "RR") == 0) ? "Round Robin" : "Random";

    printf("Cache Simulator - CS 3853 - Team #%s\n\n", TEAM_NUMBER);
    printf("Trace File(s):\n");
    for (int i = 0; i < config->trace_file_count; i++) {
        printf("        %s\n", config->trace_files[i]);
    }

    printf("\n***** Cache Input Parameters *****\n\n");
    printf("%-32s%d KB\n", "Cache Size:", config->cache_size_kb);
    printf("%-32s%d bytes\n", "Block Size:", config->block_size_bytes);
    printf("%-32s%d\n", "Associativity:", config->associativity);
    printf("%-32s%s\n", "Replacement Policy:", rep_policy);
    printf("%-32s%d MB\n", "Physical Memory:", config->phys_memory_mb);
    printf("%-32s%.1f%%\n", "Percent Memory Used by System:", config->os_mem_percent);
    
    // Present as MAX if time slice is -1
    if (config->time_slice == -1) {
        printf("%-32sMAX\n", "Instructions / Time Slice:");
    } else {
        printf("%-32s%d\n", "Instructions / Time Slice:", config->time_slice);
    }

    printf("\n***** Cache Calculated Values *****\n\n");
    printf("%-32s%lld\n", "Total # Blocks:", total_blocks);
    printf("%-32s%d bits\n", "Tag Size:", tag_size);
    printf("%-32s%d bits\n", "Index Size:", index_size);
    printf("%-32s%lld\n", "Total # Rows:", total_rows);
    printf("%-32s%lld bytes\n", "Overhead Size:", overhead_size_bytes);
    printf("%-32s%.2f KB   (%lld bytes)\n", "Implementation Memory Size:", impl_mem_size_kb, impl_mem_size_bytes);
    printf("%-32s$%.2f @ $0.07 per KB\n", "Cost:", cost);

    printf("\n***** Physical Memory Calculated Values *****\n\n");
    printf("%-32s%lld\n", "Number of Physical Pages:", phys_pages);
    printf("%-32s%lld\n", "Number of Pages for System:", system_pages);
    printf("%-32s%d bits\n", "Size of Page Table Entry:", pte_size_bits);
    printf("%-32s%lld bytes\n", "Total RAM for Page Table(s):", page_table_ram_bytes);
}

#endif