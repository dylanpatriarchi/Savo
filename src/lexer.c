#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "global.h"
#include "lineedit.h"

/* ------------------------------- keywords ------------------------------- */

static const struct { const char *word; TokKind kind; } KEYWORDS[] = {
    { "savoprint", TK_PRINT },   { "savovar", TK_VAR },
    { "savosum", TK_SUM },       { "savosubtract", TK_SUBTRACT },
    { "savomoltiplication", TK_MULTIPLICATION },
    { "savodivide", TK_DIVISION }, { "savomod", TK_MOD },
    { "savosqrt", TK_SQRT },     { "savopow", TK_POW },
    { "savoabs", TK_ABS },       { "savorandom", TK_RANDOM },
    { "savofloor", TK_FLOOR },   { "savoceil", TK_CEIL },
    { "savoround", TK_ROUND },   { "savolog10", TK_LOG10 },
    { "savolog", TK_LOG },       { "savomax", TK_MAX },
    { "savomin", TK_MIN },       { "savolen", TK_LEN },
    { "savoupper", TK_UPPER },   { "savolower", TK_LOWER },
    { "savostr", TK_TOSTR },     { "savonum", TK_TONUM },
    { "savotrim", TK_TRIM },     { "savosubstr", TK_SUBSTR },
    { "savoindexof", TK_INDEXOF }, { "savoreplace", TK_REPLACE },
    { "savosplit", TK_SPLIT },   { "savojoin", TK_JOIN },
    { "savopop", TK_POP },       { "savocontains", TK_CONTAINS },
    { "savokeys", TK_KEYS },     { "savoinput", TK_INPUT },
    { "savomap", TK_MAP },       { "savofilter", TK_FILTER },
    { "savoreduce", TK_REDUCE }, { "savosort", TK_SORT },
    { "savoforeach", TK_FOREACH },
    { "savotrue", TK_TRUE },     { "savofalse", TK_FALSE },
    { "savonil", TK_NIL },
    { "savoif", TK_IF },         { "savoelif", TK_ELIF },
    { "savoelse", TK_ELSE },
    { "savoend", TK_END },       { "savowhile", TK_WHILE },
    { "savofor", TK_FOR },       { "savodef", TK_DEF },
    { "savoreturn", TK_RETURN }, { "savobreak", TK_BREAK },
    { "savocontinue", TK_CONTINUE }, { "savoassert", TK_ASSERT },
    { "savopush", TK_PUSH },
    { "savoset", TK_SET },       { "savodir", TK_DIR },
    { "savols", TK_LS },         { "savocls", TK_CLS },
    { "savoclear", TK_CLEAR },   { "savohelp", TK_HELP },
    { "savoquit", TK_QUIT },     { "savoexit", TK_EXIT },
    { "savopointercell", TK_POINTER }
};

static TokKind keyword_lookup(const char *lower) {
    size_t i;
    for (i = 0; i < sizeof KEYWORDS / sizeof KEYWORDS[0]; i++)
        if (strcmp(KEYWORDS[i].word, lower) == 0) return KEYWORDS[i].kind;
    return TK_IDENT;
}

const char *token_name(TokKind k) {
    switch (k) {
        case TK_EOF:      return "end of input";
        case TK_NEWLINE:  return "end of line";
        case TK_NUMBER:   return "number";
        case TK_STRING:   return "string";
        case TK_IDENT:    return "identifier";
        case TK_LPAREN:   return "'('";
        case TK_RPAREN:   return "')'";
        case TK_LBRACKET: return "'['";
        case TK_RBRACKET: return "']'";
        case TK_LBRACE:   return "'{'";
        case TK_RBRACE:   return "'}'";
        case TK_COMMA:    return "','";
        case TK_COLON:    return "':'";
        case TK_ASSIGN:   return "'='";
        default:          return "token";
    }
}

/* ------------------------------ input model ----------------------------- */

void lexer_init_buffer(Lexer *lx, char *owned_buf) {
    lx->fp = NULL;
    lx->buf = owned_buf;
    lx->len = owned_buf ? strlen(owned_buf) : 0;
    lx->pos = 0;
    lx->cap = 0;
    lx->interactive = 0;
    lx->line = 1;
    lx->col = 1;
    lx->started = 1;
    lx->eof_nl_done = 0;
    lx->input_done = 0;
}

void lexer_init_stream(Lexer *lx, FILE *fp, int interactive) {
    lx->fp = fp;
    lx->buf = NULL;
    lx->len = 0;
    lx->pos = 0;
    lx->cap = 0;
    lx->interactive = interactive;
    lx->line = 1;
    lx->col = 1;
    lx->started = 0;
    lx->eof_nl_done = 0;
    lx->input_done = 0;
}

void lexer_free(Lexer *lx) {
    free(lx->buf);
    lx->buf = NULL;
}

/* Refill the buffer with the next line from the stream. Returns 1 on success,
 * 0 at end of stream. Only used in stream mode. */
static int refill(Lexer *lx) {
    ssize_t got;
    if (lx->fp == NULL) return 0;
    lx->started = 1;

    if (lx->interactive) {
        /* The line editor handles the prompt, history and cursor movement, then
         * returns the line without a newline; re-add one so the lexer still sees
         * a NEWLINE token terminating the line. */
        char  *line = savo_readline(prompt);
        size_t n;
        if (line == NULL) { lx->len = 0; lx->pos = 0; return 0; }
        savo_history_add(line);
        n = strlen(line);
        if (n + 2 > lx->cap) {
            char *nb = realloc(lx->buf, n + 2);
            if (nb == NULL) { free(line); fprintf(stderr, "savo: out of memory\n"); exit(1); }
            lx->buf = nb; lx->cap = n + 2;
        }
        memcpy(lx->buf, line, n);
        lx->buf[n] = '\n';
        lx->buf[n + 1] = '\0';
        lx->len = n + 1;
        lx->pos = 0;
        free(line);
        return 1;
    }

    got = getline(&lx->buf, &lx->cap, lx->fp);
    if (got < 0) { lx->len = 0; lx->pos = 0; return 0; }
    lx->len = (size_t) got;
    lx->pos = 0;
    return 1;
}

/* Peek the current character, refilling from the stream if needed. -1 at EOF. */
static int cur(Lexer *lx) {
    if (lx->pos >= lx->len) {
        if (lx->input_done) return -1;   /* sticky EOF: never refill again */
        if (!refill(lx) || lx->len == 0) { lx->input_done = 1; return -1; }
    }
    return (unsigned char) lx->buf[lx->pos];
}

/* Peek one character ahead within the current buffer only (multi-char tokens
 * never straddle a line boundary). -1 if none. */
static int peek2(Lexer *lx) {
    if (lx->pos + 1 >= lx->len) return -1;
    return (unsigned char) lx->buf[lx->pos + 1];
}

static int advance(Lexer *lx) {
    int c = cur(lx);
    if (c < 0) return c;
    lx->pos++;
    if (c == '\n') { lx->line++; lx->col = 1; }
    else lx->col++;
    return c;
}

/* ------------------------------ scanning -------------------------------- */

/* Rewrite \n \t \r \\ \" escapes in place (unknown escapes keep the backslash). */
static void unescape(char *s) {
    char *w = s, *r;
    for (r = s; *r; r++) {
        if (*r == '\\' && *(r + 1)) {
            r++;
            switch (*r) {
                case 'n':  *w++ = '\n'; break;
                case 't':  *w++ = '\t'; break;
                case 'r':  *w++ = '\r'; break;
                case '\\': *w++ = '\\'; break;
                case '"':  *w++ = '"';  break;
                default:   *w++ = '\\'; *w++ = *r; break;
            }
        } else {
            *w++ = *r;
        }
    }
    *w = 0;
}

static Token make(TokKind k, int line, int col) {
    Token t; t.kind = k; t.text = NULL; t.num = 0; t.line = line; t.col = col;
    return t;
}

/* Collect a run of characters into a fresh heap string. */
static char *slice(Lexer *lx, size_t start) {
    size_t n = lx->pos - start;
    char *s = malloc(n + 1);
    if (s == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    memcpy(s, lx->buf + start, n);
    s[n] = 0;
    return s;
}

Token lexer_next(Lexer *lx) {
    int c;

    for (;;) {
        c = cur(lx);
        if (c < 0) {
            if (!lx->eof_nl_done) { lx->eof_nl_done = 1; return make(TK_NEWLINE, lx->line, lx->col); }
            return make(TK_EOF, lx->line, lx->col);
        }
        if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') { advance(lx); continue; }
        if (c == '#') { while ((c = cur(lx)) >= 0 && c != '\n') advance(lx); continue; }
        break;
    }

    {
        int line = lx->line, col = lx->col;

        if (c == '\n') { advance(lx); return make(TK_NEWLINE, line, col); }

        /* number: digits, or a leading dot followed by digits */
        if (isdigit(c) || (c == '.' && isdigit(peek2(lx)))) {
            size_t start = lx->pos;
            Token t;
            while (isdigit(cur(lx))) advance(lx);
            /* a '.' is part of the number only when a digit follows, so that
             * `3.foo` stays 3 followed by the '.' field operator (as before). */
            if (cur(lx) == '.' && isdigit(peek2(lx))) {
                advance(lx);
                while (isdigit(cur(lx))) advance(lx);
            }
            t = make(TK_NUMBER, line, col);
            { char *s = slice(lx, start); t.num = atof(s); free(s); }
            return t;
        }

        /* string literal */
        if (c == '"') {
            size_t start;
            Token t;
            advance(lx);            /* opening quote */
            start = lx->pos;
            while ((c = cur(lx)) >= 0 && c != '"' && c != '\n') {
                if (c == '\\' && peek2(lx) >= 0) advance(lx);   /* keep escaped char */
                advance(lx);
            }
            t = make(TK_STRING, line, col);
            t.text = slice(lx, start);
            if (cur(lx) == '"') advance(lx);   /* closing quote (missing = unterminated, tolerated) */
            unescape(t.text);
            return t;
        }

        /* identifier or keyword: optional '@', then a letter/underscore run */
        if (c == '@' || isalpha(c) || c == '_') {
            size_t start = lx->pos;
            int at = (c == '@');
            Token t;
            advance(lx);
            while ((c = cur(lx)) >= 0 && (isalnum(c) || c == '_')) advance(lx);
            t = make(TK_IDENT, line, col);
            t.text = slice(lx, start);
            if (!at) {
                char lower[64];
                size_t i, n = lx->pos - start;
                if (n < sizeof lower) {
                    for (i = 0; i < n; i++) lower[i] = (char) tolower((unsigned char) t.text[i]);
                    lower[n] = 0;
                    { TokKind k = keyword_lookup(lower);
                      if (k != TK_IDENT) { free(t.text); t.text = NULL; t.kind = k; } }
                }
            }
            return t;
        }

        /* operators and punctuation */
        advance(lx);
        switch (c) {
            case '+': return make(TK_PLUS, line, col);
            case '-': return make(TK_MINUS, line, col);
            case '*': return make(TK_STAR, line, col);
            case '/': return make(TK_SLASH, line, col);
            case '%': return make(TK_PERCENT, line, col);
            case '(': return make(TK_LPAREN, line, col);
            case ')': return make(TK_RPAREN, line, col);
            case '[': return make(TK_LBRACKET, line, col);
            case ']': return make(TK_RBRACKET, line, col);
            case '{': return make(TK_LBRACE, line, col);
            case '}': return make(TK_RBRACE, line, col);
            case ':': return make(TK_COLON, line, col);
            case '.': return make(TK_DOT, line, col);
            case ',': return make(TK_COMMA, line, col);
            case '=': if (cur(lx) == '=') { advance(lx); return make(TK_EQ, line, col); }
                      return make(TK_ASSIGN, line, col);
            case '!': if (cur(lx) == '=') { advance(lx); return make(TK_NE, line, col); }
                      return make(TK_NOT, line, col);
            case '<': if (cur(lx) == '=') { advance(lx); return make(TK_LE, line, col); }
                      return make(TK_LT, line, col);
            case '>': if (cur(lx) == '=') { advance(lx); return make(TK_GE, line, col); }
                      return make(TK_GT, line, col);
            case '&': if (cur(lx) == '&') { advance(lx); return make(TK_AND, line, col); }
                      break;
            case '|': if (cur(lx) == '|') { advance(lx); return make(TK_OR, line, col); }
                      break;
            default:  break;
        }
        /* unrecognised byte: report as an error-bearing newline-free token so the
         * parser can flag it; represent with an identifier holding the char. */
        {
            Token t = make(TK_IDENT, line, col);
            char *s = malloc(2); s[0] = (char) c; s[1] = 0; t.text = s;
            return t;
        }
    }
}

char *lexer_read_line_arg(Lexer *lx) {
    size_t start, end;
    char *s;
    int c;
    while ((c = cur(lx)) == ' ' || c == '\t') advance(lx);
    c = cur(lx);
    if (c < 0 || c == '\n' || c == '#') return NULL;
    start = lx->pos;
    while ((c = cur(lx)) >= 0 && c != '\n') advance(lx);
    end = lx->pos;
    while (end > start && (lx->buf[end - 1] == ' ' || lx->buf[end - 1] == '\t' || lx->buf[end - 1] == '\r'))
        end--;
    if (end == start) return NULL;
    s = malloc(end - start + 1);
    if (s == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    memcpy(s, lx->buf + start, end - start);
    s[end - start] = 0;
    return s;
}
