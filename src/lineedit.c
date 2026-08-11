#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "lineedit.h"

/* ------------------------------- history -------------------------------- */

static char **hist = NULL;
static int    hist_len = 0, hist_cap = 0;

void savo_history_add(const char *line) {
    char *copy;
    if (line == NULL || line[0] == '\0') return;
    if (hist_len > 0 && strcmp(hist[hist_len - 1], line) == 0) return;   /* no dupes */
    if (hist_len == hist_cap) {
        int ncap = hist_cap ? hist_cap * 2 : 32;
        char **n = realloc(hist, sizeof(char *) * ncap);
        if (n == NULL) return;
        hist = n; hist_cap = ncap;
    }
    copy = malloc(strlen(line) + 1);
    if (copy == NULL) return;
    strcpy(copy, line);
    hist[hist_len++] = copy;
}

void savo_history_free(void) {
    int i;
    for (i = 0; i < hist_len; i++) free(hist[i]);
    free(hist);
    hist = NULL; hist_len = hist_cap = 0;
}

/* ---------------------------- editable buffer --------------------------- */

typedef struct { char *buf; size_t len, cap, pos; } Line;

static void line_init(Line *l) {
    l->cap = 64; l->len = 0; l->pos = 0;
    l->buf = malloc(l->cap);
    if (l->buf) l->buf[0] = '\0';
}

static void line_set(Line *l, const char *s) {
    size_t n = strlen(s);
    if (n + 1 > l->cap) {
        char *nb = realloc(l->buf, n + 1);
        if (nb == NULL) return;
        l->buf = nb; l->cap = n + 1;
    }
    memcpy(l->buf, s, n + 1);
    l->len = l->pos = n;
}

static void line_insert(Line *l, char c) {
    if (l->len + 2 > l->cap) {
        size_t nc = l->cap * 2;
        char *nb = realloc(l->buf, nc);
        if (nb == NULL) return;
        l->buf = nb; l->cap = nc;
    }
    memmove(l->buf + l->pos + 1, l->buf + l->pos, l->len - l->pos + 1);
    l->buf[l->pos] = c;
    l->pos++; l->len++;
}

static void line_backspace(Line *l) {
    if (l->pos == 0) return;
    memmove(l->buf + l->pos - 1, l->buf + l->pos, l->len - l->pos + 1);
    l->pos--; l->len--;
}

static void line_delete(Line *l) {
    if (l->pos >= l->len) return;
    memmove(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos);
    l->len--;
}

/* Redraw the prompt and line, then park the cursor at the edit position. */
static void refresh(const char *prompt, Line *l) {
    char seq[32];
    write(STDOUT_FILENO, "\r", 1);
    write(STDOUT_FILENO, prompt, strlen(prompt));
    write(STDOUT_FILENO, l->buf, l->len);
    write(STDOUT_FILENO, "\x1b[K", 3);            /* erase to end of line */
    if (l->pos < l->len) {
        int back = (int) (l->len - l->pos);
        int n = snprintf(seq, sizeof seq, "\x1b[%dD", back);
        write(STDOUT_FILENO, seq, (size_t) n);
    }
}

/* ------------------------------ the reader ------------------------------ */

static char *fallback_line(void) {   /* not a TTY: plain read, no editing */
    char  *buf = NULL;
    size_t cap = 0;
    ssize_t n = getline(&buf, &cap, stdin);
    if (n < 0) { free(buf); return NULL; }
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
    return buf;
}

char *savo_readline(const char *prompt) {
    struct termios orig, raw;
    Line l;
    char *stash = NULL;      /* in-progress line saved while browsing history */
    int   hpos;
    char *result;

    if (!isatty(STDIN_FILENO)) {
        if (prompt && *prompt) { fputs(prompt, stdout); fflush(stdout); }
        return fallback_line();
    }

    if (tcgetattr(STDIN_FILENO, &orig) == -1) return fallback_line();
    raw = orig;
    raw.c_lflag &= ~(unsigned) (ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(unsigned) (IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(unsigned) (OPOST);
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return fallback_line();

    line_init(&l);
    hpos = hist_len;
    refresh(prompt, &l);

    for (;;) {
        char c;
        ssize_t nr = read(STDIN_FILENO, &c, 1);
        if (nr <= 0) {                       /* EOF */
            if (l.len == 0) { result = NULL; goto done; }
            break;
        }

        if (c == '\r' || c == '\n') {
            write(STDOUT_FILENO, "\r\n", 2);
            break;
        } else if (c == 3) {                 /* Ctrl-C: cancel this line */
            write(STDOUT_FILENO, "^C\r\n", 4);
            l.buf[0] = '\0'; l.len = l.pos = 0;
            break;
        } else if (c == 4) {                 /* Ctrl-D */
            if (l.len == 0) { write(STDOUT_FILENO, "\r\n", 2); result = NULL; goto done; }
            line_delete(&l); refresh(prompt, &l);
        } else if (c == 127 || c == 8) {     /* Backspace */
            line_backspace(&l); refresh(prompt, &l);
        } else if (c == 1) {                 /* Ctrl-A: home */
            l.pos = 0; refresh(prompt, &l);
        } else if (c == 5) {                 /* Ctrl-E: end */
            l.pos = l.len; refresh(prompt, &l);
        } else if (c == 21) {                /* Ctrl-U: clear line */
            l.buf[0] = '\0'; l.len = l.pos = 0; refresh(prompt, &l);
        } else if (c == 11) {                /* Ctrl-K: kill to end */
            l.buf[l.pos] = '\0'; l.len = l.pos; refresh(prompt, &l);
        } else if (c == 27) {                /* escape sequence */
            char s[2];
            if (read(STDIN_FILENO, &s[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &s[1], 1) != 1) continue;
            if (s[0] == '[') {
                if (s[1] == 'C') { if (l.pos < l.len) l.pos++; refresh(prompt, &l); }         /* right */
                else if (s[1] == 'D') { if (l.pos > 0) l.pos--; refresh(prompt, &l); }        /* left */
                else if (s[1] == 'H') { l.pos = 0; refresh(prompt, &l); }
                else if (s[1] == 'F') { l.pos = l.len; refresh(prompt, &l); }
                else if (s[1] == '3') { char t; if (read(STDIN_FILENO, &t, 1) == 1 && t == '~') { line_delete(&l); refresh(prompt, &l); } }
                else if (s[1] == 'A') {                                                       /* up: older */
                    if (hpos > 0) {
                        if (hpos == hist_len) { free(stash); stash = malloc(l.len + 1); if (stash) memcpy(stash, l.buf, l.len + 1); }
                        hpos--; line_set(&l, hist[hpos]); refresh(prompt, &l);
                    }
                } else if (s[1] == 'B') {                                                     /* down: newer */
                    if (hpos < hist_len) {
                        hpos++;
                        if (hpos == hist_len) line_set(&l, stash ? stash : "");
                        else line_set(&l, hist[hpos]);
                        refresh(prompt, &l);
                    }
                }
            }
        } else if ((unsigned char) c >= 32) { /* printable */
            line_insert(&l, c);
            refresh(prompt, &l);
        }
    }

    result = malloc(l.len + 1);
    if (result) memcpy(result, l.buf, l.len + 1);

done:
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    free(l.buf);
    free(stash);
    return result;
}
