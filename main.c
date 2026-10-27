#include <stdlib.h>
#include "config.h"
#include "milestone1.h"

int main(int argc, char *argv[]) {
    SimConfig config;

    if (!parse_arguments(argc, argv, &config)) {
        return EXIT_FAILURE;
    }

    run_milestone_1(&config);

    return EXIT_SUCCESS;
}