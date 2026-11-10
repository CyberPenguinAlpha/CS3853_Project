#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int length;          // Decimal instruction length from EIP (XX)
    uint32_t eip;        // Instruction fetch virtual address
    uint32_t dstM;       // Destination write virtual address
    bool dstM_valid;     // True if dstM != 0 and data != "--------"
    uint32_t srcM;       // Source read virtual address
    bool srcM_valid;     // True if srcM != 0 and data != "--------"
} TraceInstruction;

static inline bool read_next_instruction(FILE *fp, TraceInstruction *instr) {
    char line1[512];
    char line2[512];

    while (fgets(line1, sizeof(line1), fp) != NULL) {
        if (strncmp(line1, "EIP", 3) != 0) {
            continue;
        }

        if (fgets(line2, sizeof(line2), fp) == NULL) {
            return false;
        }

        memset(instr, 0, sizeof(TraceInstruction));

        if (sscanf(line1, "EIP (%d): %x", &instr->length, &instr->eip) != 2) {
            continue;
        }

        char dst_data[32] = {0};
        char src_data[32] = {0};
        if (sscanf(line2, "dstM: %x %31s srcM: %x %31s",
                   &instr->dstM, dst_data, &instr->srcM, src_data) != 4) {
            continue;
        }

        instr->dstM_valid = (instr->dstM != 0) && (strncmp(dst_data, "---", 3) != 0);
        instr->srcM_valid = (instr->srcM != 0) && (strncmp(src_data, "---", 3) != 0);

        return true;
    }

    return false;
}

#endif