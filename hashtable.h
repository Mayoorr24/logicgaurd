#ifndef HASHTABLE_H
#define HASHTABLE_H

#define TABLE_SIZE 101

typedef struct VarState {
    char name[30];
    char type[10];
    long value;
    struct VarState *next;
} VarState;

void insert_var(char *name, char *type, long value);
VarState* lookup_var(char *name);
unsigned int hash(char *str);

#endif