#ifndef VALUE_H
#define VALUE_H

/*
 * A dynamically-typed Savo value.
 *
 * Values are passed around by copy. Heap-backed kinds (strings, and later
 * arrays/objects) own their storage, so every Value must eventually be handed
 * to value_free, and a Value that is stored or duplicated must go through
 * value_copy. Keeping this discipline in one module makes it straightforward to
 * add new kinds without touching the interpreter's ownership logic.
 */

typedef enum {
    VAL_NUM,
    VAL_STR
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        double  num;   /* VAL_NUM */
        char   *str;   /* VAL_STR (owned) */
    } as;
} Value;

/* constructors */
Value value_num(double n);
Value value_str(char *owned);        /* takes ownership of the string */
Value value_str_copy(const char *s); /* copies the string */

/* ownership */
Value value_copy(Value v);           /* deep copy */
void  value_free(Value v);           /* release owned storage */

/* coercions */
double value_to_number(Value v);     /* strings parse as numbers (atof) */
char  *value_to_string(Value v);     /* fresh heap string; numbers via %g */
int    value_truthy(Value v);        /* number != 0, or non-empty string */

/* helpers */
int    value_is_str(Value v);
void   value_print(Value v);         /* print without a trailing newline */

#endif
