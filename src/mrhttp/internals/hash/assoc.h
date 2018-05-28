#pragma once

#include <stddef.h>

typedef struct _item {
  // lru ll
  //struct _item *lnext;
  //struct _item *lprev;
  // hash chain
  struct _item *hnext;
  unsigned int hv;
  char *key;
  char *val;
  int nkey;
  int nval;
} item;

typedef struct HashTable
{
    int hashpower;
    item **buckets;
} Assoc_t;

/* associative array */
Assoc_t *assoc_create(void);
Assoc_t *assoc_init(void);

char *assoc_get(   Assoc_t *t, const char *key, const size_t nkey);
int   assoc_insert(Assoc_t *t, const char *key, const size_t nkey, const char *val, const size_t nval);
void  assoc_delete(Assoc_t *t, const char *key, const size_t nkey);
//char *assoc_get(   Assoc_t *t, const char *key, const size_t nkey, const uint32_t hv);
//int   assoc_insert(Assoc_t *t, const char *key, const size_t nkey, const char *val, const size_t nval, const uint32_t hv);
//void  assoc_delete(Assoc_t *t, const char *key, const size_t nkey, const uint32_t hv);
