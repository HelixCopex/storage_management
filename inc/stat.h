#ifndef STAT_H
#define STAT_H

#include "data.h"

typedef struct {
  char category[32];

  char productId[32];

  char name[64];

  int inQuantity;

  int outQuantity;

  int remainQuantity;

} ProductStat;

/*

* 统计指定商品
  */
ProductStat buildStat(const char *productId);

/*

* 输出所有商品统计
  */
void printStatistics(void);

#endif
