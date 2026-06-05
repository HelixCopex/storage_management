#ifndef INVENTORY_H
#define INVENTORY_H

#include "data.h"

/* 数据校验 */
int validateDate(const char *date);

int validateFlag(int flag);

int validateQuantity(int quantity);

/* 库存查询 */
int getStockByProductId(const char *productId);

int getStockByName(const char *name);

/* 统计 */
int getTotalIn(const char *productId);

int getTotalOut(const char *productId);

#endif
