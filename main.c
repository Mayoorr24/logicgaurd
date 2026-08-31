#include <stdio.h>
#include "parser.h"
#include "hashtable.h"

int main() {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int total_lines = load_file("test.c", lines);

    printf("Total lines read: %d\n\n", total_lines);

    extract_variable_declarations(lines, total_lines);

    // Confirm the hash table actually stored it correctly
    VarState *v = lookup_var("i");
    if (v != NULL) {
        printf("\nLookup check -> Found 'i': type=%s, value=%ld\n", v->type, v->value);
    } else {
        printf("\nLookup check -> 'i' not found\n");
    }

    return 0;
}