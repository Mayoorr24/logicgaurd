#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "hashtable.h"

/* ---------- File Reading ---------- */
int load_file(const char *filename, char lines[][MAX_LINE_LEN]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: could not open file '%s'. Please check the filename and try again.\n", filename);
        return 0;   // return 0 lines instead of killing the whole program
    }

    int count = 0;
    while (count < MAX_LINES && fgets(lines[count], MAX_LINE_LEN, fp)) {
        count++;
    }

    fclose(fp);
    return count;
}

/* ---------- Variable Extraction ---------- */
void extract_variable_declarations(char lines[][MAX_LINE_LEN], int n) {
    char type[10];
    char name[30];
    long value;

    for (int i = 0; i < n; i++) {
        int matched = sscanf(lines[i], "%9s %29[^= ] = %ld", type, name, &value);

        if (matched == 3) {
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

/* ---------- Brace Stack ---------- */
void init_stack(BraceStack *s) {
    s->top = -1;
}

void push(BraceStack *s, int value) {
    if (s->top < MAX_STACK_DEPTH - 1) {
        s->top++;
        s->items[s->top] = value;
    }
}

int pop(BraceStack *s) {
    if (s->top >= 0) {
        int value = s->items[s->top];
        s->top--;
        return value;
    }
    return -1;
}

int is_empty(BraceStack *s) {
    return s->top == -1;
}

/* ---------- Brace Matching ---------- */
int find_matching_brace(char lines[][MAX_LINE_LEN], int open_line, int n) {
    BraceStack stack;
    init_stack(&stack);

    for (int i = open_line; i < n; i++) {
        if (strstr(lines[i], "{") != NULL) {
            push(&stack, i);
        }
        if (strstr(lines[i], "}") != NULL) {
            pop(&stack);
            if (is_empty(&stack)) {
                return i;
            }
        }
    }
    return -1;
}