#ifndef MILESTONE2_H
#define MILESTONE2_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "config.h"
#include "trace_parse.h"
#include "file_search.h"

#define PAGE_TABLE_SIZE 524288 // Fixed 512K entries for 31-bit user virtual space
#define PAGE_SIZE_BYTES 4096   // 4K pages

typedef struct {
    bool valid;          // Currently mapped to a physical page
    bool ever_used;      // True if this PTE was ever mapped during the process's lifetime
    int phys_page;       // Assigned physical page number
} PageTableEntry;

typedef struct {
    bool in_use;
    int owner_proc;      // Process index (0, 1, or 2)
    uint32_t owner_vpn;  // Virtual Page Number (0 to 524287)
} PhysPageFrame;

typedef struct {
    long long system_pages;
    long long user_pages;
    long long virtual_pages_mapped;
    long long page_table_hits;
    long long pages_from_free;
    long long total_page_faults;
    long long used_pte_count[MAX_TRACE_FILES];
    int pte_size_bits;
} VMStats;

static inline void vm_access_address(
    int proc_id,
    uint32_t vaddr,
    PageTableEntry page_tables[MAX_TRACE_FILES][PAGE_TABLE_SIZE],
    PhysPageFrame *phys_frames,
    long long user_pages,
    long long *free_page_count,
    long long *next_victim_frame,
    VMStats *stats
) {
    uint32_t vpn = (vaddr & 0x7FFFFFFFU) >> 12; //mask of all but lower 31, then shift to drop the offset, left with index
    stats->virtual_pages_mapped++;

    PageTableEntry *pte = &page_tables[proc_id][vpn];

    // 1. Page Table Hit
    // If the page is already in memory, no new mapping is needed
    if (pte->valid) {
        stats->page_table_hits++;
        return;
    }

    // Counting each unique virtual page only once 
    if (!pte->ever_used) {
        pte->ever_used = true;
        stats->used_pte_count[proc_id]++;
    }

    //Allocate from Free Physical Pages if available
    if (*free_page_count > 0) {
        for (long long p = 0; p < user_pages; p++) {
            if (!phys_frames[p].in_use) {
                phys_frames[p].in_use = true;
                phys_frames[p].owner_proc = proc_id;
                phys_frames[p].owner_vpn = vpn;

                pte->valid = true;
                pte->phys_page = (int)p;

                (*free_page_count)--;
                stats->pages_from_free++;
                return;
            }
        }
    }

    //Page Fault: No free physical page available; snag one in use
    stats->total_page_faults++;

    // Selecting next physical frame to be replaced
    long long victim = *next_victim_frame;
    *next_victim_frame = (*next_victim_frame + 1) % user_pages;

    //  Clear the old page mapping before reusing the physical frame
    int old_proc = phys_frames[victim].owner_proc;
    uint32_t old_vpn = phys_frames[victim].owner_vpn;
    if (old_proc >= 0 && old_proc < MAX_TRACE_FILES) {
        page_tables[old_proc][old_vpn].valid = false;
        page_tables[old_proc][old_vpn].phys_page = -1;
    }

    // Assigning the reused frame to the current process
    phys_frames[victim].in_use = true;
    phys_frames[victim].owner_proc = proc_id;
    phys_frames[victim].owner_vpn = vpn;

    pte->valid = true;
    pte->phys_page = (int)victim;
}

// Free physical pages when a process finishes.
static inline void vm_free_process_pages(
    int proc_id,
    PageTableEntry page_tables[MAX_TRACE_FILES][PAGE_TABLE_SIZE],
    PhysPageFrame *phys_frames,
    long long user_pages,
    long long *free_page_count
) {
    for (long long p = 0; p < user_pages; p++) {
        if (phys_frames[p].in_use && phys_frames[p].owner_proc == proc_id) {
            uint32_t vpn = phys_frames[p].owner_vpn;
            page_tables[proc_id][vpn].valid = false;
            page_tables[proc_id][vpn].phys_page = -1;

            phys_frames[p].in_use = false;
            phys_frames[p].owner_proc = -1;
            phys_frames[p].owner_vpn = 0;
            (*free_page_count)++;
        }
    }
}
//These string were formatted using Gemini Argon, because I am terrible at formatting strings
static inline void print_milestone_2(const SimConfig *config, const VMStats *stats) {
    printf("\n***** VIRTUAL MEMORY SIMULATION RESULTS *****\n\n");
    printf("%-32s%lld\n", "Physical Pages Used By SYSTEM:", stats->system_pages);
    printf("%-32s%lld\n\n", "Pages Available to User:", stats->user_pages);
    printf("%-32s%lld\n", "Virtual Pages Mapped:", stats->virtual_pages_mapped);
    printf("------------------------------\n");
    printf("        %-24s%lld\n\n", "Page Table Hits:", stats->page_table_hits);
    printf("        %-24s%lld\n\n", "Pages from Free:", stats->pages_from_free);
    printf("        %-24s%lld\n\n\n", "Total Page Faults:", stats->total_page_faults);

    printf("Page Table Usage Per Process:\n");
    printf("------------------------------\n");
    for (int i = 0; i < config->trace_file_count; i++) {
        double pct_used = ((double)stats->used_pte_count[i] / (double)PAGE_TABLE_SIZE) * 100.0;
        
        // Calculate the memory occupied by unused page table entries
        long long wasted_bytes = ((PAGE_TABLE_SIZE - stats->used_pte_count[i]) * (long long)stats->pte_size_bits) / 8LL;

        printf("[%d] %s:\n", i, config->trace_files[i]);
        printf("        Used Page Table Entries: %lld  (%5.2f%%)\n", stats->used_pte_count[i], pct_used);
        printf("        Page Table Wasted: %lld bytes\n\n", wasted_bytes);
    }
}

static inline bool run_milestone_2(const SimConfig *config) {
    VMStats stats;
    memset(&stats, 0, sizeof(VMStats));

    long long phys_mem_bytes = (long long)config->phys_memory_mb * 1024LL * 1024LL;
    long long total_phys_pages = phys_mem_bytes / PAGE_SIZE_BYTES;
    double system_fraction = config->os_mem_percent / 100.0;

    // Reserve system memory and make the remaining pages available to users
    stats.system_pages = (long long)llround((double)total_phys_pages * system_fraction);
    stats.user_pages = total_phys_pages - stats.system_pages;

    // Prevent division by zero when all physical memory is reserved by the system.
    if (stats.user_pages == 0) {
        fprintf(stderr, "\nError - ALL pages tables have zero physical pages.\n");
        return false;
    }

    int phys_page_bits = (int)round(log2((double)total_phys_pages));
    stats.pte_size_bits = 1 + phys_page_bits;

    PageTableEntry (*page_tables)[PAGE_TABLE_SIZE] = calloc(MAX_TRACE_FILES, sizeof(*page_tables));
    PhysPageFrame *phys_frames = calloc(stats.user_pages > 0 ? stats.user_pages : 1, sizeof(PhysPageFrame));
    if (!page_tables || !phys_frames) {
        fprintf(stderr, "Error: Memory allocation failed for Virtual Memory structures.\n");
        free(page_tables);
        free(phys_frames);
        return false;
    }

    FILE *fps[MAX_TRACE_FILES] = {NULL};
    bool active[MAX_TRACE_FILES] = {false};
    int active_count = 0;

    for (int i = 0; i < config->trace_file_count; i++) {
        // 1. Try opening directly in the current directory first
        fps[i] = fopen(config->trace_files[i], "r");

        // 2. If it fails, search all subdirectories recursively using file_search.h
        if (!fps[i]) {
            char resolved_path[1024];
            
            // Extract just the filename/basename in case a path was provided
            const char *basename = strrchr(config->trace_files[i], '/');
            if (!basename) {
                basename = strrchr(config->trace_files[i], '\\');
            }
            basename = basename ? basename + 1 : config->trace_files[i];

            if (find_file_recursive(".", basename, resolved_path, sizeof(resolved_path))) {
                fps[i] = fopen(resolved_path, "r");
            }
        }

        if (!fps[i]) {
            fprintf(stderr, "Error: Unable to open trace file '%s' in current directory or any subdirectories\n", config->trace_files[i]);
            for (int j = 0; j < i; j++) {
                if (fps[j]) fclose(fps[j]);
            }
            free(page_tables);
            free(phys_frames);
            return false;
        }
        active[i] = true;
        active_count++;
    }

    long long free_page_count = stats.user_pages;
    long long next_victim_frame = 0;
    TraceInstruction instr;

    // Process each active trace for its time slice before moving to the next.
    while (active_count > 0) {
        for (int i = 0; i < config->trace_file_count; i++) {
            if (!active[i]) continue;

            int instructions_processed = 0;
            while (config->time_slice == -1 || instructions_processed < config->time_slice) {
                if (read_next_instruction(fps[i], &instr)) {
                    vm_access_address(i, instr.eip, page_tables, phys_frames,
                                      stats.user_pages, &free_page_count, &next_victim_frame, &stats);

                    if (instr.dstM_valid) {
                        vm_access_address(i, instr.dstM, page_tables, phys_frames,
                                          stats.user_pages, &free_page_count, &next_victim_frame, &stats);
                    }

                    if (instr.srcM_valid) {
                        vm_access_address(i, instr.srcM, page_tables, phys_frames,
                                          stats.user_pages, &free_page_count, &next_victim_frame, &stats);
                    }

                    instructions_processed++;
                } else {
                    vm_free_process_pages(i, page_tables, phys_frames, stats.user_pages, &free_page_count);
                    fclose(fps[i]);
                    fps[i] = NULL;
                    active[i] = false;
                    active_count--;
                    break;
                }
            }
        }
    }

    print_milestone_2(config, &stats);

    free(page_tables);
    free(phys_frames);
    return true;
}

#endif