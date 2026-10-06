#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "report.h"

static FindingNode *root = NULL;

static FindingNode* create_node(int line, Severity severity, const char *message) {
    FindingNode *node = malloc(sizeof(FindingNode));
    node->line_number = line;
    node->severity = severity;
    strncpy(node->message, message, sizeof(node->message) - 1);
    node->message[sizeof(node->message) - 1] = '\0';
    node->left = node->right = NULL;
    return node;
}

static FindingNode* insert_node(FindingNode *node, int line, Severity severity, const char *message) {
    if (node == NULL) {
        return create_node(line, severity, message);
    }
    if (line < node->line_number) {
        node->left = insert_node(node->left, line, severity, message);
    } else {
        node->right = insert_node(node->right, line, severity, message);
    }
    return node;
}

void insert_finding(int line, Severity severity, const char *message) {
    root = insert_node(root, line, severity, message);
}

int has_findings(void) {
    return root != NULL;
}

static const char* severity_color(Severity s) {
    switch (s) {
        case CRITICAL: return "\033[31m";
        case WARNING:  return "\033[33m";
        case INFO:     return "\033[36m";
        default:       return "\033[0m";
    }
}

static const char* severity_label(Severity s) {
    switch (s) {
        case CRITICAL: return "CRITICAL";
        case WARNING:  return "WARNING";
        case INFO:     return "INFO";
        default:       return "UNKNOWN";
    }
}

static void print_inorder(FindingNode *node) {
    if (node == NULL) return;
    print_inorder(node->left);
    printf("%sLine %d [%s]: %s\033[0m\n",
           severity_color(node->severity), node->line_number,
           severity_label(node->severity), node->message);
    print_inorder(node->right);
}

void print_report(void) {
    printf("\n=== Static Analysis Report ===\n\n");
    if (root == NULL) {
        printf("No issues found.\n");
        return;
    }
    print_inorder(root);
    printf("\n=== End of Report ===\n");
}

static void write_inorder_to_file(FindingNode *node, FILE *fp) {
    if (node == NULL) return;
    write_inorder_to_file(node->left, fp);
    fprintf(fp, "Line %d [%s]: %s\n",
            node->line_number, severity_label(node->severity), node->message);
    write_inorder_to_file(node->right, fp);
}

void save_report_to_file(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: could not create report file %s\n", filename);
        return;
    }

    fprintf(fp, "=== Static Analysis Report ===\n\n");
    if (root == NULL) {
        fprintf(fp, "No issues found.\n");
    } else {
        write_inorder_to_file(root, fp);
    }
    fprintf(fp, "\n=== End of Report ===\n");

    fclose(fp);
    printf("\nReport saved to: %s\n", filename);
}