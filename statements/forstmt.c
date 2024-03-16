#include <stdio.h>
#include <stdlib.h>
#include "../global.h"
#include <string.h>
#include "printstmt.h"

void forStatement(int index, char *argument) {
    int i;

    for (i = 0; i < index; i++) {
        printf("%s", argument);
    }
}
