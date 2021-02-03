#pragma once

#include <assert.h>

typedef struct ZNode {
  struct ZNode* pprev;
  struct ZNode* pnext;
  void* val;
} ZNode;

void remove_self(ZNode* node) {
  assert(node);
  if (node->pprev) node->pprev->pnext = node->pnext;
  if (node->pnext) node->pnext->pprev = node->pprev;
  node->pnext = NULL;
  node->pprev = NULL;
}

int is_linked(ZNode* node) {
  assert(node);
  return (node->pprev || node->pprev);
}

void insert_before(ZNode* node_on_list, ZNode* node) {
  assert(node_on_list && node);
  node->pprev = node_on_list->pprev;
  node_on_list->pprev->pnext = node;
  node->pnext = node_on_list;
  node_on_list->pprev = node;
}

void insert_after(ZNode* node_on_list, ZNode* node) {
  assert(node_on_list && node);
  node->pnext = node_on_list->pnext;
  node_on_list->pnext->pprev = node;
  node->pprev = node_on_list;
  node_on_list->pnext = node;
}

struct ZList {}
