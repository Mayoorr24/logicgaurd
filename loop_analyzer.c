#include <stdio.h>
#include <string.h>
#include "loop_analyzer.h"
#include "hashtable.h"

int scan_for_break(char lines[][MAX_LINE_LEN], int start, int end) {
    for (int i = start; i <= end; i++) {
        if (strstr(lines[i], "break") != NULL || strstr(lines[i], "continue") != NULL) {
            return 1;
        }
    }
    return 0;
}

int track_variable_direction(char lines[][MAX_LINE_LEN], int start, int end, char *var_name) {
    char pattern_inc[40], pattern_dec[40];
    sprintf(pattern_inc, "%s++", var_name);
    sprintf(pattern_dec, "%s--", var_name);

    for (int i = start; i <= end; i++) {
        if (strstr(lines[i], pattern_inc) != NULL) {
            return 1;
        }
        if (strstr(lines[i], pattern_dec) != NULL) {
            return -1;
        }
    }
    return 0;
}

void extract_loops(char lines[][MAX_LINE_LEN], int n) {
    for (int i = 0; i < n; i++) {

        if (strstr(lines[i], "while") != NULL) {
            char var_name[30];
            char op[3];
            long bound;

            int matched = sscanf(lines[i], " while (%29[^ <>=!] %2[<>=!] %ld)", var_name, op, &bound);

            if (matched == 3) {
                int close_line = find_matching_brace(lines, i, n);

                if (close_line != -1) {
                    int has_break = scan_for_break(lines, i, close_line);
                    int direction = track_variable_direction(lines, i, close_line, var_name);

                    printf("\nLoop found: Line %d to %d\n", i + 1, close_line + 1);
                    printf("  Variable: %s | Operator: %s | Bound: %ld\n", var_name, op, bound);
                    printf("  Direction: %s\n", direction == 1 ? "increasing" :
                                                  direction == -1 ? "decreasing" : "unknown");
                    printf("  Has break/continue: %s\n", has_break ? "yes" : "no");

                    if (!has_break) {
                        if (strcmp(op, "<") == 0 && direction == -1) {
                            printf("  WARNING CRITICAL: Likely infinite loop - variable decreasing, condition needs increase\n");
                        }
                        else if (strcmp(op, ">") == 0 && direction == 1) {
                            printf("  WARNING CRITICAL: Likely infinite loop - variable increasing, condition needs decrease\n");
                        }
                        else {
                            printf("  OK: Loop appears safe\n");
                        }
                    } else {
                        printf("  INFO: Loop has break/continue - infinite loop risk downgraded\n");
                    }
                }
            }
        }
    }
}