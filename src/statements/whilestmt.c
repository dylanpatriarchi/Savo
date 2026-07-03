#include <stdio.h>
#include "whilestmt.h"

/*
 * Repeat `argument` `count` times.
 *
 * The previous implementation looped forever when count == 1; Savo has no way
 * to break out of a loop, so an unconditional infinite loop was a footgun.
 * `savowhile` now behaves as a bounded repeat: a negative or zero count prints
 * nothing.
 */
void whileStatement(int count, const char *argument) {
    int i;
    for (i = 0; i < count; i++) {
        printf("%s\n", argument);
    }
}
