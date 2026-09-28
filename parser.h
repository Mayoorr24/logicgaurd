#ifndef PARSER_H
#define PARSER_H

#define MAX_LINES 500
#define MAX_LINE_LEN 200
#define MAX_STACK_DEPTH 100

typedef struct {
    int items[MAX_STACK_DEPTH];
    int top;
} BraceStack;

// File reading
int load_file(const char *filename, char lines[][MAX_LINE_LEN]);

// Variable extraction
void extract_variable_declarations(char lines[][MAX_LINE_LEN], int n);

// Stack operations
void init_stack(BraceStack *s);
void push(BraceStack *s, int value);
int pop(BraceStack *s);
int is_empty(BraceStack *s);

// Brace matching
int find_matching_brace(char lines[][MAX_LINE_LEN], int open_line, int n);

#endif