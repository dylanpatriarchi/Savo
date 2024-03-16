#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "../global.h"
#include <string.h>
#include "printstmt.h"

void whileStatement(int condition, const char* argument) {
    if (condition != 1 && condition != 0) {
        int iterations = atoi(condition);
        int i;
        for (i = 0; i < iterations; i++) {
            printf("%s\n", argument);
        }
    } else {
        if (condition == 1) {
            while (1) {
                printf("%s\n", argument);
            }
        } else {
            while (0) {
                printf("%s\n", argument);
            }
        }
    }
}