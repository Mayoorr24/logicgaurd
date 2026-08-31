#ifndef PARSER_H
#define PARSER_H

#define MAX_LINES 500
#define MAX_LINE_LEN 200

int load_file(const char *filename, char lines[][MAX_LINE_LEN]);
void extract_variable_declarations(char lines[][MAX_LINE_LEN], int n);

#endif