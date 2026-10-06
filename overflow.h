#ifndef OVERFLOW_H
#define OVERFLOW_H

#include "parser.h"

void check_overflow_for_var(char lines[][MAX_LINE_LEN], int start, int end,
                             const char *var_name, int iterations);

#endif