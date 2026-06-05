#include "inventory.h"

int validateFlag(int flag) { return flag == 0 || flag == 1; }

int validateQuantity(int quantity) { return quantity > 0; }

int validateDate(const char *date) {
  int y, m, d;

  if (sscanf(date, "%d-%d-%d", &y, &m, &d) != 3)
    return 0;

  if (m < 1 || m > 12)
    return 0;

  if (d < 1 || d > 31)
    return 0;

  return 1;
}

int getTotalIn(const char *productId) {
  int total = 0;

  Node *p = head;

  while (p) {
    if (strcmp(p->productId, productId) == 0 && p->flag == 0) {
      total += p->quantity;
    }

    p = p->next;
  }

  return total;
}

int getTotalOut(const char *productId) {
  int total = 0;

  Node *p = head;

  while (p) {
    if (strcmp(p->productId, productId) == 0 && p->flag == 1) {
      total += p->quantity;
    }

    p = p->next;
  }

  return total;
}

int getStockByProductId(const char *productId) {
  return getTotalIn(productId) - getTotalOut(productId);
}

int getStockByName(const char *name) {
  Node *p = head;

  while (p) {
    if (strcmp(p->name, name) == 0) {
      return getStockByProductId(p->productId);
    }

    p = p->next;
  }

  return 0;
}