#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "trie.h"

TrieNode* trie_create_node(void) {
    TrieNode *node = malloc(sizeof(TrieNode));
    for (int i = 0; i < 26; i++) {
        node->children[i] = NULL;
    }
    node->is_end_of_word = 0;
    node->keyword[0] = '\0';
    return node;
}

void trie_insert(TrieNode *root, const char *word) {
    TrieNode *curr = root;
    for (int i = 0; word[i] != '\0'; i++) {
        int idx = word[i] - 'a';
        if (idx < 0 || idx >= 26) return;   // only handles lowercase a-z keywords
        if (curr->children[idx] == NULL) {
            curr->children[idx] = trie_create_node();
        }
        curr = curr->children[idx];
    }
    curr->is_end_of_word = 1;
    strncpy(curr->keyword, word, sizeof(curr->keyword) - 1);
}

int trie_search(TrieNode *root, const char *word) {
    TrieNode *curr = root;
    for (int i = 0; word[i] != '\0'; i++) {
        int idx = word[i] - 'a';
        if (idx < 0 || idx >= 26) return 0;
        if (curr->children[idx] == NULL) return 0;
        curr = curr->children[idx];
    }
    return curr->is_end_of_word;
}

TrieNode* build_keyword_trie(void) {
    TrieNode *root = trie_create_node();
    trie_insert(root, "return");
    trie_insert(root, "break");
    trie_insert(root, "continue");
    trie_insert(root, "while");
    trie_insert(root, "for");
    trie_insert(root, "do");
    return root;
}

/* Scans a line word-by-word, checking each word against the trie.
   Returns 1 and fills found_keyword if any keyword is found, else returns 0. */
int line_contains_keyword(TrieNode *root, const char *line, char *found_keyword) {
    char buffer[200];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, " \t\n(){};,");
    while (token != NULL) {
        char lower[30];
        int i;
        for (i = 0; token[i] != '\0' && i < 29; i++) {
            lower[i] = (char)tolower((unsigned char)token[i]);
        }
        lower[i] = '\0';

        if (trie_search(root, lower)) {
            if (found_keyword != NULL) {
                strcpy(found_keyword, lower);
            }
            return 1;
        }
        token = strtok(NULL, " \t\n(){};,");
    }
    return 0;
}