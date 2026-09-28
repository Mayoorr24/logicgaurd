#ifndef LOOP_ANALYZER_H
#define LOOP_ANALYZER_H

#include "parser.h"

typedef struct {
    int start_line;
    int end_line;
    char loop_var[30];
    char condition_op[3];
    long condition_bound;
    int has_break;
    int direction;
} LoopInfo;

void extract_loops(char lines[][MAX_LINE_LEN], int n);

#endif