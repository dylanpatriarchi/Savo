#ifndef GLOBAL_H
#define GLOBAL_H

#define SAVO_VERSION "3.4.0"

/* Runtime prompt shown in interactive mode; empty string when running a file. */
extern char *prompt;

/* ASCII-art banner printed on start-up in interactive mode. */
extern char *consoleMex;

/* Set on any syntax or runtime error; becomes the CLI's exit status. */
extern int had_error;

/* Source line of the statement currently executing (0 if unknown). Set by the
 * interpreter and read by runtime diagnostics so every message can name a line. */
extern int savo_line;

/* When non-zero (the CLI default), savoquit exits the process. Embedders set it
 * to 0 so savoquit merely stops the current run via savo_quit_flag. */
extern int savo_exit_on_quit;
extern int savo_quit_flag;

/* Print a runtime warning to stderr, prefixed with the current line when known.
 * Does not set the error flag (these are recoverable, "using 0" situations). */
void savo_warn(const char *fmt, ...);

#endif
