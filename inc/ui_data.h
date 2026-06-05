#ifndef UI_DATA_H
#define UI_DATA_H

#include "data.h"
#include "inventory.h"
#include "stat.h"

/*
 * 构建全部记录表格
 */
void buildRecordTable(char *out, int maxlen);

/*
 * 按商品编号构建查询结果
 */
void buildQueryResultByProductId(const char *productId, char *out, int maxlen);

/*
 * 按商品名称构建查询结果
 */
void buildQueryResultByName(const char *name, char *out, int maxlen);

/*
 * 模糊查询结果
 */
void buildFuzzyQueryResult(const char *keyword, char *out, int maxlen);

/*
 * 构建统计表
 */
void buildStatisticsTable(char *out, int maxlen);

void buildRecordHeader(
    char *out,
    int maxlen
);

int getRecordCount(void);

#endif