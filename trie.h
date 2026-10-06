#ifndef TRIE_H
#define TRIE_H

typedef struct TrieNode {
    struct TrieNode *children[26];
    int is_end_of_word;
    char keyword[20];
} TrieNode;

TrieNode* trie_create_node(void);
void trie_insert(TrieNode *root, const char *word);
int trie_search(TrieNode *root, const char *word);
TrieNode* build_keyword_trie(void);
int line_contains_keyword(TrieNode *root, const char *line, char *found_keyword);

#endif