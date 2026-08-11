#ifndef SAVO_H
#define SAVO_H

/*
 * Embeddable Savo interpreter API.
 *
 * Link against libsavo.a (see `make lib`) and include this header to run Savo
 * source from a host C program. The interpreter keeps global state, so it is
 * single-threaded: drive it from one thread at a time.
 *
 *   #include "savo.h"
 *   savo_run_string("savoprint \"hello\\n\"");
 *   savo_reset();   // clear variables and functions between programs
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Parse and execute a NUL-terminated Savo program. Returns 0 on success or 1 if
 * any syntax or runtime error was reported. Variables and functions defined in
 * one call persist into the next until savo_reset(). In embedded use savoquit
 * stops the current run instead of terminating the host process. */
int savo_run_string(const char *code);

/* Discard all interpreter state (variables and defined functions). */
void savo_reset(void);

#ifdef __cplusplus
}
#endif

#endif
