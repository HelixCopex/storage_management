#ifndef UI_H
#define UI_H

#include "tuibox.h"

void uiInit(ui_t *u);

#endif

enum
{
    SCREEN_MAIN = 0,
    SCREEN_RECORD,
    SCREEN_QUERY,
    SCREEN_STAT,
    SCREEN_ADD
};

typedef struct
{
    char tableBuf[32768];

    char queryInput[64];
    char queryResult[8192];
    int queryLen;

    /* 查询结果选中与确认 */
    int  querySelRow;       /* 选中的行号，-1=无 */
    long long querySelId;   /* 选中记录的 ID */
    int  confirmMode;       /* 0=无, 1=删除确认, 2=编辑确认 */

    /* 新增记录表单 */
    char addProductId[32];
    char addCategory[32];
    char addName[64];
    char addQuantity[16];
    char addDate[16];
    int  addFlag;       /* 0=进货, 1=卖出 */
    int  addField;      /* 当前编辑字段 0..4 */
    char addMsg[256];   /* 提交结果提示 */
} UIState;