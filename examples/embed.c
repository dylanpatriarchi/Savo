/*
 * Embedding the Savo interpreter in a C program.
 *
 * Build the library and this demo:
 *     make lib
 *     cc -Isrc examples/embed.c libsavo.a -lm -o embed
 *     ./embed
 */
#include <stdio.h>
#include "savo.h"

int main(void) {
    /* Run a program; variables persist across calls until savo_reset(). */
    savo_run_string("savovar @x = 6 * 7\n"
                    "savoprint \"x = \" + @x + \"\\n\"");

    savo_run_string("savoprint \"@x is still \" + @x + \"\\n\"");

    /* Higher-order features work through the embedded API too. */
    savo_run_string("savodef square(@n)\n"
                    "    savoreturn @n * @n\n"
                    "savoend\n"
                    "savoprint savomap([1, 2, 3, 4], square)\n");

    /* Clear state, then observe that @x is gone (nil). */
    savo_reset();
    {
        int err = savo_run_string("savoprint \"after reset, @x = \" + @x + \"\\n\"");
        printf("(run reported error status %d for the undefined variable)\n", err);
    }
    return 0;
}
