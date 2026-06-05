#ifndef CSV_H
#define CSV_H

#include "data.h"

/*
 * 保存链表到CSV文件
 * 成功返回1
 * 失败返回0
 */
int saveCSV(const char *filename);

/*
 * 从CSV文件加载数据
 * 成功返回1
 * 失败返回0
 */
int loadCSV(const char *filename);

#endif
