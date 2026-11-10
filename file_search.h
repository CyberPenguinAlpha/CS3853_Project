#ifndef FILE_SEARCH_H
#define FILE_SEARCH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

// Recursively searches subdirectories for a given filename without truncation warnings
static inline bool find_file_recursive(const char *base_path, const char *filename, char *found_path, size_t max_len) {
    char path[4096]; // Expanded buffer to prevent truncation warnings
    struct dirent *dp;
    DIR *dir = opendir(base_path);

    if (!dir) {
        return false;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            int written = snprintf(path, sizeof(path), "%s/%s", base_path, dp->d_name);
            if (written < 0 || (size_t)written >= sizeof(path)) {
                continue; // Skip if path formatting fails or overflows
            }

            struct stat statbuf;
            if (stat(path, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    if (find_file_recursive(path, filename, found_path, max_len)) {
                        closedir(dir);
                        return true;
                    }
                } else if (S_ISREG(statbuf.st_mode)) {
                    if (strcmp(dp->d_name, filename) == 0) {
						snprintf(found_path, max_len, "%.*s", (int)(max_len - 1), path);
                        closedir(dir);
                        return true;
                    }
                }
            }
        }
    }
    closedir(dir);
    return false;
}

#endif