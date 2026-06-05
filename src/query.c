#include <stdio.h>
#include <string.h>

#include "inventory.h"
#include "query.h"

static void printHeader() {
  printf("\n%-15s %-10s %-10s %-10s %-8s %-12s %-8s\n", "ID", "编号", "类别",
         "名称", "数量", "日期", "类型");
}

void queryByProductId(const char *productId) {
  Node *p = head;

  int found = 0;

  printHeader();

  while (p) {
    if (strcmp(p->productId, productId) == 0) {
      found = 1;

      printf("%-15lld %-10s %-10s %-10s %-8d %-12s %-8s\n", p->id, p->productId,
             p->category, p->name, p->quantity, p->date,
             p->flag ? "卖出" : "进货");
    }

    p = p->next;
  }

  if (!found) {
    printf("\n未找到商品编号: %s\n", productId);
  }
}

void queryByName(const char *name) {
  Node *p = head;

  int found = 0;

  printHeader();

  while (p) {
    if (strcmp(p->name, name) == 0) {
      found = 1;

      printf("%-15lld %-10s %-10s %-10s %-8d %-12s %-8s\n", p->id, p->productId,
             p->category, p->name, p->quantity, p->date,
             p->flag ? "卖出" : "进货");
    }

    p = p->next;
  }

  if (!found) {
    printf("\n未找到商品: %s\n", name);
  }
}

void queryStockByProductId(const char *productId) {
  printf("\n商品编号: %s\n", productId);

  printf("当前库存: %d\n", getStockByProductId(productId));
}

void queryStockByName(const char *name) {
  printf("\n商品名称: %s\n", name);

  printf("当前库存: %d\n", getStockByName(name));
}

// 模糊查询
void fuzzyQuery(const char *keyword) {
  Node *p = head;

  int found = 0;

  printHeader();

  while (p) {
    if (strstr(p->name, keyword) != NULL ||
        strstr(p->productId, keyword) != NULL) {
      found = 1;

      printf("%-15lld %-10s %-10s %-10s %-8d %-12s %-8s\n", p->id, p->productId,
             p->category, p->name, p->quantity, p->date,
             p->flag ? "卖出" : "进货");
    }

    p = p->next;
  }

  if (!found) {
    printf("\n未找到关键字: %s\n", keyword);
  }
}