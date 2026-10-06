#ifndef REPORT_H
#define REPORT_H

typedef enum { INFO, WARNING, CRITICAL } Severity;

typedef struct FindingNode {
    int line_number;
    Severity severity;
    char message[200];
    struct FindingNode *left;
    struct FindingNode *right;
} FindingNode;

void insert_finding(int line, Severity severity, const char *message);
void print_report(void);
void save_report_to_file(const char *filename);
int has_findings(void);

#endif