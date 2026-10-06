#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "loop_analyzer.h"
#include "hashtable.h"
#include "overflow.h"
#include "report.h"

#define MAX_PIECES 4

int verbose_mode = 0;

static int is_id_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static int line_direction(const char *line, const char *var) {
    size_t len = strlen(var);
    const char *p = line;

    while ((p = strstr(p, var)) != NULL) {
        int before_ok = (p == line) || !is_id_char(*(p - 1));
        int after_ok = !is_id_char(p[len]);

        if (before_ok && after_ok) {

            if (p - line >= 2 && p[-2] == '+' && p[-1] == '+') return 1;
            if (p - line >= 2 && p[-2] == '-' && p[-1] == '-') return -1;

            const char *q = p + len;
            while (*q == ' ') q++;

            if (q[0] == '+' && q[1] == '+') return 1;
            if (q[0] == '-' && q[1] == '-') return -1;
            if (q[0] == '+' && q[1] == '=') return 1;
            if (q[0] == '*' && q[1] == '=') return 1;
            if (q[0] == '-' && q[1] == '=') return -1;
            if (q[0] == '/' && q[1] == '=') return -1;

            if (q[0] == '=' && q[1] != '=') {
                q++;
                while (*q == ' ') q++;
                if (strncmp(q, var, len) == 0 && !is_id_char(q[len])) {
                    q += len;
                    while (*q == ' ') q++;
                    if (*q == '+' || *q == '*') return 1;
                    if (*q == '-' || *q == '/') return -1;
                }
            }
        }
        p += len;
    }
    return 0;
}

static int track_variable_direction(char lines[][MAX_LINE_LEN], int start, int end, const char *var) {
    for (int i = start; i <= end; i++) {
        int d = line_direction(lines[i], var);
        if (d != 0) return d;
    }
    return 0;
}

static int scan_for_break(char lines[][MAX_LINE_LEN], int start, int end) {
    for (int i = start; i <= end; i++) {
        if (strstr(lines[i], "break") != NULL || strstr(lines[i], "continue") != NULL)
            return 1;
    }
    return 0;
}

static int piece_status(const char *op, int dir) {
    if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0) {
        if (dir == -1) return 1;
        if (dir == 1) return 0;
        return -1;
    }
    if (strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        if (dir == 1) return 1;
        if (dir == -1) return 0;
        return -1;
    }
    return (dir != 0) ? 0 : -1;
}

static int combine(int st[], int n, int is_or) {
    int any_stuck = 0, any_prog = 0, all_stuck = 1, all_prog = 1;
    for (int i = 0; i < n; i++) {
        if (st[i] == 1)      { any_stuck = 1; all_prog = 0; }
        else if (st[i] == 0) { any_prog = 1;  all_stuck = 0; }
        else                 { all_stuck = 0; all_prog = 0; }
    }
    if (!is_or) {
        if (all_stuck) return 1;
        if (any_prog) return 0;
        return -1;
    }
    if (any_stuck) return 1;
    if (all_prog) return 0;
    return -1;
}

static char *trim(char *s) {
    while (*s == ' ') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\n')) s[--n] = '\0';
    return s;
}

static int is_entry_condition_false(const char *var, const char *op, long bound) {
    VarState *vstart = lookup_var((char *)var);
    if (vstart == NULL) return 0;

    long start_val = vstart->value;

    if (strcmp(op, "<") == 0  && !(start_val < bound))  return 1;
    if (strcmp(op, "<=") == 0 && !(start_val <= bound)) return 1;
    if (strcmp(op, ">") == 0  && !(start_val > bound))  return 1;
    if (strcmp(op, ">=") == 0 && !(start_val >= bound)) return 1;

    return 0;
}

/* ---------- while-loop analysis ---------- */
void extract_loops(char lines[][MAX_LINE_LEN], int n) {
    for (int i = 0; i < n; i++) {
        if (strstr(lines[i], "while") == NULL || strstr(lines[i], "{") == NULL)
            continue;

        char *open = strchr(lines[i], '(');
        char *close = strrchr(lines[i], ')');
        if (!open || !close || close < open) continue;

        char cond[100];
        int clen = (int)(close - open) - 1;
        if (clen <= 0 || clen >= (int)sizeof(cond)) continue;
        strncpy(cond, open + 1, clen);
        cond[clen] = '\0';

        int has_and = strstr(cond, "&&") != NULL;
        int has_or = strstr(cond, "||") != NULL;

        int close_line = find_matching_brace(lines, i, n);
        if (close_line == -1) continue;

        if (verbose_mode) {
            printf("\nLoop found: Line %d to %d\n", i + 1, close_line + 1);
            printf("  Condition: %s\n", trim(cond));
        }

        if (has_and && has_or) {
            if (verbose_mode) printf("  INFO: mixed && and || - unable to analyse\n");
            insert_finding(i + 1, INFO, "This loop's condition mixes AND and OR, so it could not be fully analyzed");
            continue;
        }

        const char *conn = has_or ? "||" : "&&";
        char *pieces[MAX_PIECES];
        int np = 0;
        char *cur = cond;
        while (np < MAX_PIECES) {
            pieces[np++] = cur;
            char *s = strstr(cur, conn);
            if (!s) break;
            *s = '\0';
            cur = s + 2;
        }

        int status[MAX_PIECES];
        char piece_vars[MAX_PIECES][30];
        long piece_bounds[MAX_PIECES];
        int piece_dirs[MAX_PIECES];

        for (int k = 0; k < np; k++) {
            char var[30], op[3];
            long bound = 0;
            char *piece = trim(pieces[k]);

            int matched = sscanf(piece, " %29[^ <>=!] %2[<>=!] %ld", var, op, &bound);

            if (matched < 2) {
                if (verbose_mode) printf("  Part %d '%s': cannot parse\n", k + 1, piece);
                status[k] = -1;
                piece_vars[k][0] = '\0';
                piece_dirs[k] = 0;
                continue;
            }

            if (matched == 3 && is_entry_condition_false(var, op, bound)) {
                if (verbose_mode) {
                    printf("  Part %d '%s': condition is false on entry - loop body never runs\n", k + 1, piece);
                }
                status[k] = 0;
                piece_vars[k][0] = '\0';
                piece_dirs[k] = 0;
                continue;
            }

            int dir = track_variable_direction(lines, i + 1, close_line, var);
            status[k] = piece_status(op, dir);

            strcpy(piece_vars[k], var);
            piece_bounds[k] = bound;
            piece_dirs[k] = dir;

            if (verbose_mode) {
                printf("  Part %d '%s': %s is %s -> %s\n", k + 1, piece, var,
                       dir == 1 ? "increasing" : dir == -1 ? "decreasing" : "not changed",
                       status[k] == 1 ? "stuck" : status[k] == 0 ? "progresses" : "unknown");
            }
        }

        int has_break = scan_for_break(lines, i + 1, close_line);
        if (verbose_mode) printf("  Has break/continue: %s\n", has_break ? "yes" : "no");

        int verdict = combine(status, np, has_or);

        if (verdict == 1 && !has_break) {
            if (verbose_mode) printf("  CRITICAL: Likely infinite loop\n");
            insert_finding(i + 1, CRITICAL,
                "This loop may never end - its variable moves away from the value needed to stop it");
        }
        else if (verdict == 1 && has_break) {
            if (verbose_mode) printf("  INFO: Loop looks stuck but has break/continue - risk downgraded\n");
            insert_finding(i + 1, INFO,
                "This loop looks risky, but it has a break/continue that likely lets it exit safely");
        }
        else if (verdict == 0) {
            if (verbose_mode) printf("  OK: Loop appears safe\n");
        }
        else {
            if (verbose_mode) printf("  INFO: Could not determine (not claiming safe)\n");
            insert_finding(i + 1, INFO,
                "Could not determine if this loop is safe - its variable's behavior is unclear");
        }

        for (int k = 0; k < np; k++) {
            if (piece_vars[k][0] != '\0' && piece_dirs[k] != 0) {
                VarState *vk = lookup_var(piece_vars[k]);
                if (vk) {
                    long diff = (piece_dirs[k] == 1) ? (piece_bounds[k] - vk->value)
                                                      : (vk->value - piece_bounds[k]);
                    int iterations = (diff > 0) ? (int)diff : 0;
                    if (iterations > 0) {
                        check_overflow_for_var(lines, i + 1, close_line, piece_vars[k], iterations);
                    }
                }
            }
        }
    }
}

/* ---------- for-loop analysis ---------- */
void extract_for_loops(char lines[][MAX_LINE_LEN], int n) {
    for (int i = 0; i < n; i++) {
        if (strstr(lines[i], "for") == NULL || strstr(lines[i], "{") == NULL)
            continue;

        char *for_kw = strstr(lines[i], "for");
        if (for_kw != lines[i] && is_id_char(*(for_kw - 1))) continue;

        char *open = strchr(lines[i], '(');
        char *close = strrchr(lines[i], ')');
        if (!open || !close || close < open) continue;

        char header[150];
        int hlen = (int)(close - open) - 1;
        if (hlen <= 0 || hlen >= (int)sizeof(header)) continue;
        strncpy(header, open + 1, hlen);
        header[hlen] = '\0';

        char *init_part = header;
        char *cond_part = strchr(header, ';');
        if (!cond_part) continue;
        *cond_part = '\0';
        cond_part++;

        char *incr_part = strchr(cond_part, ';');
        if (!incr_part) continue;
        *incr_part = '\0';
        incr_part++;

        char var[30], op[3];
        long bound = 0;
        char *cond_trimmed = trim(cond_part);

        int matched = sscanf(cond_trimmed, " %29[^ <>=!] %2[<>=!] %ld", var, op, &bound);
        if (matched < 3) continue;

        int close_line = find_matching_brace(lines, i, n);
        if (close_line == -1) continue;

        if (verbose_mode) {
            printf("\nFor-loop found: Line %d to %d\n", i + 1, close_line + 1);
            printf("  Init: %s | Condition: %s | Increment: %s\n",
                   trim(init_part), cond_trimmed, trim(incr_part));
        }

        if (is_entry_condition_false(var, op, bound)) {
            if (verbose_mode) printf("  INFO: condition is false on entry - loop body never runs\n");
            continue;
        }

        int dir = line_direction(incr_part, var);

        if (dir == 0) {
            dir = track_variable_direction(lines, i + 1, close_line, var);
        }

        int stat = piece_status(op, dir);
        int has_break = scan_for_break(lines, i + 1, close_line);

        if (verbose_mode) {
            printf("  Variable '%s' is %s -> %s\n", var,
                   dir == 1 ? "increasing" : dir == -1 ? "decreasing" : "not changed",
                   stat == 1 ? "stuck" : stat == 0 ? "progresses" : "unknown");
            printf("  Has break/continue: %s\n", has_break ? "yes" : "no");
        }

        if (stat == 1 && !has_break) {
            if (verbose_mode) printf("  CRITICAL: Likely infinite loop\n");
            insert_finding(i + 1, CRITICAL,
                "This for-loop may never end - its variable moves away from the value needed to stop it");
        }
        else if (stat == 1 && has_break) {
            if (verbose_mode) printf("  INFO: Loop looks stuck but has break/continue - risk downgraded\n");
            insert_finding(i + 1, INFO,
                "This for-loop looks risky, but it has a break/continue that likely lets it exit safely");
        }
        else if (stat == 0) {
            if (verbose_mode) printf("  OK: Loop appears safe\n");
        }
        else {
            if (verbose_mode) printf("  INFO: Could not determine (not claiming safe)\n");
        }

        if (dir != 0) {
            VarState *vk = lookup_var(var);
            if (vk) {
                long diff = (dir == 1) ? (bound - vk->value) : (vk->value - bound);
                int iterations = (diff > 0) ? (int)diff : 0;
                if (iterations > 0) {
                    check_overflow_for_var(lines, i + 1, close_line, var, iterations);
                }
            }
        }
    }
}

/* ---------- do-while loop analysis ---------- */
void extract_do_while_loops(char lines[][MAX_LINE_LEN], int n) {
    for (int i = 0; i < n; i++) {
        char *trimmed_start = lines[i];
        while (*trimmed_start == ' ' || *trimmed_start == '\t') trimmed_start++;

        /* must start with "do" as a whole word, and the line must contain '{' */
        if (strncmp(trimmed_start, "do", 2) != 0) continue;
        if (is_id_char(trimmed_start[2])) continue;   // reject "double", "download", etc.
        if (strstr(lines[i], "{") == NULL) continue;

        int close_line = find_matching_brace(lines, i, n);
        if (close_line == -1) continue;

        /* the while(...) condition should appear on the same line as the closing brace,
           or on the line right after it */
        int cond_line = close_line;
        char *open = strchr(lines[cond_line], '(');
        char *close = strrchr(lines[cond_line], ')');

        if (!open || !close || close < open || strstr(lines[cond_line], "while") == NULL) {
            /* try the next line instead */
            if (close_line + 1 < n) {
                cond_line = close_line + 1;
                open = strchr(lines[cond_line], '(');
                close = strrchr(lines[cond_line], ')');
            }
        }

        if (!open || !close || close < open || strstr(lines[cond_line], "while") == NULL) {
            continue;   // couldn't find a matching while(...) condition
        }

        char cond[100];
        int clen = (int)(close - open) - 1;
        if (clen <= 0 || clen >= (int)sizeof(cond)) continue;
        strncpy(cond, open + 1, clen);
        cond[clen] = '\0';

        char var[30], op[3];
        long bound = 0;
        char *cond_trimmed = trim(cond);
        int matched = sscanf(cond_trimmed, " %29[^ <>=!] %2[<>=!] %ld", var, op, &bound);
        if (matched < 3) continue;

        if (verbose_mode) {
            printf("\nDo-while loop found: Line %d to %d\n", i + 1, cond_line + 1);
            printf("  Condition: %s\n", cond_trimmed);
        }

        /* No entry-condition check here - the body always runs at least once */

        int dir = track_variable_direction(lines, i + 1, close_line, var);
        int stat = piece_status(op, dir);
        int has_break = scan_for_break(lines, i + 1, close_line);

        if (verbose_mode) {
            printf("  Variable '%s' is %s -> %s\n", var,
                   dir == 1 ? "increasing" : dir == -1 ? "decreasing" : "not changed",
                   stat == 1 ? "stuck" : stat == 0 ? "progresses" : "unknown");
            printf("  Has break/continue: %s\n", has_break ? "yes" : "no");
        }

        if (stat == 1 && !has_break) {
            if (verbose_mode) printf("  CRITICAL: Likely infinite loop\n");
            insert_finding(i + 1, CRITICAL,
                "This do-while loop may never end - its variable moves away from the value needed to stop it");
        }
        else if (stat == 1 && has_break) {
            if (verbose_mode) printf("  INFO: Loop looks stuck but has break/continue - risk downgraded\n");
            insert_finding(i + 1, INFO,
                "This do-while loop looks risky, but it has a break/continue that likely lets it exit safely");
        }
        else if (stat == 0) {
            if (verbose_mode) printf("  OK: Loop appears safe\n");
        }
        else {
            if (verbose_mode) printf("  INFO: Could not determine (not claiming safe)\n");
        }

        if (dir != 0) {
            VarState *vk = lookup_var(var);
            if (vk) {
                long diff = (dir == 1) ? (bound - vk->value) : (vk->value - bound);
                int iterations = (diff > 0) ? (int)diff : 0;
                if (iterations > 0) {
                    check_overflow_for_var(lines, i + 1, close_line, var, iterations);
                }
            }
        }
    }
}