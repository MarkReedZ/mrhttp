
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "assoc.h"
#include "city.h"

typedef  unsigned long  int  ub4;
typedef  unsigned       char ub1;

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)


extern char *strndup(const char *s, size_t n);

Assoc_t *assoc_create(void) {
  Assoc_t *t = malloc( sizeof *t );
  if (!t) {
    fprintf(stderr, "Failed to init hashtable.\n");
    exit(-1);
  }
  t->hashpower = 16;
  t->buckets = malloc(sizeof(*t->buckets) * hashsize(t->hashpower));
  for (int i = 0; i < hashsize(t->hashpower); i++) t->buckets[i] = NULL;
  return t;
}

char *assoc_get(Assoc_t *t, const char *key, const size_t nkey) { //, const uint32_t hv) {
  item *it;
  unsigned long hv = CityHash64(key, nkey);

  it = t->buckets[hv & hashmask(t->hashpower)];

  while (it) {
    //keys are the same size  if ((nkey == it->nkey) && (memcmp(key, ITEM_key(it), nkey) == 0)) {
    // memcmp is fast at powers of 2, but we can do 3x better otherwise or make all keys a power of 2 len
    if ((memcmp(key, it->key, nkey) == 0)) {
      return it->val;
    }
    it = it->hnext;
  }
  return NULL;
}

/* 
static void assoc_expand(void) {
    old_hashtable = primary_hashtable;

    primary_hashtable = calloc(hashsize(hashpower + 1), sizeof(void *));
    if (primary_hashtable) {
        if (settings.verbose > 1)
            fprintf(stderr, "Hash table expansion starting\n");
        hashpower++;
        expanding = true;
        expand_bucket = 0;
        STATS_LOCK();
        stats_state.hash_power_level = hashpower;
        stats_state.hash_bytes += hashsize(hashpower) * sizeof(void *);
        stats_state.hash_is_expanding = true;
        STATS_UNLOCK();
    } else {
        primary_hashtable = old_hashtable;
    }
}
*/

/* Note: this isn't an assoc_update.  The key must not already exist to call this */
int assoc_insert(Assoc_t *t, const char *key, const size_t nkey, const char *val, const size_t nval) {
  item *it;
  it = (item*)malloc(sizeof *it);
  if ( it == NULL ) return -1;
  it->key = strndup(key, nkey);
  if ( it->key == NULL ) { free(it); return -1; }
  it->val = strndup(val, nval);
  if ( it->val == NULL ) { free(it->key); free(it); return -1; }

  //printf("key %.*s val %.*s\n", nkey, it->key, nval, it->val );

  unsigned long hv = CityHash64(key, nkey);
  it->hnext = t->buckets[hv & hashmask(t->hashpower)];
  t->buckets[hv & hashmask(t->hashpower)] = it;
  return 1;
}

void assoc_free(item *it) {
  free(it->key);
  free(it->val);
  free(it);
}

void  assoc_delete(Assoc_t *t, const char *key, const size_t nkey) {
  unsigned long hv = CityHash64(key, nkey);
  item *it = t->buckets[hv & hashmask(t->hashpower)];
  item *prev = NULL;

  //while ( it && ((nkey != (*pos)->nkey) || memcmp(key, ITEM_key(*pos), nkey))) {
  while ( it && memcmp(key, it->key, nkey)) {
    prev = it;
    it = it->hnext;
  }

  if ( prev ) {
    prev->hnext = it->hnext;
    assoc_free(it);
  } else {
    t->buckets[hv & hashmask(t->hashpower)] = NULL;
  }

}

