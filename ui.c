#include <stdio.h>
#include "ui.h"

void print_header(const char *filename) {
    printf("\033[36m");
    printf("+==============================================================+\n");
    printf("|              LOGICAL ERROR DETECTOR FOR C                     |\n");
    printf("|        A Static Code Analysis Tool for C Programs             |\n");
    printf("+==============================================================+\n");
    printf("| Detects: Infinite Loops | Unreachable Code | Integer Overflow |\n");
    printf("+==============================================================+\n");
    printf("\033[0m");
    if (filename != NULL) {
        printf("File: %s\n\n", filename);
    }
}

void print_section_title(const char *title) {
    printf("\n\033[33m--- %s ---\033[0m\n", title);
}