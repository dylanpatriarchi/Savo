#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

static char *xstrdup(const char *s) {
    char *p = strdup(s ? s : "");
    if (p == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    return p;
}

Value value_num(double n) {
    Value v; v.type = VAL_NUM; v.as.num = n; return v;
}

Value value_str(char *owned) {
    Value v; v.type = VAL_STR; v.as.str = owned ? owned : xstrdup(""); return v;
}

Value value_str_copy(const char *s) {
    return value_str(xstrdup(s));
}

Value value_copy(Value v) {
    if (v.type == VAL_STR) return value_str(xstrdup(v.as.str));
    return v;
}

void value_free(Value v) {
    if (v.type == VAL_STR) free(v.as.str);
}

double value_to_number(Value v) {
    if (v.type == VAL_NUM) return v.as.num;
    return atof(v.as.str);
}

/* Numbers use Savo's usual two-decimal format so existing output is unchanged. */
char *value_to_string(Value v) {
    if (v.type == VAL_STR) return xstrdup(v.as.str);
    {
        char buf[64];
        snprintf(buf, sizeof buf, "%.2f", v.as.num);
        return xstrdup(buf);
    }
}

int value_truthy(Value v) {
    if (v.type == VAL_NUM) return v.as.num != 0;
    return v.as.str[0] != '\0';
}

int value_is_str(Value v) { return v.type == VAL_STR; }

void value_print(Value v) {
    if (v.type == VAL_STR) printf("%s", v.as.str);
    else                   printf("%.2f", v.as.num);
}
