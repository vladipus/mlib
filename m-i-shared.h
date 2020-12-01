/*
 * M*LIB - INTRUSIVE SHARED PTR Module
 *
 * Copyright (c) 2017-2020, Patrick Pelissier
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
#ifndef MSTARLIB_I_SHARED_PTR_H
#define MSTARLIB_I_SHARED_PTR_H

#include "m-core.h"
#include "m-atomic.h"
#include "m-mutex.h"

M_BEGIN_PROTECTED_CODE

/* Define the oplist of a intrusive shared pointer.
   USAGE: ISHARED_OPLIST(name [, oplist_of_the_type]) */
#define ISHARED_PTR_OPLIST(...)                                      \
  ISHAREDI_PTR_OPLIST_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                 \
                         ((__VA_ARGS__, M_DEFAULT_OPLIST),           \
                          (__VA_ARGS__ )))

/**
 * @brief Interface to add to a structure to allow intrusive support.
 *
 * @param name The name of the intrusive shared pointer.
 * @param type The name of the type of the structure (aka. struct test_s) - not used currently.
 * NOTE: There can be only one interface of this kind in a type!
 */
#define ISHARED_PTR_INTERFACE(name, type)       \
  atomic_int M_C(name, _cpt)

/**
 * @brief Define the intrusive shared pointer type and its static inline functions.
 *
 * USAGE: ISHARED_PTR_DEF(name, type, [, oplist])
 *
 * @param name The name of the intrusive shared pointer.
 */
#define ISHARED_PTR_DEF(name, ...)                                      \
  ISHAREDI_PTR_DEF_P1(M_IF_NARGS_EQ1(__VA_ARGS__)                       \
                      ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__)() ), \
                       (name, __VA_ARGS__ )))


/********************************** INTERNAL ************************************/

// Deferred evaluation
#define ISHAREDI_PTR_OPLIST_P1(arg) ISHAREDI_PTR_OPLIST_P2 arg

/* Validation of the given oplist */
#define ISHAREDI_PTR_OPLIST_P2(name, oplist)				\
  M_IF_OPLIST(oplist)(ISHAREDI_PTR_OPLIST_P3, ISHAREDI_PTR_OPLIST_FAILURE)(name, oplist)

/* Prepare a clean compilation failure */
#define ISHAREDI_PTR_OPLIST_FAILURE(name, oplist)			\
  ((M_LIB_ERROR(ARGUMENT_OF_ISHARED_PTR_OPLIST_IS_NOT_AN_OPLIST, name, oplist)))

// Define the oplist
#define ISHAREDI_PTR_OPLIST_P3(name, oplist) (                          \
  INIT(M_INIT_DEFAULT),                                                 \
  INIT_SET(API_4(M_F(name, M_NAMING_INIT_SET))),				                \
  SET(M_F(name, M_NAMING_SET) M_IPTR),						                      \
  CLEAR(M_F(name, M_NAMING_CLEAR)),						                          \
  CLEAN(M_F(name, M_NAMING_CLEAN) M_IPTR),					                    \
  TYPE(M_C(name, _t)),                                                  \
  OPLIST(oplist),                                                       \
  SUBTYPE(M_C(name, _type_t))						                                \
  ,M_IF_METHOD(NEW, oplist)(NEW(M_GET_NEW oplist),)                     \
  ,M_IF_METHOD(REALLOC, oplist)(REALLOC(M_GET_REALLOC oplist),)         \
  ,M_IF_METHOD(DEL, oplist)(DEL(M_GET_DEL oplist),)                     \
  )

// Deferred evaluation
#define ISHAREDI_PTR_DEF_P1(arg) ISHAREDI_PTR_DEF_P2 arg

/* Validate the oplist before going further */
#define ISHAREDI_PTR_DEF_P2(name, type, oplist)                         \
  M_IF_OPLIST(oplist)(ISHAREDI_PTR_DEF_P3, ISHAREDI_PTR_DEF_FAILURE)(name, type, oplist)

/* Stop processing with a compilation failure */
#define ISHAREDI_PTR_DEF_FAILURE(name, type, oplist)   \
  M_STATIC_FAILURE(M_LIB_NOT_AN_OPLIST, "(ISHARED_PTR_DEF): the given argument is not a valid oplist: " #oplist)

#define ISHAREDI_PTR_DEF_P3(name, type, oplist)                         \
                                                                        \
  typedef type *M_T(name, t);						                                \
  typedef type M_T(name, type_t);					                              \
                                                                        \
  M_CHECK_COMPATIBLE_OPLIST(name, 1, type, oplist)                      \
                                                                        \
  static inline M_T(name, t)                                            \
  M_F(name, M_NAMING_INIT)(type *ptr)						                        \
  {									                                                    \
    if (M_LIKELY(ptr != NULL))                                          \
      atomic_init(&ptr->M_C(name, _cpt), 1);                            \
    return ptr;                                                         \
  }									                                                    \
                                                                        \
  									                                                    \
  static inline M_T(name, t)                                            \
  M_F(name, M_NAMING_INIT_SET)(M_T(name, t) shared)				              \
  {									                                                    \
    if (M_LIKELY(shared != NULL)) {                                     \
      int n = atomic_fetch_add(&(shared->M_C(name, _cpt)), 1);	      	\
      (void) n;								                                          \
    }									                                                  \
    return shared;                                                      \
  }									                                                    \
  									                                                    \
  static inline void M_ATTR_DEPRECATED                                  \
  M_F(name, M_C(M_NAMING_INIT_SET, 2))(M_T(name, t) *ptr,               \
                                       M_T(name, t) shared)             \
  {									                                                    \
    assert (ptr != NULL);                                               \
    *ptr = M_F(name, M_NAMING_INIT_SET)(shared);				                \
  }									                                                    \
  									                                                    \
  M_IF_DISABLED_METHOD(NEW, oplist)                                           \
  (                                                                           \
    static inline M_T(name, t)                                                \
    M_F3(_, name, init_static_once)()						                              \
    {                                                                         \
      M_CALL_INIT(oplist, *ptr);                                              \
    }                                                                         \
    static inline M_T(name, t)                                                \
    M_F3(name, M_NAMING_INIT, once)(type *ptr)						                    \
    {									                                                        \
      if (M_LIKELY(ptr != NULL)) {                                            \
        m_oncei_call(M_C3(_, name, _once), M_C3(_, name, _init_static_once)); \
        if (atomic_fetch_add(&(ptr->M_C(name, _cpt)), 1) == 0) {              \
          M_CALL_INIT(oplist, *ptr);                                          \
        }                                                                     \
        m_mutex_unlock(ptr->M_C(name, _mtx));                                 \
      }                                                                       \
      return ptr;                                                             \
    }									                                                        \
                                                                              \
    static inline bool                                                  \
    M_F(name, M_NAMING_TEST_NULL)(type *ptr)						                \
    {									                                                  \
      return (ptr == NULL) ||                                           \
             (atomic_load(&(ptr->M_C(name, _cpt))) == 0);               \
    }									                                                  \
    ,                                                                   \
    M_IF_METHOD(INIT, oplist)                                           \
    (                                                                   \
      static inline M_T(name, t)                                        \
      M_F(name, M_NAMING_INIT_NEW)(void)                                \
      {									                                                \
        type *ptr = M_CALL_NEW(oplist, type);                           \
        if (ptr == NULL) {                                              \
          M_MEMORY_FULL(sizeof(type));                                  \
          return NULL;                                                  \
        }                                                               \
        M_CALL_INIT(oplist, *ptr);                                      \
        atomic_init (&ptr->M_C(name, _cpt), 1);                         \
        return ptr;                                                     \
      }									                                                \
    , /* End of INIT */)                                                \
    /* End of NEW */)                                                   \
  									                                                    \
  static inline void				                                            \
  M_F(name, M_NAMING_CLEAR)(M_T(name, t) shared)                        \
  {									                                                    \
    if (shared != NULL)	{						                                    \
      if (atomic_fetch_sub(&(shared->M_C(name, _cpt)), 1) == 1)	{       \
        M_CALL_CLEAR(oplist, *shared);                                  \
        M_IF_DISABLED_METHOD(DEL, oplist)(, M_CALL_DEL(oplist, shared);)\
      }									                                                \
    }                                                                   \
  }								                                                    	\
  									                                                    \
  static inline void				                                            \
  M_F3(name, M_NAMING_CLEAR, ptr)(M_T(name, t) *shared)                 \
  {									                                                    \
    assert(shared != NULL);                                             \
    M_F(name, M_NAMING_CLEAR)(*shared);                                 \
    *shared = NULL;                                                     \
  }									                                                    \
  									                                                    \
  static inline void				                                            \
  M_F(name, M_NAMING_CLEAN)(M_T(name, t) *shared)                       \
  {									                                                    \
    M_F(name, M_NAMING_CLEAR)(*shared);						                      \
    *shared = NULL;                                                     \
  }                                                                     \
                                                                        \
  static inline void				                                            \
  M_F(name, M_NAMING_SET)(M_T(name, t) *ptr, M_T(name, t) shared)       \
  {									                                                    \
    assert(ptr != NULL);                                                \
    if (M_LIKELY (*ptr != shared)) {                                    \
      M_F(name, M_NAMING_CLEAR)(*ptr);						                      \
      *ptr = M_F(name, M_NAMING_INIT_SET)(shared);				              \
    }                                                                   \
  }

M_END_PROTECTED_CODE

#endif
