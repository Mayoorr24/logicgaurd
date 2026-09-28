#include <stdio.h>
#include <string.h>
#include "unreachable.h"

void check_unreachable_code(char lines[][MAX_LINE_LEN], int n) {
    int unreachable_flag = 0;

    for (int i = 0; i < n; i++) {

        if (unreachable_flag) {
            if (strstr(lines[i], "}") != NULL) {
                unreachable_flag = 0;   // block closed, reachable again
            }
            else if (strlen(lines[i]) > 2) {   // skip blank/near-empty lines
                printf("WARNING (Line %d): Unreachable code detected -> %s",
                       i + 1, lines[i]);
            }
        }

        if (strstr(lines[i], "return") != NULL ||
            strstr(lines[i], "break") != NULL ||
            strstr(lines[i], "continue") != NULL) {
            unreachable_flag = 1;
        }
    }
}