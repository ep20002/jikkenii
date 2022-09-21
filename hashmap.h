#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct hashmap hashmap;

hashmap *hnew(void);

void hinsert(hashmap *hmap, char *key, int value);

int hget(hashmap *hmap, char *key);

void hdestroy(hashmap *hmap);

#endif
