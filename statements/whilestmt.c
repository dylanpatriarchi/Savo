#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "../global.h"
#include <string.h>
#include "printstmt.h"

void whileStatement(const char* condition, const char* argument) {
    if (isStringNumeric(condition)) {
        int iterations = atoi(condition);
        int i;
        for (i = 0; i < iterations; i++) {
            printf("%s\n", argument);
        }
    } else {
        if (strcasecmp(condition, "true") == 0) {
            while (1) {
                printf("%s\n", argument);
            }
        } else if (strcasecmp(condition, "false") == 0) {
            while (0) {
                printf("%s\n", argument);
            }
        } else {
            printf("Condizione non riconosciuta: %s\n", condition);
        }
    }
}

int isStringNumeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}
