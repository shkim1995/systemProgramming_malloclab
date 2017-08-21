#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);


typedef struct {
    const char *name;    /* full name */
    const char *id;      /* login ID */
} student_t;

extern student_t student;

