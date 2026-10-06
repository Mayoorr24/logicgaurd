#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "hashtable.h"
#include "unreachable.h"
#include "loop_analyzer.h"
#include "report.h"
#include "trie.h"

void run_analysis(const char *filename) {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int total_lines = load_file(filename, lines);

    if (total_lines == 0) {
        printf("File appears empty or could not be read properly.\n");
        return;
    }

    if (verbose_mode) {
        printf("\nTotal lines read: %d\n\n", total_lines);
        printf("--- Variable Detection ---\n");
    }
    extract_variable_declarations(lines, total_lines);

    if (verbose_mode) {
        printf("\n--- Keyword Scan (Trie-based) ---\n");
        TrieNode *keyword_trie = build_keyword_trie();
        char found[30];
        for (int i = 0; i < total_lines; i++) {
            if (line_contains_keyword(keyword_trie, lines[i], found)) {
                printf("  Line %d: keyword '%s' found\n", i + 1, found);
            }
        }
    }

    if (verbose_mode) printf("\n--- Unreachable Code Check ---\n");
    check_unreachable_code(lines, total_lines);

    if (verbose_mode) printf("\n--- Loop Analysis ---\n");
    extract_loops(lines, total_lines);
    extract_for_loops(lines, total_lines);
    extract_do_while_loops(lines, total_lines);

    print_report();

    if (has_findings()) {
        char save_choice;
        printf("\nSave this report to a file? (y/n): ");
        scanf(" %c", &save_choice);
        if (save_choice == 'y' || save_choice == 'Y') {
            save_report_to_file("report.txt");
        }
    }
}

int main() {
    int choice;
    char filename[100];

    while (1) {
        printf("\n========================================\n");
        printf("   Logical Error Detector for C           \n");
        printf("========================================\n");
        printf("1. Analyze a C file (summary only)\n");
        printf("2. Analyze a C file (detailed trace)\n");
        printf("3. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }

        if (choice == 1 || choice == 2) {
            verbose_mode = (choice == 2) ? 1 : 0;
            printf("Enter the C file name to analyze (e.g., test.c): ");
            scanf("%99s", filename);
            run_analysis(filename);
        }
        else if (choice == 3) {
            printf("Exiting Logical Error Detector. Goodbye!\n");
            break;
        }
        else {
            printf("Invalid choice. Please select 1, 2, or 3.\n");
        }
    }

    return 0;
}