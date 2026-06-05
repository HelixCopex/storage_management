#include "modify.h"

int modifyRecordById(long long id, const char *productId, const char *category,
                     const char *name, int quantity, const char *date,
                     int flag) {
  Node *node = findById(id);

  if (node == NULL)
    return 0;

  if (!validateQuantity(quantity))
    return 0;

  if (!validateDate(date))
    return 0;

  if (!validateFlag(flag))
    return 0;

  strcpy(node->productId, productId);

  strcpy(node->category, category);

  strcpy(node->name, name);

  node->quantity = quantity;

  strcpy(node->date, date);

  node->flag = flag;

  return 1;
}

void printRecordsByProductId(const char *productId) {
  Node *p = head;

  while (p) {
    if (strcmp(p->productId, productId) == 0) {
      printf("%lld %-10s %-10s %-10s %-6d %-12s %s\n", p->id, p->productId,
             p->category, p->name, p->quantity, p->date,
             p->flag ? "卖出" : "进货");
    }

    p = p->next;
  }
}

void printRecordsByName(const char *name) {
  Node *p = head;

  while (p) {
    if (strcmp(p->name, name) == 0) {
      printf("%lld %-10s %-10s %-10s %-6d %-12s %s\n", p->id, p->productId,
             p->category, p->name, p->quantity, p->date,
             p->flag ? "卖出" : "进货");
    }

    p = p->next;
  }
}
