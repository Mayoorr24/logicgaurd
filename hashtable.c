#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

VarState *table[TABLE_SIZE];

unsigned int hash(char *str) {
    unsigned int h = 5381;
    while (*str) {
        h = ((h << 5) + h) + *str++;
    }
    return h % TABLE_SIZE;
}

void insert_var(char *name, char *type, long value) {
    unsigned int idx = hash(name);
    VarState *node = malloc(sizeof(VarState));
    strcpy(node->name, name);
    strcpy(node->type, type);
    node->value = value;
    node->next = table[idx];
    table[idx] = node;
}

VarState* lookup_var(char *name) {
    unsigned int idx = hash(name);
    VarState *curr = table[idx];
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

long get_type_max(char *type) {
    if (strcmp(type, "short") == 0) return 32767;
    if (strcmp(type, "int") == 0) return 2147483647;
    if (strcmp(type, "char") == 0) return 127;
    if (strcmp(type, "long") == 0) return 2147483647;
    return -1;
}