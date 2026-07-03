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
    VAL_STR,
    VAL_ARR,
    VAL_OBJ
} ValueType;

/*
 * Arrays are heap objects shared by reference and reclaimed by reference
 * counting: copying a Value that holds an array bumps the count rather than
 * duplicating the elements, so `savovar @b = @a` makes @b and @a see the same
 * list. (Reference cycles are not collected, but a script is short-lived.)
 */
typedef struct Array {
    int           rc;
    struct Value *items;
    int           count;
    int           cap;
} Array;

/* Objects are string-keyed maps, also shared by reference and refcounted. */
typedef struct MapEntry {
    char            *key;
    struct Value    *val;
    struct MapEntry *next;
} MapEntry;

typedef struct Map {
    int       rc;
    MapEntry *head;
    int       count;
} Map;

typedef struct Value {
    ValueType type;
    union {
        double        num;   /* VAL_NUM */
        char         *str;   /* VAL_STR (owned) */
        struct Array *arr;   /* VAL_ARR (shared, refcounted) */
        struct Map   *obj;   /* VAL_OBJ (shared, refcounted) */
    } as;
} Value;

/* constructors */
Value value_num(double n);
Value value_str(char *owned);        /* takes ownership of the string */
Value value_str_copy(const char *s); /* copies the string */
Value value_array(void);             /* fresh empty array (rc = 1) */
Value value_object(void);            /* fresh empty object (rc = 1) */

/* ownership */
Value value_copy(Value v);           /* deep copy for strings, share for arrays */
void  value_free(Value v);           /* release owned storage / drop a reference */

/* array operations (the Value must be VAL_ARR) */
void   array_push(Value v, Value elem);       /* appends a copy of elem */
int    array_length(Value v);
Value  array_get(Value v, int i);             /* a copy; 0 if out of range */
void   array_set(Value v, int i, Value elem); /* stores a copy of elem */

/* object operations (the Value must be VAL_OBJ) */
void   object_set(Value v, const char *key, Value elem); /* stores a copy */
Value  object_get(Value v, const char *key);             /* a copy; 0 if absent */
int    object_length(Value v);

/* coercions */
double value_to_number(Value v);     /* strings parse as numbers (atof) */
char  *value_to_string(Value v);     /* fresh heap string; numbers via %g */
int    value_truthy(Value v);        /* number != 0, or non-empty string */

/* helpers */
int    value_is_str(Value v);
void   value_print(Value v);         /* print without a trailing newline */

#endif
