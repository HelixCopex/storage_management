#ifndef MODIFY_H
#define MODIFY_H

#include "data.h"
#include "inventory.h"

/*
 * 根据ID修改
 */
int modifyRecordById(long long id, const char *productId, const char *category,
                     const char *name, int quantity, const char *date,
                     int flag);

/*
 * 打印指定商品的全部记录
 */
void printRecordsByProductId(const char *productId);

/*
 * 打印指定名称的全部记录
 */
void printRecordsByName(const char *name);

#endif
