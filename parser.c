#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "hashtable.h"

int load_file(const char *filename, char lines[][MAX_LINE_LEN]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: could not open file %s\n", filename);
        exit(1);
    }

    int count = 0;
    while (fgets(lines[count], MAX_LINE_LEN, fp) && count < MAX_LINES) {
        count++;
    }

    fclose(fp);
    return count;
}

void extract_variable_declarations(char lines[][MAX_LINE_LEN], int n) {
    char type[10];
    char name[30];
    long value;

    for (int i = 0; i < n; i++) {
        // Try to match: "int x = 5;" or "short count = 32760;"
        int matched = sscanf(lines[i], "%9s %29[^= ] = %ld", type, name, &value);

        if (matched == 3) {
            // Only accept if it's a recognized C type
            if (strcmp(type, "int") == 0 ||
                strcmp(type, "short") == 0 ||
                strcmp(type, "char") == 0 ||
                strcmp(type, "long") == 0) {

                insert_var(name, type, value);
                printf("Detected variable: %s (%s) = %ld  [Line %d]\n",
                       name, type, value, i + 1);
            }
        }
    }
}