#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Node {
  long long id;
  char productId[32];
  char category[32];
  char name[64];
  int quantity;
  char date[16];
  int flag;
  struct Node *next;
} Node;

extern Node *head;

/* 节点创建 */
Node *createNode(const char *productId, const char *category, const char *name,
                 int quantity, const char *date, int flag);

Node *createNodeWithId(long long id, const char *productId,
                       const char *category, const char *name, int quantity,
                       const char *date, int flag);

/* 链表操作 */
void insertNode(Node *node);
void freeList(void);
void printList(void);

/* 查询 */
Node *findById(long long id);
Node *findByProductId(const char *productId);
Node *findByName(const char *name);

/* 删除 */
int deleteById(long long id);

#endif
