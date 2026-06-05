#include "stat.h"
#include "inventory.h"

ProductStat buildStat(const char *productId) {
  ProductStat stat;

  memset(&stat, 0, sizeof(ProductStat));

  Node *p = head;

  while (p) {
    if (strcmp(p->productId, productId) == 0) {
      strcpy(stat.productId, p->productId);

      strcpy(stat.category, p->category);

      strcpy(stat.name, p->name);

      if (p->flag == 0) {
        stat.inQuantity += p->quantity;
      } else {
        stat.outQuantity += p->quantity;
      }
    }

    p = p->next;
  }

  stat.remainQuantity = stat.inQuantity - stat.outQuantity;

  return stat;
}

void printStatistics() {
  Node *p;

  char printed[100][32];

  int count = 0;

  printf("\n%-10s %-10s %-10s %-10s %-10s %-10s\n", "类别", "编号", "商品",
         "进货", "卖出", "库存");

  p = head;

  while (p) {
    int exist = 0;

    for (int i = 0; i < count; i++) {
      if (strcmp(printed[i], p->productId) == 0) {
        exist = 1;
        break;
      }
    }

    if (!exist) {
      strcpy(printed[count++], p->productId);

      ProductStat stat = buildStat(p->productId);

      printf("%-10s %-10s %-10s %-10d %-10d %-10d\n", stat.category,
             stat.productId, stat.name, stat.inQuantity, stat.outQuantity,
             stat.remainQuantity);
    }

    p = p->next;
  }
}
