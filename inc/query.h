#ifndef QUERY_H
#define QUERY_H

#include "data.h"

/*
 * 按商品编号查询并显示全部记录
 */
void queryByProductId(const char *productId);

/*
 * 按商品名称查询并显示全部记录
 */
void queryByName(const char *name);

/*
 * 查询库存
 */
void queryStockByProductId(const char *productId);

void queryStockByName(const char *name);

/*
 * 模糊查询
 */
void fuzzyQuery(const char *keyword);

#endif