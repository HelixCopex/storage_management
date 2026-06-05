#include "data.h"

Node *head = NULL;

/* 时间戳生成唯一ID */
static long long generateId() {
  return (long long)time(NULL) * 1000LL + rand() % 1000;
}

Node *createNode(const char *productId, const char *category, const char *name,
                 int quantity, const char *date, int flag) {
  Node *node = malloc(sizeof(Node));

  if (node == NULL)
    return NULL;

  node->id = generateId();

  strcpy(node->productId, productId);
  strcpy(node->category, category);
  strcpy(node->name, name);

  node->quantity = quantity;

  strcpy(node->date, date);

  node->flag = flag;
  node->next = NULL;

  return node;
}

Node *createNodeWithId(long long id, const char *productId,
                       const char *category, const char *name, int quantity,
                       const char *date, int flag) {
  Node *node = malloc(sizeof(Node));

  if (node == NULL)
    return NULL;

  node->id = id;

  strcpy(node->productId, productId);
  strcpy(node->category, category);
  strcpy(node->name, name);

  node->quantity = quantity;

  strcpy(node->date, date);

  node->flag = flag;

  node->next = NULL;

  return node;
}

void insertNode(Node *node) {
  if (head == NULL) {
    head = node;
    return;
  }

  Node *p = head;

  while (p->next)
    p = p->next;

  p->next = node;
}

Node *findById(long long id) {
  Node *p = head;

  while (p) {
    if (p->id == id)
      return p;

    p = p->next;
  }

  return NULL;
}

Node *findByProductId(const char *productId) {
  Node *p = head;

  while (p) {
    if (strcmp(p->productId, productId) == 0)
      return p;

    p = p->next;
  }

  return NULL;
}

Node *findByName(const char *name) {
  Node *p = head;

  while (p) {
    if (strcmp(p->name, name) == 0)
      return p;

    p = p->next;
  }

  return NULL;
}

int deleteById(long long id) {
  Node *p = head, *prev = NULL;
  while (p) {
    if (p->id == id) {
      if (prev)
        prev->next = p->next;
      else
        head = p->next;
      free(p);
      return 1;
    }
    prev = p;
    p = p->next;
  }
  return 0;
}

void printList() {
  Node *p = head;

  while (p) {
    printf("%lld %s %s %s %d %s %d\n", p->id, p->productId, p->category,
           p->name, p->quantity, p->date, p->flag);

    p = p->next;
  }
}

void freeList() {
  Node *p = head;

  while (p) {
    Node *tmp = p;
    p = p->next;
    free(tmp);
  }

  head = NULL;
}
