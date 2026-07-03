#include <stdio.h>
#include "forstmt.h"

/* Print `argument` `count` times (count <= 0 prints nothing). */
void forStatement(int count, const char *argument) {
    int i;
    for (i = 0; i < count; i++) {
        printf("%s\n", argument);
    }
}
