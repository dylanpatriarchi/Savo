#ifndef LINEEDIT_H
#define LINEEDIT_H

/*
 * A small, dependency-free line editor for the REPL: cursor movement, history
 * (Up/Down), Home/End, backspace/delete, and Ctrl-C/Ctrl-D. When stdin is not a
 * terminal it falls back to a plain line read so pipes and files still work.
 */

/* Read one line shown after `prompt`. Returns a malloc'd string without the
 * trailing newline (caller frees), or NULL at end of input (Ctrl-D on an empty
 * line, or EOF). */
char *savo_readline(const char *prompt);

/* Append a line to the in-memory history (skipping blanks and duplicates). */
void  savo_history_add(const char *line);

/* Free the history (call at shutdown). */
void  savo_history_free(void);

#endif
