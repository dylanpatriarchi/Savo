/*
 * libFuzzer entry point for Savo.
 *
 * Feeds arbitrary bytes to the embeddable interpreter and resets state between
 * runs, so a crash means a real bug (memory error, UB, or an unbounded
 * recursion the guards missed). Build and run with:
 *
 *     make fuzz
 *     ./fuzz_savo -max_total_time=60 corpus
 *
 * The interpreter runs in embedded mode, so savoquit stops a run instead of
 * exiting the fuzzer.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "savo.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char *code = malloc(size + 1);
    if (code == NULL) return 0;
    memcpy(code, data, size);
    code[size] = '\0';

    savo_run_string(code);
    savo_reset();

    free(code);
    return 0;
}
