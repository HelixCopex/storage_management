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

} UIState;