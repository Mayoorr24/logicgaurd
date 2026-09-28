#include <stdio.h>
#include "parser.h"
#include "hashtable.h"
#include "unreachable.h"
#include "loop_analyzer.h"

int main() {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int total_lines = load_file("test.c", lines);

    printf("Total lines read: %d\n\n", total_lines);

    printf("--- Variable Detection ---\n");
    extract_variable_declarations(lines, total_lines);

    printf("\n--- Unreachable Code Check ---\n");
    check_unreachable_code(lines, total_lines);

    printf("\n--- Loop Analysis ---\n");
    extract_loops(lines, total_lines);

    return 0;
}