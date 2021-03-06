/*
 * M*LIB - DEQUE module
 *
 * Copyright (c) 2017-2021, Patrick Pelissier
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef MSTARLIB_DEQUE_H
#define MSTARLIB_DEQUE_H

#include "m-i-list.h"

/* Define a deque of a given type and its associated functions.
   USAGE: M_DEQUE_DEF(name, type [, oplist_of_the_type]) */
#define M_DEQUE_DEF(name, ...)                                                 \
  M_BEGIN_PROTECTED_CODE                                                       \
  DEQUEI_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                    \
                ((name, __VA_ARGS__,                                           \
                 M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(),                        \
                 M_T(name, t), M_T(name, it, t), M_T(name, node, ct) ),        \
                 (name, __VA_ARGS__,                                           \
                 M_T(name, t), M_T(name, it, t), M_T(name, node, ct))))        \
  M_END_PROTECTED_CODE


/* Define a deque of a given type and its associated functions.
   as the provided type name_t with the iterator named it_t.
   USAGE: M_DEQUE_DEF(name, name_t, it_t, type [, oplist_of_the_type]) */
#define DEQUE_DEF_AS(name, name_t, it_t, ...)                                  \
  M_BEGIN_PROTECTED_CODE                                                       \
  DEQUEI_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                    \
  ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)(),                  \
    name_t, it_t, M_T(name, node, ct)),                                        \
   (name, __VA_ARGS__,                                                         \
    name_t, it_t, M_T(name, node, ct))))                                       \
  M_END_PROTECTED_CODE

/* Define the oplist of a deque of a type.
   USAGE: DEQUE_OPLIST(name[, oplist of the type]) */
#define DEQUE_OPLIST(...)                                                      \
  DEQUEI_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                                 \
                   ((__VA_ARGS__, M_DEFAULT_OPLIST),                           \
                    (__VA_ARGS__ )))

/********************************** INTERNAL ************************************/

/* Default initial size of a bucket of items */
#ifndef DEQUEUI_DEFAULT_SIZE
#define DEQUEUI_DEFAULT_SIZE  8
#endif

/* Define the internal contract of a deque */
#define DEQUEI_CONTRACT(d) do {                                                \
    M_ASSERT((d) != NULL);                                                     \
    M_ASSERT((d)->default_size >= DEQUEUI_DEFAULT_SIZE);                       \
    M_ASSERT((d)->front->node != NULL);                                        \
    M_ASSERT((d)->front->index <= (d)->front->node->size);                     \
    M_ASSERT((d)->back->node != NULL);                                         \
    M_ASSERT((d)->back->index <= (d)->back->node->size);                       \
    M_ASSERT((d)->front->node != (d)->back->node ||                            \
             (d)->front->index <= (d)->back->index);                           \
    M_ASSERT((d)->front->node != (d)->back->node ||                            \
             (d)->back->index - (d)->front->index == (d)->count);              \
  } while (0)

/* Deferred evaluation for the deque definition,
   so that all arguments are evaluated before further expansion */
#define DEQUEI_DEF_P1(arg) DEQUEI_DEF_P2 arg

/* Validate the oplist before going further */
#define DEQUEI_DEF_P2(name, type, oplist, deque_t, it_t, node_t)               \
  M_IF_OPLIST(oplist)(DEQUEI_DEF_P3, DEQUEI_DEF_FAILURE)                       \
        (name, type, oplist, deque_t, it_t, node_t)

/* Stop processing with a compilation failure */
#define DEQUEI_DEF_FAILURE(name, type, oplist, deque_t, it_t, node_t)          \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST,                                        \
                   "(M_DEQUE_DEF): the given argument is not a valid oplist: "   \
                   #oplist)

/* Internal deque definition
   - name: prefix to be used
   - type: type of the elements of the deque
   - oplist: oplist of the type of the elements of the container
   - deque_t: alias for M_T(name, t) [ type of the container ]
   - it_t: alias for M_T(name, it_t) [ iterator of the container ]
   - node_t: alias for M_T(name, node_t) [ node ]
 */
#define DEQUEI_DEF_P3(name, type, oplist, deque_t, it_t, node_t)                                                                                                                             \
                                                                              \
  /* It is a linked list of buckets of the types,                             \
     each new created bucket size grows compared to the previous one          \
     resulting in:                                                            \
     strict O(1) for push/pop                                                 \
     O(ln(n)) for random access.                                              \
     No insert / delete operations are planned.                               \
     [Could be done in O(n) complexity if needed]                             \
     Define the bucket (aka node) structure.                                  \
  */                                                                          \
  typedef struct M_T(name, node_s) {                                          \
    ILIST_INTERFACE(M_T(name, node_list), struct M_T(name, node_s));          \
    size_t size;                                                              \
    type   data[M_MIN_FLEX_ARRAY_SIZE];                                       \
  } node_t;                                                                   \
                                                                              \
  /* Each node is allocated with a variable size bucket (so we use            \
     M_GET_REALLOC for the allocation). But we want to delete the nodes       \
     automatically with the intrusive list used for storing the nodes:        \
     so we register as a DEL operator the FREE operator of the oplist.        \
     The interfaces are compatible.                                           \
  */                                                                          \
  ILIST_DEF(M_T(name, node_list), node_t, (DEL(M_GET_FREE oplist)) )          \
                                                                              \
  /* Define an internal iterator */                                           \
  typedef struct M_T(name, it2_s) {                                           \
    node_t *node;                                                             \
    size_t  index;                                                            \
  } M_T(name, it2_ct)[1];                                                     \
                                                                              \
  /* Define the deque type:                                                   \
     - 'list' if the list of buckets containing the objects.                  \
     - 'front' and 'back' are iterators to the first and last elements.       \
     - 'default_size' is the size used for the creation of a new bucket       \
     - 'count' is the number of elements in the container.                    \
  */                                                                          \
  typedef struct M_T(name, s) {                                               \
    M_T(name, node_list_t) list;                                              \
    M_T(name, it2_ct)      front;                                             \
    M_T(name, it2_ct)      back;                                              \
    size_t                 default_size;                                      \
    size_t                 count;                                             \
  } deque_t[1];                                                               \
                                                                              \
  /* Define pointer alias */                                                  \
  typedef struct M_T(name, s) *M_T(name, ptr);                                \
  typedef const struct M_T(name, s) *M_T(name, srcptr);                       \
                                                                              \
  /* Define the iterator object */                                            \
  typedef struct M_T(name, it_s) {                                            \
    node_t                    *node;                                          \
    size_t                     index;                                         \
    const struct M_T(name, s) *deque;                                         \
  } it_t[1];                                                                  \
                                                                              \
  /* Define the internal types for the op-list */                             \
  typedef deque_t M_T(name, ct);                                              \
  typedef type    M_T(name, subtype_ct);                                      \
  typedef it_t    M_T(name, it_ct);                                           \
                                                                              \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                            \
                                                                              \
  static inline node_t*                                                       \
  M_F(name, int, new_node)(deque_t d)                                        \
  {                                                                           \
    size_t def = d->default_size;                                             \
    /* Test for overflow of the size computation */                           \
    if (M_UNLIKELY(def > SIZE_MAX / sizeof (type) - sizeof(node_t))) {        \
      M_MEMORY_FULL(sizeof(node_t) + def * sizeof(type));                     \
      return NULL;                                                            \
    }                                                                         \
    /* Alloc a new node */                                                    \
    node_t* n = (node_t*) (void*)                                             \
      M_CALL_REALLOC(oplist, char, NULL,                                      \
                     sizeof(node_t) + def * sizeof(type));                    \
    if (n == NULL) {                                                          \
      M_MEMORY_FULL(sizeof(node_t) + def * sizeof(type));                     \
      return NULL;                                                            \
    }                                                                         \
    /* Initialize the node */                                                 \
    n->size = def;                                                            \
    M_F(name, node_list, init_field)(n);                                     \
    /* Increase the next bucket allocation */                                 \
    /* Do not increase it too much if there are few items */                  \
    def = M_MIN(def, d->count);                                               \
    d->default_size = M_CALL_INC_ALLOC(oplist, def);                          \
    /* FIXME: Check for overflow? */                                          \
    return n;                                                                 \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_INITIALIZE)(deque_t d)                                         \
  {                                                                           \
    M_F(name, node_list, M_NAMING_INITIALIZE)(d->list);                            \
    d->default_size = DEQUEUI_DEFAULT_SIZE;                                   \
    d->count        = 0;                                                      \
    node_t *n       = M_F(name, int, new_node)(d);                           \
    if (n == NULL) return;                                                    \
    M_F(name, node_list, push_back)(d->list, n);                             \
    d->front->node  = n;                                                      \
    d->front->index = DEQUEUI_DEFAULT_SIZE / 2;                               \
    d->back->node   = n;                                                      \
    d->back->index  = DEQUEUI_DEFAULT_SIZE / 2;                               \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_CLEAN)(deque_t d)                                        \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    node_t *min_node = NULL;                                                  \
    for (node_t *n = d->front->node;                                          \
         n != NULL ;                                                          \
         n = (n == d->back->node) ? NULL :                                    \
           M_F(name, node_list, next_obj)(d->list, n)){                      \
      size_t min = n == d->front->node ? d->front->index : 0;                 \
      size_t max = n == d->back->node ? d->back->index : n->size;             \
      for(size_t i = min; i < max; i++) {                                     \
        M_CALL_CLEAR(oplist, n->data[i]);                                     \
      }                                                                       \
      min_node = (min_node == NULL || min_node->size > n->size) ?             \
                 n : min_node;                                                \
    }                                                                         \
    M_ASSERT(min_node != NULL);                                               \
    d->front->node = min_node;                                                \
    d->front->index = min_node->size / 2;                                     \
    d->back->node = min_node;                                                 \
    d->back->index = min_node->size / 2;                                      \
    d->count = 0;                                                             \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_FINALIZE)(deque_t d)                                     \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_F(name, M_NAMING_CLEAN)(d);                                             \
    /* We have registered the delete operator to clear all objects */         \
    M_F(name, node_list, M_NAMING_FINALIZE)(d->list);                        \
    /* It is safer to clean some variables */                                 \
    d->front->node  = NULL;                                                   \
    d->back->node   = NULL;                                                   \
    d->count = 0;                                                             \
  }                                                                           \
                                                                              \
  static inline type *                                                        \
  M_F(name, push_back, raw)(deque_t d)                                       \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    node_t *n = d->back->node;                                                \
    size_t index = d->back->index;                                            \
    if (M_UNLIKELY(n->size <= index)) {                                       \
      /* try to get an already allocated node */                              \
      n = M_F(name, node_list, next_obj)(d->list, n);                        \
      if (n == NULL) {                                                        \
        /* No node exists, allocate a new one */                              \
        n = M_F(name, int, new_node)(d);                                     \
        if (M_UNLIKELY(n == NULL)) return NULL;                               \
        M_F(name, node_list, push_back)(d->list, n);                         \
      }                                                                       \
      d->back->node = n;                                                      \
      index = 0;                                                              \
    }                                                                         \
    type *ret = &n->data[index];                                              \
    index++;                                                                  \
    d->count++;                                                               \
    d->back->index = index;                                                   \
    DEQUEI_CONTRACT(d);                                                       \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, push_back)(deque_t d, type const x)                               \
  {                                                                           \
    type *p = M_F(name, push_back, raw)(d);                                  \
    if (M_LIKELY(p != NULL)) {                                                \
      M_CALL_INIT_SET(oplist, *p, x);                                         \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  static inline type *                                                        \
  M_F(name, push_back, new)(deque_t d)                                       \
  {                                                                           \
    type *p = M_F(name, push_back, raw)(d);                                  \
    if (M_LIKELY(p != NULL)) {                                                \
      M_CALL_INIT(oplist, *p);                                                \
    }                                                                         \
    return p;                                                                 \
  }                                                                           \
  , )                                                                         \
                                                                              \
  static inline void                                                          \
  M_F(name, push_back, move)(deque_t d, type *x)                             \
  {                                                                           \
    M_ASSERT(x != NULL);                                                      \
    type *p = M_F(name, push_back, raw)(d);                                  \
    if (M_UNLIKELY(p == NULL))                                                \
      return;                                                                 \
    M_DO_INIT_MOVE(oplist, *p, *x);                                           \
  }                                                                           \
                                                                              \
  static inline type*                                                         \
  M_F(name, push_front, raw)(deque_t d)                                      \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    node_t *n = d->front->node;                                               \
    size_t index = d->front->index;                                           \
    index--;                                                                  \
    /* If overflow */                                                         \
    if (M_UNLIKELY(n->size <= index)) {                                       \
      n = M_F(name, node_list, previous_obj)(d->list, n);                    \
      if (n == NULL) {                                                        \
        n = M_F(name, int, new_node)(d);                                     \
        if (M_UNLIKELY(n == NULL)) return NULL;                               \
        M_F(name, node_list, push_front)(d->list, n);                        \
      }                                                                       \
      d->front->node = n;                                                     \
      index = n->size -1;                                                     \
    }                                                                         \
    type *ret = &n->data[index];                                              \
    d->count ++;                                                              \
    d->front->index = index;                                                  \
    DEQUEI_CONTRACT(d);                                                       \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, push_front)(deque_t d, type const x)                              \
  {                                                                           \
    type *p = M_F(name, push_front, raw)(d);                                 \
    if (M_LIKELY(p != NULL)) {                                                \
      M_CALL_INIT_SET(oplist, *p, x);                                         \
    }                                                                         \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  static inline type *                                                        \
  M_F(name, push_front, new)(deque_t d)                                      \
  {                                                                           \
    type *p = M_F(name, push_front, raw)(d);                                 \
    if (M_LIKELY(p != NULL)) {                                                \
      M_CALL_INIT(oplist, *p);                                                \
    }                                                                         \
    return p;                                                                 \
  }                                                                           \
  ,)                                                                          \
                                                                              \
  static inline void                                                          \
  M_F(name, push_front, move)(deque_t d, type *x)                            \
  {                                                                           \
    M_ASSERT(x != NULL);                                                      \
    type *p = M_F(name, push_front, raw)(d);                                 \
    if (M_UNLIKELY(p == NULL))                                                \
      return;                                                                 \
    M_DO_INIT_MOVE(oplist, *p, *x);                                           \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, pop_back)(type *ptr, deque_t d)                                   \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(d->count > 0);                                                   \
    node_t *n = d->back->node;                                                \
    size_t index = d->back->index;                                            \
    index--;                                                                  \
    if (M_UNLIKELY(n->size <= index)) {                                       \
      /* If there is a next node,                                             \
         pop the back node and push it back to the front. This                \
         reduce the used memory if the deque is used as a FIFO queue.*/       \
      node_t *next = M_F(name, node_list, next_obj)(d->list, n);              \
      if (next != NULL) {                                                     \
        next = M_F(name, node_list, pop_back)(d->list);                       \
        M_ASSERT(next != n);                                                  \
        M_F(name, node_list, push_front)(d->list, next);                      \
      }                                                                       \
      n = M_F(name, node_list, previous_obj)(d->list, n);                     \
      M_ASSERT(n != NULL && n->size > 1);                                     \
      d->back->node = n;                                                      \
      index = n->size - 1;                                                    \
    }                                                                         \
    if (ptr != NULL)                                                          \
      M_IF_METHOD(MOVE, oplist) (                                             \
      M_CALL_MOVE(oplist, *ptr, n->data[index]); else                         \
      ,                                                                       \
      M_CALL_SET(oplist, *ptr, n->data[index]);                               \
      )                                                                       \
    M_CALL_CLEAR(oplist,  n->data[index]);                                    \
    d->count--;                                                               \
    d->back->index = index;                                                   \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  static inline void                                                          \
  M_F(name, pop_back, move)(type *ptr, deque_t d)                             \
  {                                                                           \
    M_ASSERT(ptr != NULL);                                                    \
    /* Note: Lazy implementation. Can be improved if needed */                \
    M_CALL_INIT(oplist, *ptr);                                                \
    M_F(name, pop_back)(ptr, d);                                              \
  }                                                                           \
  , )                                                                         \
                                                                              \
  static inline void                                                          \
  M_F(name, pop_front)(type *ptr, deque_t d)                                  \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(d->count > 0);                                                   \
    node_t *n = d->front->node;                                               \
    size_t index = d->front->index;                                           \
    if (M_UNLIKELY (n->size <= index)) {                                      \
      /* If there is a previous node,                                         \
         pop the front node and push it back to the back. This                \
         reduce the used memory if the deque is used as a FIFO queue.*/       \
      node_t *prev = M_F(name, node_list, previous_obj)(d->list, n);          \
      if (prev != NULL) {                                                     \
        prev = M_F(name, node_list, pop_front)(d->list);                      \
        M_ASSERT(prev != n);                                                  \
        M_F(name, node_list, push_back)(d->list, prev);                       \
      }                                                                       \
      n = M_F(name, node_list, next_obj)(d->list, n);                         \
      M_ASSERT(n != NULL);                                                    \
      d->front->node = n;                                                     \
      index = 0;                                                              \
    }                                                                         \
    if (ptr != NULL)                                                          \
      M_IF_METHOD(MOVE, oplist) (                                             \
      M_CALL_MOVE(oplist, *ptr, n->data[index]); else                         \
      ,                                                                       \
      M_CALL_SET(oplist, *ptr, n->data[index]);                               \
      )                                                                       \
    M_CALL_CLEAR(oplist, n->data[index]);                                     \
    index++;                                                                  \
    d->count--;                                                               \
    d->front->index = index;                                                  \
  }                                                                           \
                                                                              \
  M_IF_METHOD(INIT, oplist)(                                                  \
  static inline void                                                          \
  M_F(name, pop_front, move)(type *ptr, deque_t d)                            \
  {                                                                           \
    M_ASSERT(ptr != NULL);                                                    \
    /* Note: Lazy implementation */                                           \
    M_CALL_INIT(oplist, *ptr);                                                \
    M_F(name, pop_front)(ptr, d);                                             \
  }                                                                           \
  ,)                                                                          \
                                                                              \
  static inline type *                                                        \
  M_F(name, back)(const deque_t d)                                            \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(d->count > 0);                                                   \
    size_t i = d->back->index;                                                \
    node_t *n = d->back->node;                                                \
    if (M_UNLIKELY(i == 0)) {                                                 \
      n = M_F(name, node_list, previous_obj)(d->list, n);                     \
      M_ASSERT(n != NULL);                                                    \
      i = n->size;                                                            \
    }                                                                         \
    return &n->data[i-1];                                                     \
  }                                                                           \
                                                                              \
  static inline type *                                                        \
  M_F(name, front)(const deque_t d)                                           \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(d->count > 0);                                                   \
    size_t i = d->front->index;                                               \
    node_t *n = d->front->node;                                               \
    if (M_UNLIKELY (n->size <= i)) {                                          \
      n = M_F(name, node_list, next_obj)(d->list, n);                         \
      M_ASSERT(n != NULL);                                                    \
      i = 0;                                                                  \
    }                                                                         \
    return &n->data[i];                                                       \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_F(name, M_NAMING_GET_SIZE)(const deque_t d)                               \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    return d->count;                                                          \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_F(name, capacity_back)(const deque_t v)                                   \
  {                                                                           \
    DEQUEI_CONTRACT(v);                                                       \
    size_t s = 0;                                                             \
    for (node_t *n = M_F(name, node_list, back)(v->list);                     \
         n != NULL;                                                           \
         n = (n == v->back->node) ? NULL :                                    \
          M_F(name, node_list, previous_obj)(v->list, n)) {                   \
      s += (n == v->back->node ? v->back->index : n->size);                   \
    }                                                                         \
    return s;                                                                 \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_F(name, capacity_front)(const deque_t v)                                  \
  {                                                                           \
    DEQUEI_CONTRACT(v);                                                       \
    size_t s = 0;                                                             \
    for (node_t *n = M_F(name, node_list, front)(v->list);                    \
         n != NULL;                                                           \
         n = (n == v->front->node) ? NULL :                                   \
          M_F(name, node_list, next_obj)(v->list, n) ){                       \
      s += n->size - (n == v->front->node ? v->front->index : 0);             \
    }                                                                         \
    return s;                                                                 \
  }                                                                           \
                                                                              \
  static inline size_t                                                        \
  M_F(name, capacity)(const deque_t v)                                        \
  {                                                                           \
    return M_F(name, capacity_back)(v) + M_F(name, capacity_front)(v);        \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_F(name, M_NAMING_TEST_EMPTY)(const deque_t d)                             \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    return d->count == 0;                                                     \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_IT_FIRST)(it_t it, const deque_t d)                      \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(it != NULL);                                                     \
    it->node  = d->front->node;                                               \
    it->index = d->front->index;                                              \
    it->deque = d;                                                            \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_IT_LAST)(it_t it, const deque_t d)                       \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(it != NULL);                                                     \
    it->node  = d->back->node;                                                \
    it->index = d->back->index - 1;                                           \
    it->deque = d;                                                            \
    if (M_UNLIKELY(it->index >= it->node->size)) {                            \
      it->node = M_F(name, node_list, previous_obj)(d->list, it->node);       \
      M_ASSERT(it->node != NULL && it->node->size > 1);                       \
      it->index = it->node->size-1;                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_IT_END)(it_t it, const deque_t d)                        \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT(it != NULL);                                                     \
    it->node  = d->back->node;                                                \
    it->index = d->back->index;                                               \
    it->deque = d;                                                            \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_IT_SET)(it_t it1, const it_t it2)                        \
  {                                                                           \
    M_ASSERT(it1 != NULL);                                                    \
    M_ASSERT(it2 != NULL);                                                    \
    it1->node  = it2->node;                                                   \
    it1->index = it2->index;                                                  \
    it1->deque = it2->deque;                                                  \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_F(name, M_NAMING_IT_TEST_END)(it_t it)                                    \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    return (it->node == it->deque->back->node                                 \
      && it->index >= it->deque->back->index)                                 \
      || (it->node == it->deque->front->node                                  \
    && it->index < it->deque->front->index);                                  \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, next)(it_t it)                                                    \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    node_t *n = it->node;                                                     \
    it->index++;                                                              \
    if (M_UNLIKELY(it->index >= n->size)) {                                   \
      n = M_F(name, node_list, next_obj)(it->deque->list, n);                \
      if (M_UNLIKELY(n == NULL)) {                                            \
        /* Point to 'end' (can't undo it) */                                  \
        it->node  = it->deque->back->node;                                    \
        it->index = it->deque->back->node->size;                              \
        return;                                                               \
      }                                                                       \
      it->node = n;                                                           \
      it->index = 0;                                                          \
    }                                                                         \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, previous)(it_t it)                                                \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    node_t *n = it->node;                                                     \
    it->index--;                                                              \
    if (M_UNLIKELY(it->index >= n->size)) {                                   \
      n = M_F(name, node_list, previous_obj)(it->deque->list, n);            \
      if (M_UNLIKELY(n == NULL)) {                                            \
        /* Point to 'end' (can't undo it) */                                  \
        it->node  = it->deque->back->node;                                    \
        it->index = it->deque->back->node->size;                              \
        return;                                                               \
      }                                                                       \
      it->node = n;                                                           \
      it->index = 0;                                                          \
    }                                                                         \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_F(name, M_NAMING_IT_TEST_LAST)(it_t it)                                   \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    it_t it2;                                                                 \
    M_F(name, M_NAMING_IT_SET)(it2, it);                                      \
    M_F(name, next)(it2);                                                     \
    return M_F(name, M_NAMING_IT_TEST_END)(it2);                              \
  }                                                                           \
                                                                              \
  static inline bool                                                          \
  M_F(name, M_NAMING_IT_TEST_EQUAL)(it_t it1, const it_t it2)                 \
  {                                                                           \
    M_ASSERT(it1 != NULL);                                                    \
    M_ASSERT(it2 != NULL);                                                    \
    return it1->deque == it2->deque                                           \
        && it1->node == it2->node                                             \
        && it1->index == it2->index;                                          \
  }                                                                           \
                                                                              \
  static inline type *                                                        \
  M_F(name, ref)(it_t it)                                                     \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    M_ASSERT(it->index < it->node->size);                                     \
    return &it->node->data[it->index];                                        \
  }                                                                           \
                                                                              \
  static inline type const *                                                  \
  M_F(name, cref)(it_t it)                                                    \
  {                                                                           \
    M_ASSERT(it != NULL);                                                     \
    M_ASSERT(it->index < it->node->size);                                     \
    return M_CONST_CAST(type, &it->node->data[it->index]);                    \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_INIT_WITH)(deque_t d, const deque_t src)                 \
  {                                                                           \
    DEQUEI_CONTRACT(src);                                                     \
    M_ASSERT(d != NULL);                                                      \
    M_F(name, node_list, M_NAMING_INITIALIZE)(d->list);                            \
    d->default_size = DEQUEUI_DEFAULT_SIZE + src->count;                      \
    d->count        = src->count;                                             \
    node_t *n = M_F(name, int, new_node)(d);                                 \
    if (n == NULL) return;                                                    \
    d->default_size /= 2;                                                     \
    M_F(name, node_list, push_back)(d->list, n);                             \
    d->front->node  = n;                                                      \
    d->front->index = DEQUEUI_DEFAULT_SIZE / 2;                               \
    d->back->node   = n;                                                      \
    d->back->index  = DEQUEUI_DEFAULT_SIZE / 2 + src->count;                  \
    it_t it;                                                                  \
    size_t i = DEQUEUI_DEFAULT_SIZE / 2;                                      \
    for (M_F(name, M_NAMING_IT_FIRST)(it, src);                               \
         !M_F(name, M_NAMING_IT_TEST_END)(it);                                \
         M_F(name, next)(it)) {                                               \
      type const *obj = M_F(name, cref)(it);                                  \
      M_CALL_INIT_SET(oplist, n->data[i], *obj);                              \
      i++;                                                                    \
      M_ASSERT(i <= d->back->index);                                          \
    }                                                                         \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_SET_AS)(deque_t d, deque_t const src)                    \
  {                                                                           \
    if (M_UNLIKELY(src == d))                                                 \
      return;                                                                 \
    /* TODO: Reuse memory of d! */                                            \
    M_F(name, M_NAMING_FINALIZE)(d);                                          \
    M_F(name, M_NAMING_INIT_WITH)(d, src);                                    \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, init_move)(deque_t d, deque_t src)                                \
  {                                                                           \
    DEQUEI_CONTRACT(src);                                                     \
    M_ASSERT(d != NULL);                                                      \
    M_F(name, node_list, init_move)(d->list, src->list);                     \
    d->front->node  = src->front->node;                                       \
    d->front->index = src->front->index;                                      \
    d->back->node   = src->back->node;                                        \
    d->back->index  = src->back->index;                                       \
    d->default_size = src->default_size;                                      \
    d->count        = src->count;                                             \
    memset(src, 0, sizeof(deque_t));                                          \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, move)(deque_t d, deque_t src)                                     \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    DEQUEI_CONTRACT(src);                                                     \
    M_F(name, M_NAMING_FINALIZE)(d);                                          \
    M_F(name, init_move)(d, src);                                             \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, swap)(deque_t d, deque_t e)                                       \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    DEQUEI_CONTRACT(e);                                                       \
    M_F(name, node_list, swap)(d->list, e->list);                            \
    M_SWAP(node_t *, d->front->node, e->front->node);                         \
    M_SWAP(node_t *, d->back->node, e->back->node);                           \
    M_SWAP(size_t, d->front->index, e->front->index);                         \
    M_SWAP(size_t, d->back->index, e->back->index);                           \
    M_SWAP(size_t, d->default_size, e->default_size);                         \
    M_SWAP(size_t, d->count, e->count);                                       \
    DEQUEI_CONTRACT(d);                                                       \
    DEQUEI_CONTRACT(e);                                                       \
  }                                                                           \
                                                                              \
  static inline type*                                                         \
  M_F(name, M_NAMING_GET)(deque_t d, size_t key)                              \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT_INDEX(key, d->count);                                            \
    const size_t index0 = d->front->index;                                    \
    size_t count = 0;                                                         \
    /* This loop is in log(N) since the size increase exponentially.*/        \
    for(node_t *n = d->front->node;                                           \
        n != NULL ;                                                           \
        n = (n == d->back->node) ? NULL :                                     \
          M_F(name, node_list, next_obj)(d->list, n) ){                      \
      if (index0 + key < count + n->size) {                                   \
        return &n->data[index0 + key - count];                                \
      }                                                                       \
      count += n->size;                                                       \
    }                                                                         \
    M_ASSERT(false);                                                          \
    return NULL;                                                              \
  }                                                                           \
                                                                              \
  static inline type const *                                                  \
  M_F(name, cget)(deque_t d, size_t key)                                      \
  {                                                                           \
    return M_CONST_CAST(type, M_F(name, M_NAMING_GET)(d, key));               \
  }                                                                           \
                                                                              \
  static inline void                                                          \
  M_F(name, M_NAMING_SET_AT)(deque_t d, size_t key, type const x)             \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT_INDEX(key, d->count);                                            \
    type *p = M_F(name, M_NAMING_GET)(d, key);                                \
    M_CALL_SET(oplist, *p, x);                                                \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
                                                                              \
  M_IF_METHOD(EQUAL, oplist)(                                                 \
  static inline bool                                                          \
  M_F(name, M_NAMING_TEST_EQUAL_TO)(const deque_t d1, const deque_t d2)       \
  {                                                                           \
    DEQUEI_CONTRACT(d1);                                                      \
    DEQUEI_CONTRACT(d2);                                                      \
    if (d1->count != d2->count)                                               \
      return false;                                                           \
    it_t it1;                                                                 \
    it_t it2;                                                                 \
    for(M_F(name, M_NAMING_IT_FIRST)(it1, d1),                                \
        M_F(name, M_NAMING_IT_FIRST)(it2, d2);                                \
        !M_F(name, M_NAMING_IT_TEST_END)(it1);                                \
        M_F(name, next)(it1), M_F(name, next)(it2)) {                         \
      type const *obj1 = M_F(name, cref)(it1);                                \
      type const *obj2 = M_F(name, cref)(it2);                                \
      if (M_CALL_EQUAL(oplist, *obj1, *obj2) == false)                        \
        return false;                                                         \
    }                                                                         \
    M_ASSERT(M_F(name, M_NAMING_IT_TEST_END)(it2));                           \
    return true;                                                              \
  }                                                                           \
  , /* NO EQUAL */)                                                           \
                                                                              \
  M_IF_METHOD(HASH, oplist)(                                                  \
  static inline size_t                                                        \
  M_F(name, hash)(const deque_t d)                                            \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_HASH_DECL(hash);                                                        \
    it_t it;                                                                  \
    for(M_F(name, M_NAMING_IT_FIRST)(it, d);                                  \
        !M_F(name, M_NAMING_IT_TEST_END)(it);                                 \
        M_F(name, next)(it)) {                                                \
      type const *obj = M_F(name, cref)(it);                                  \
      M_HASH_UP (hash, M_CALL_HASH(oplist, *obj));                            \
    }                                                                         \
    return M_HASH_FINAL(hash);                                                \
  }                                                                           \
  , /* NO HASH */)                                                            \
                                                                              \
  M_IF_METHOD(SWAP, oplist)(                                                  \
  static inline void                                                          \
  M_F(name, swap_at)(deque_t d, size_t i, size_t j)                           \
  {                                                                           \
    DEQUEI_CONTRACT(d);                                                       \
    M_ASSERT_INDEX(i, d->count);                                              \
    M_ASSERT_INDEX(j, d->count);                                              \
    type *obj1 = M_F(name, M_NAMING_GET)(d, i);                               \
    type *obj2 = M_F(name, M_NAMING_GET)(d, j);                               \
    M_CALL_SWAP(oplist, *obj1, *obj2);                                        \
    DEQUEI_CONTRACT(d);                                                       \
  }                                                                           \
  , /* NO SWAP */)                                                            \
                                                                              \
  M_IF_METHOD(GET_STR, oplist)(                                               \
  static inline void                                                          \
  M_F(name, get_str)(string_t str, deque_t const deque, bool append)          \
  {                                                                           \
    iM_STRING_CONTRACT(str);                                                    \
    DEQUEI_CONTRACT(deque);                                                   \
    (append ? M_F(string, M_NAMING_CONCATENATE_WITH, cstr) :                 \
              M_F(string, set, cstr)) (str, "[");                            \
    it_t it;                                                                  \
    for (M_F(name, M_NAMING_IT_FIRST)(it, deque) ;                            \
         !M_F(name, M_NAMING_IT_TEST_END)(it);                                \
         M_F(name, next)(it)){                                                \
      type const *item = M_F(name, cref)(it);                                 \
      M_CALL_GET_STR(oplist, str, *item, true);                               \
      if (!M_F(name, M_NAMING_IT_TEST_LAST)(it))                              \
        M_F(string, push_back)(str, M_GET_SEPARATOR oplist);                  \
    }                                                                         \
    M_F(string, push_back)(str, ']');                                         \
    iM_STRING_CONTRACT(str);                                                    \
  }                                                                           \
  , /* no GET_STR */ )                                                        \
                                                                              \
  M_IF_METHOD(OUT_STR, oplist)(                                               \
  static inline void                                                          \
  M_F(name, out_str)(FILE *file, const deque_t deque)                         \
  {                                                                           \
    DEQUEI_CONTRACT(deque);                                                   \
    M_ASSERT(file != NULL);                                                   \
    fputc ('[', file);                                                        \
    it_t it;                                                                  \
    for (M_F(name, M_NAMING_IT_FIRST)(it, deque) ;                            \
         !M_F(name, M_NAMING_IT_TEST_END)(it);                                \
         M_F(name, next)(it)) {                                               \
      type const *item = M_F(name, cref)(it);                                 \
      M_CALL_OUT_STR(oplist, file, *item);                                    \
      if (!M_F(name, M_NAMING_IT_TEST_LAST)(it))                              \
        fputc (M_GET_SEPARATOR oplist, file);                                 \
    }                                                                         \
    fputc (']', file);                                                        \
  }                                                                           \
  , /* no OUT_STR */ )                                                        \
                                                                              \
  M_IF_METHOD2(PARSE_CSTR, INIT, oplist)(                                     \
  static inline bool                                                          \
  M_F(name, parse, cstr)(deque_t deque, const char str[], const char **endp) \
  {                                                                           \
    DEQUEI_CONTRACT(deque);                                                   \
    M_ASSERT(str != NULL);                                                    \
    M_F(name, M_NAMING_CLEAN)(deque);                                         \
    bool success = false;                                                     \
    int c = *str++;                                                           \
    if (M_UNLIKELY (c != '[')) goto exit;                                     \
    c = *str++;                                                               \
    if (M_UNLIKELY (c == ']')) {success = true; goto exit;}                   \
    if (M_UNLIKELY (c == 0)) goto exit;                                       \
    str--;                                                                    \
    type item;                                                                \
    M_CALL_INIT(oplist, item);                                                \
    do {                                                                      \
      bool b = M_CALL_PARSE_CSTR(oplist, item, str, &str);                    \
      do { c = *str++; } while (isspace(c));                                  \
      if (b == false || c == 0) { goto exit_clear; }                          \
      M_F(name, push_back)(deque, item);                                      \
    } while (c == M_GET_SEPARATOR oplist);                                    \
    DEQUEI_CONTRACT(deque);                                                   \
    success = (c == ']');                                                     \
  exit_clear:                                                                 \
    M_CALL_CLEAR(oplist, item);                                               \
  exit:                                                                       \
    if (endp) *endp = str;                                                    \
    return success;                                                           \
  }                                                                           \
  , /* no PARSE_CSTR */ )                                                     \
                                                                              \
  M_IF_METHOD2(IN_STR, INIT, oplist)(                                         \
  static inline bool                                                          \
  M_F(name, in_str)(deque_t deque, FILE *file)                                \
  {                                                                           \
    DEQUEI_CONTRACT(deque);                                                   \
    M_ASSERT(file != NULL);                                                   \
    M_F(name, M_NAMING_CLEAN)(deque);                                         \
    int c = fgetc(file);                                                      \
    if (M_UNLIKELY (c != '[')) return false;                                  \
    c = fgetc(file);                                                          \
    if (M_UNLIKELY (c == ']')) return true;                                   \
    if (M_UNLIKELY (c == EOF)) return false;                                  \
    ungetc(c, file);                                                          \
    type item;                                                                \
    M_CALL_INIT(oplist, item);                                                \
    do {                                                                      \
      bool b = M_CALL_IN_STR(oplist, item, file);                             \
      do { c = fgetc(file); } while (isspace(c));                             \
      if (b == false || c == EOF) { break; }                                  \
      M_F(name, push_back)(deque, item);                                      \
    } while (c == M_GET_SEPARATOR oplist);                                    \
    M_CALL_CLEAR(oplist, item);                                               \
    DEQUEI_CONTRACT(deque);                                                   \
    return c == ']';                                                          \
  }                                                                           \
  , /* no IN_STR */ )                                                         \
                                                                              \
  M_IF_METHOD(OUT_SERIAL, oplist)(                                            \
  static inline m_serial_return_code_t                                        \
  M_F(name, out_serial)(m_serial_write_t f, const deque_t deque)              \
  {                                                                           \
    DEQUEI_CONTRACT(deque);                                                   \
    M_ASSERT (f != NULL && f->m_interface != NULL);                           \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    bool first_done = false;                                                  \
    ret = f->m_interface->write_array_start(local, f, deque->count);          \
    M_T(name, it_ct) it;                                                      \
    for (M_F(name, M_NAMING_IT_FIRST)(it, deque) ;                            \
         !M_F(name, M_NAMING_IT_TEST_END)(it);                                \
         M_F(name, next)(it)){                                                \
      type const *item = M_F(name, cref)(it);                                 \
      if (first_done)                                                         \
        ret |= f->m_interface->write_array_next(local, f);                    \
      ret |= M_CALL_OUT_SERIAL(oplist, f, *item);                             \
      first_done = true;                                                      \
    }                                                                         \
    ret |= f->m_interface->write_array_end(local, f);                         \
    return ret & M_SERIAL_FAIL;                                               \
  }                                                                           \
  , /* no OUT_SERIAL */ )                                                     \
                                                                              \
  M_IF_METHOD2(IN_SERIAL, INIT, oplist)(                                      \
  static inline m_serial_return_code_t                                        \
  M_F(name, in, serial)(deque_t deque, m_serial_read_t f)                    \
  {                                                                           \
    DEQUEI_CONTRACT(deque);                                                   \
    M_ASSERT(f != NULL && f->m_interface != NULL);                            \
    m_serial_local_t local;                                                   \
    m_serial_return_code_t ret;                                               \
    size_t estimated_size = 0;                                                \
    M_F(name, M_NAMING_CLEAN)(deque);                                         \
    ret = f->m_interface->read_array_start(local, f, &estimated_size);        \
    if (M_UNLIKELY(ret != M_SERIAL_OK_CONTINUE)) return ret;                  \
    type item;                                                                \
    M_CALL_INIT(oplist, item);                                                \
    do {                                                                      \
      ret = M_CALL_IN_SERIAL(oplist, item, f);                                \
      if (ret != M_SERIAL_OK_DONE) { break; }                                 \
      M_F(name, push_back)(deque, item);                                      \
      ret = f->m_interface->read_array_next(local, f);                        \
    } while (ret == M_SERIAL_OK_CONTINUE);                                    \
    M_CALL_CLEAR(oplist, item);                                               \
    return ret;                                                               \
  }                                                                           \
  , /* no IN_SERIAL & INIT */ )                                               \
                                                                              \

/* Deferred evaluation for the oplist definition,
   so that all arguments are evaluated before further expansion */
#define DEQUEI_OPLIST_P1(arg) DEQUEI_OPLIST_P2 arg

/* Validation of the given oplist */
#define DEQUEI_OPLIST_P2(name, oplist)                                         \
  M_IF_OPLIST(oplist)(DEQUEI_OPLIST_P3, DEQUEI_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define DEQUEI_OPLIST_FAILURE(name, oplist)                                    \
  ((M_LIB_ERROR(ARGUMENT_OF_DEQUE_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

/* OPLIST definition of a deque */
#define DEQUEI_OPLIST_P3(name, oplist)                                 \
  (INIT(M_F(name, M_NAMING_INITIALIZE)),                                   \
   INIT_SET(M_F(name, M_NAMING_INIT_WITH)),                            \
   INIT_WITH(API_1(M_INIT_VAI)),                                               \
   SET(M_F(name, M_NAMING_SET_AS)),                                      \
   CLEAR(M_F(name, M_NAMING_FINALIZE)),                              \
   INIT_MOVE(M_F(name, init_move)),                                    \
   MOVE(M_F(name, move)),                                          \
   SWAP(M_F(name, swap)),                                          \
   TYPE(M_T(name, ct)),                                            \
   SUBTYPE(M_T(name, subtype_ct)),                                   \
   EMPTY_P(M_F(name, M_NAMING_TEST_EMPTY)),                                 \
   IT_TYPE(M_T(name, it_ct)),                                      \
   IT_FIRST(M_F(name, M_NAMING_IT_FIRST)),                             \
   IT_LAST(M_F(name, M_NAMING_IT_LAST)),                           \
   IT_END(M_F(name, M_NAMING_IT_END)),                               \
   IT_SET(M_F(name, M_NAMING_IT_SET)),                               \
   IT_END_P(M_F(name, M_NAMING_IT_TEST_END)),                          \
   IT_LAST_P(M_F(name, M_NAMING_IT_TEST_LAST)),                            \
   IT_EQUAL_P(M_F(name, M_NAMING_IT_TEST_EQUAL)),                          \
   IT_NEXT(M_F(name, next)),                                       \
   IT_PREVIOUS(M_F(name, previous)),                                 \
   IT_REF(M_F(name, ref)),                                           \
   IT_CREF(M_F(name, cref)),                                       \
   CLEAN(M_F(name, M_NAMING_CLEAN)),                               \
   GET_SIZE(M_F(name, M_NAMING_GET_SIZE)),                                         \
   PUSH(M_F(name, push_back)),                                       \
   POP(M_F(name, pop_back)),                                       \
   OPLIST(oplist),                                                             \
   M_IF_METHOD(GET_STR, oplist)(GET_STR(M_F(name, get_str)),),               \
   M_IF_METHOD(PARSE_CSTR, oplist)(PARSE_CSTR(M_F(name, parse_cstr)),),           \
   M_IF_METHOD(OUT_STR, oplist)(OUT_STR(M_F(name, out_str)),),               \
   M_IF_METHOD(IN_STR, oplist)(IN_STR(M_F(name, in_str)),),                  \
   M_IF_METHOD(OUT_SERIAL, oplist)(OUT_SERIAL(M_F(name, out_serial)),),        \
   M_IF_METHOD(IN_SERIAL, oplist)(IN_SERIAL(M_F(name, in_serial)),),           \
   M_IF_METHOD(EQUAL, oplist)(EQUAL(M_F(name, M_NAMING_TEST_EQUAL_TO)),),        \
   M_IF_METHOD(HASH, oplist)(HASH(M_F(name, hash)),),                    \
   M_IF_METHOD(NEW, oplist)(NEW(M_GET_NEW oplist),),                           \
   M_IF_METHOD(REALLOC, oplist)(REALLOC(M_GET_REALLOC oplist),),               \
   M_IF_METHOD(DEL, oplist)(DEL(M_GET_DEL oplist),)                            \
   )

#endif // MSTARLIB_DEQUE_H
