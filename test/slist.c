/* Most of these macro definitions are taken from
 * https://github.com/openbsd/src/blob/master/sys/sys/queue.h */

#define NULL 0

/*
 * Singly-linked List definitions.
 */

#define SLIST_HEAD(name, type)                                                 \
  struct name {                                                                \
    struct type *slh_first; /* first element */                                \
  }

#define SLIST_HEAD_INITIALIZER(head)                                           \
  { NULL }

#define SLIST_ENTRY(type)                                                      \
  struct {                                                                     \
    struct type *sle_next; /* next element */                                  \
  }

/*
 * Singly-linked List access methods.
 */
#define SLIST_FIRST(head) ((head)->slh_first)
#define SLIST_END(head) NULL
#define SLIST_EMPTY(head) (SLIST_FIRST(head) == SLIST_END(head))
#define SLIST_NEXT(elm, field) ((elm)->field.sle_next)

#define SLIST_FOREACH(var, head, field)                                        \
  for ((var) = SLIST_FIRST(head); (var) != SLIST_END(head);                    \
       (var) = SLIST_NEXT(var, field))

struct entry {
  int a;
  int b;
  char *s;
  char *t;
  SLIST_ENTRY(entry) entries;
};

SLIST_HEAD(slisthead, entry) head = SLIST_HEAD_INITIALIZER(head);

/* Goal is to generate:

struct entry* np;
SLIST_FOREACH(np, &head, entries) {
      printf("%d", np->a);
      printf("%s", np->b);
}

*/
