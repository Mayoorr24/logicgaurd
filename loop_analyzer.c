#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "loop_analyzer.h"
#include "hashtable.h"

#define MAX_PIECES 4

static int is_id_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

/* Direction of change of `var` on one line: 1 up, -1 down, 0 none */
static int line_direction(const char *line, const char *var) {
    size_t len = strlen(var);
    const char *p = line;

    while ((p = strstr(p, var)) != NULL) {
        int before_ok = (p == line) || !is_id_char(*(p - 1));
        int after_ok = !is_id_char(p[len]);

        if (before_ok && after_ok) {
            /* prefix ++i / --i */
            if (p - line >= 2 && p[-2] == '+' && p[-1] == '+') return 1;
            if (p - line >= 2 && p[-2] == '-' && p[-1] == '-') return -1;

            const char *q = p + len;
            while (*q == ' ') q++;

            if (q[0] == '+' && q[1] == '+') return 1;    /* i++  */
            if (q[0] == '-' && q[1] == '-') return -1;   /* i--  */
            if (q[0] == '+' && q[1] == '=') return 1;    /* i += */
            if (q[0] == '*' && q[1] == '=') return 1;    /* i *= */
            if (q[0] == '-' && q[1] == '=') return -1;   /* i -= */
            if (q[0] == '/' && q[1] == '=') return -1;   /* i /= */

            /* i = i + x, i = i * x, i = i - x, i = i / x */
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

/* 1 = condition stays true forever, 0 = will become false, -1 = unknown */
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
    /* == and != */
    return (dir != 0) ? 0 : -1;
}

/* Combine parts. AND: ends when ANY part fails. OR: ends when ALL parts fail. */
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

        printf("\nLoop found: Line %d to %d\n", i + 1, close_line + 1);
        printf("  Condition: %s\n", trim(cond));

        if (has_and && has_or) {
            printf("  INFO: mixed && and || - unable to analyse\n");
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
        int parsed_ok = 1;

        for (int k = 0; k < np; k++) {
            char var[30], op[3];
            char *piece = trim(pieces[k]);

            if (sscanf(piece, " %29[^ <>=!] %2[<>=!]", var, op) != 2) {
                printf("  Part %d '%s': cannot parse\n", k + 1, piece);
                status[k] = -1;
                parsed_ok = 0;
                continue;
            }
            int dir = track_variable_direction(lines, i + 1, close_line, var);
            status[k] = piece_status(op, dir);
            printf("  Part %d '%s': %s is %s -> %s\n", k + 1, piece, var,
                   dir == 1 ? "increasing" : dir == -1 ? "decreasing" : "not changed",
                   status[k] == 1 ? "stuck" : status[k] == 0 ? "progresses" : "unknown");
        }

        int has_break = scan_for_break(lines, i + 1, close_line);
        printf("  Has break/continue: %s\n", has_break ? "yes" : "no");

        int verdict = combine(status, np, has_or);
        (void)parsed_ok;

        if (verdict == 1 && !has_break)
            printf("  CRITICAL: Likely infinite loop\n");
        else if (verdict == 1 && has_break)
            printf("  INFO: Loop looks stuck but has break/continue - risk downgraded\n");
        else if (verdict == 0)
            printf("  OK: Loop appears safe\n");
        else
            printf("  INFO: Could not determine (not claiming safe)\n");
    }
}