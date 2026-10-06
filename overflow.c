#include <stdio.h>
#include <string.h>
#include "overflow.h"
#include "hashtable.h"
#include "report.h"

void check_overflow_for_var(char lines[][MAX_LINE_LEN], int start, int end,
                             const char *var_name, int iterations) {
    VarState *v = lookup_var((char *)var_name);
    if (v == NULL) return;

    long type_max = get_type_max(v->type);
    if (type_max == -1) return;

    int dir = 0;
    char inc[40], dec[40];
    sprintf(inc, "%s++", var_name);
    sprintf(dec, "%s--", var_name);

    for (int i = start; i <= end; i++) {
        if (strstr(lines[i], inc)) { dir = 1; break; }
        if (strstr(lines[i], dec)) { dir = -1; break; }
    }

    if (dir == 0) return;

    long predicted_final = v->value + ((long)dir * iterations);

    if (predicted_final > type_max) {
        char msg[200];
        sprintf(msg, "'%s' may exceed its storage limit (max %ld) after this loop - predicted value: %ld",
                v->name, type_max, predicted_final);
        insert_finding(start + 1, CRITICAL, msg);
    }
}