#include "trie.h"
#include "zvec.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Allocate a trie node */
trie *maketrie() {
  trie *self = malloc(sizeof(trie));
	self->children = NULL;
	self->keys = NULL;
  self->final = false;
  return self;
}

/* Free the allocated trie recursively. So it frees all children and keys. */
void freetrie(trie *root) {
  if (!root) return;
	for (size_t i = 0; i < veclen(root->children); i++) {
		freetrie(root->children[i]);
	}
	vecfree(root->children);
	vecfree(root->keys);
  free(root);
}

/* Get the child, if exists, of the corresponding key */
trie *triech(trie *self, char key) {
  if (!self) return self;
  for (size_t i = 0; i < veclen(self->keys); i++) {
    if (self->keys[i] == key) return self->children[i];
  }
  return NULL;
}

/* Insert a string into the trie.
 * It marks the last node as final.*/
void trieinsert(trie *root, char *token) {
  trie *curr = root;
  for (char *ch = token; *ch; ch++) {
    trie *child = triech(curr, *ch);
    if (!child) {
      child = maketrie();
      vecpush(curr->children, child);
      vecpush(curr->keys, *ch);
    }
    curr = child;
  }
  curr->final = true;
}

/* It walks into the trie to check if the given string exists. */
bool triesearch(trie *root, char *token) {
  trie *curr = root;
  for (char *ch = token; *ch; ch++) {
    trie *child = triech(curr, *ch);
    if (!child) return false;
    curr = child;
  }
  if (!curr->final) return false;
  return curr->final;
}

/* This function return the next token of the string.
* It return an heap allocation so the returning pointer must be freed manually.
* If it found a valid token advance the pointer and return the token.
* Otherwise it keeps the pointer as is and return null.*/
char *trietok(trie *root, char **source) {
  char *original = *source;
  trie *curr = root;
  char *last = NULL;

  char *start = *source;
  while (curr && **source) {
    curr = triech(curr, **source);
    if (!curr) break;
    (*source)++;
    if (curr->final) {
      last = *source;
      if (veclen(curr->children) == 0) break;
    }
  }
  if (!last || last == start) {
    *source = original;
    return NULL;
  }

  return strndup(start, last - start);
}
