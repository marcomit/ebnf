
#ifndef TRIE_H
#define TRIE_H

#include "glob.h"
#include "stdbool.h"

typedef struct trie {
  char *keys;
  struct trie **children;
  bool final;
} trie;

trie *maketrie();
void freetrie(trie *);
void trieinsert(trie *, char *);
bool triesearch(trie *, char *);
char *trietok(trie *, char **);

#endif // TRIE_H
