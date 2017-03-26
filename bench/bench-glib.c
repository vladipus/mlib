#define NDEBUG

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <glib.h>

static unsigned long long
cputime (void)
{
  struct rusage rus;
  getrusage (0, &rus);
  return rus.ru_utime.tv_sec * 1000000ULL + rus.ru_utime.tv_usec;
}

static unsigned int randValue = 0;
static void rand_init(void)
{
  randValue = 0;
}
static unsigned int rand_get(void)
{
  randValue = randValue * 31421U + 6927U;
  return randValue;
}

static void test_function(const char str[], size_t n, void (*func)(size_t))
{
  unsigned long long start, end;
  //  (*func)(n);
  start = cputime();
  (*func)(n);
  end = cputime();
  end = (end - start) / 1000U;
  printf ("%s %Lu ms for n = %lu\n", str, end, n);
}

/********************************************************************************************/

static void test_array(size_t n)
{
  rand_init();
  GArray *a1, *a2;
  a1 = g_array_new (FALSE, FALSE, sizeof (unsigned int));
  a2 = g_array_new (FALSE, FALSE, sizeof (unsigned int));
  for(size_t i = 0; i < n; i++) {
    unsigned int v = rand_get();
    g_array_append_val(a1, v );
    v = rand_get();
    g_array_append_val(a2, v );
  }
  unsigned int s = 0;
  for(unsigned long i = 0; i < n; i++) {
    s += g_array_index(a1, unsigned int, i ) * g_array_index(a2, unsigned int, i );
  }
  g_array_free(a1, FALSE);
  g_array_free(a2, FALSE);
}

/********************************************************************************************/

static void test_list (size_t n)
{
  rand_init();
  GSList *a1 = NULL, *a2 = NULL;

  for(size_t i = 0; i < n; i++) {
    a1 = g_slist_prepend (a1, GINT_TO_POINTER (rand_get() ));
    a2 = g_slist_prepend (a2, GINT_TO_POINTER (rand_get() ));
  }
  unsigned int s = 0;
  for(GSList *it1 = a1, *it2 = a2 ; it1 != NULL; it1 = g_slist_next(it1), it2 = g_slist_next(it2) ) {
    s += GPOINTER_TO_INT(it1->data) * GPOINTER_TO_INT(it2->data);
    n--;
  }
  g_slist_free(a1);
  g_slist_free(a2);
}

/********************************************************************************************/

static int compare(gconstpointer a, gconstpointer b, gpointer user_data)
{
  const unsigned long *pa = a;
  const unsigned long *pb = b;
  (void) user_data;
  return (*pa < *pb) ? -1 : (*pa > *pb);
}

static void test_rbtree(size_t n)
{
  rand_init();
  GTree *tree = g_tree_new_full(compare, NULL, free, free);
  for (size_t i = 0; i < n; i++) {
    // NOTE: unsigned long are not designed to work with GPOINTER_TO_INT according to documentation.
    unsigned long *p = malloc(sizeof(unsigned long));
    *p = rand_get();
    g_tree_insert(tree, p, NULL);
  }
  unsigned int s = 0;
  for (size_t i = 0; i < n; i++) {
    unsigned long key = rand_get();
    unsigned long *p = g_tree_lookup(tree, &key);
    if (p)
      s += *p;
  }
  g_tree_destroy(tree);
}

/********************************************************************************************/
static guint hash_func (gconstpointer a)
{
  const unsigned long *pa = a;  
  return *pa;
}

static gboolean equal_func(gconstpointer a, gconstpointer b)
{
  const unsigned long *pa = a;
  const unsigned long *pb = b;
  return *pa == *pb;
}
          
static void
test_dict(unsigned long  n)
{
  rand_init();
  GHashTable *dict = g_hash_table_new_full(hash_func, equal_func, free, free);
  for (size_t i = 0; i < n; i++) {
    unsigned long *key = malloc(sizeof(unsigned long));
    unsigned long *value = malloc(sizeof(unsigned long));
    *key = rand_get();
    *value = rand_get();   
    g_hash_table_insert(dict, key, value );
  }    
  unsigned int s = 0;
  for (size_t i = 0; i < n; i++) {
    unsigned long key = rand_get();
    unsigned long *p = g_hash_table_lookup(dict, &key);
    if (p)
      s += *p;
  }
  g_hash_table_destroy(dict);
}

/********************************************************************************************/

typedef char char_array_t[256];

static gboolean char_equal (gconstpointer a, gconstpointer b)
{
  const char_array_t const *pa = (const char_array_t const *)a;
  const char_array_t const *pb = (const char_array_t const *)b;
  return strcmp(*pa,*pb)==0;
}

static guint char_hash(gconstpointer a)
{
  const char_array_t *pa = (const char_array_t const *)a;
  const char *s = *pa;
  guint hash = 0;
  while (*s) hash = hash * 31421 + (*s++) + 6927;
  return hash;
}

static void
test_dict_big(unsigned long  n)
{
  rand_init();
  GHashTable *dict = g_hash_table_new_full(char_hash, char_equal, free, free); 
  for (size_t i = 0; i < n; i++) {
    char_array_t *key = malloc (sizeof (char_array_t));
    char_array_t *value = malloc (sizeof (char_array_t));
    sprintf(*key, "%u", rand_get());
    sprintf(*value, "%u", rand_get());
    g_hash_table_insert(dict, key, value );
  }
  unsigned int s = 0;
  for (size_t i = 0; i < n; i++) {
    char_array_t s1;
    sprintf(s1, "%u", rand_get());
    char_array_t *p = g_hash_table_lookup(dict, &s1);
    if (p)
      s ++;
  }
  g_hash_table_destroy(dict);
}

/********************************************************************************************/

int main(int argc, const char *argv[])
{
  int n = (argc > 1) ? atoi(argv[1]) : 0;
  if (n == 1)
    test_function("List   time",10000000, test_list);
  if (n == 2)
    test_function("Array  time", 100000000, test_array);
  if (n == 3)
    test_function("Rbtree time", 1000000, test_rbtree);
  if (n == 4)
    test_function("Dict   time", 1000000, test_dict);
  if (n == 6)
    test_function("DictB  time", 1000000, test_dict_big);
}
