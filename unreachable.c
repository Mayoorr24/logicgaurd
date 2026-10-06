#include <stdio.h>
#include <string.h>
#include "unreachable.h"
#include "report.h"

void check_unreachable_code(char lines[][MAX_LINE_LEN], int n) {
    int unreachable_flag = 0;

    for (int i = 0; i < n; i++) {

        if (unreachable_flag) {
            if (strstr(lines[i], "}") != NULL) {
                unreachable_flag = 0;
            }
            else if (strlen(lines[i]) > 2) {
                insert_finding(i + 1, WARNING,
                    "This code can never run - it comes right after a return/break/continue");
            }
        }

        if (strstr(lines[i], "return") != NULL ||
            strstr(lines[i], "break") != NULL ||
            strstr(lines[i], "continue") != NULL) {
            unreachable_flag = 1;
        }
    }
}