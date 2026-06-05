#include <stdio.h>
#include <string.h>

#include "ui.h"

#include "csv.h"
#include "ui_data.h"

static UIState state;

static void drawMain(ui_box_t *b, char *out) {
  sprintf(out, "超市管理系统\n\n"
               "[1] 浏览记录\n"
               "[2] 查询商品\n"
               "[3] 分类统计\n"
               "[4] 新增记录\n\n"
               "[ESC] 退出\n");
}

static void drawRecordHeader(ui_box_t *b, char *out) {
  sprintf(
      out,
      "ID             编号     类别     名称       数量   日期         类型\n"
      "-----------------------------------------------------------------");
}

static void drawRecord(ui_box_t *b, char *out) {
  Node *p = head;

  out[0] = '\0';

  while (p) {
    char line[256];

    snprintf(
        line,
        sizeof(line),
        "%-14lld %-8s %-8s %-10s %-6d %-12s %s\n",
        p->id,
        p->productId,
        p->category,
        p->name,
        p->quantity,
        p->date,
        p->flag ? "卖出" : "进货");

    strcat(out, line);

    p = p->next;
  }
}

static void drawQuery(ui_box_t *b, char *out) {
  sprintf(out,
          "商品编号:\n\n"
          "%s\n\n"
          "Enter 查询\n\n"
          "%s",
          state.queryInput, state.queryResult);
}

static void drawStat(ui_box_t *b, char *out) {
  buildStatisticsTable(state.tableBuf, sizeof(state.tableBuf));

  strcpy(out, state.tableBuf);
}

static ui_t *g_ui;

static void drawScrollInfo(ui_box_t *b, char *out) {
  int total = getRecordCount();

  int visible = g_ui->ws.ws_row - 4;

  int current = -g_ui->scroll;

  if (current < 0)
    current = 0;

  int maxScroll = total - visible;

  if (maxScroll < 1)
    maxScroll = 1;

  int percent =
      current * 100 /
      maxScroll;

  sprintf(
      out,
      "[%d/%d] %d%%",
      current,
      total,
      percent);
}

static void drawScrollBar(ui_box_t *b, char *out) {
  int total = getRecordCount();
  int height = b->h;

  /* 实际可见高度 = min(box高度, 终端剩余行数) */
  int visible = height;
  int termRemaining = g_ui->ws.ws_row - b->y;
  if (visible > termRemaining)
    visible = termRemaining;
  if (visible < 1)
    visible = 1;

  if (total <= visible) {
    /* 数据不足一页，不显示滚动条滑块 */
    char *p = out;
    for (int i = 0; i < height; i++) {
      p += sprintf(p, " \n");
    }
    return;
  }

  int maxScroll = total - visible;
  if (maxScroll < 1)
    maxScroll = 1;

  int pos = (-g_ui->scroll) * (visible - 1) / maxScroll;
  if (pos < 0)
    pos = 0;
  if (pos >= visible)
    pos = visible - 1;

  char *p = out;
  for (int i = 0; i < height; i++) {
    if (i < visible) {
      p += sprintf(p, "%s\n", i == pos ? "█" : "│");
    } else {
      p += sprintf(p, " \n");
    }
  }
}

static void quitApp() {
  saveCSV("data.csv");

  ui_free(g_ui);

  exit(0);
}

static void scrollDown() {
  if (g_ui->screen != SCREEN_RECORD)
    return;

  g_ui->scroll -= 2;

  if (g_ui->scroll < -10000)
    g_ui->scroll = -10000;

  ui_draw(g_ui);
}

static void scrollUp() {
  if (g_ui->screen != SCREEN_RECORD)
    return;

  g_ui->scroll += 2;

  if (g_ui->scroll > 0)
    g_ui->scroll = 0;

  ui_draw(g_ui);
}

static void goRecord() {
  ui_screen(SCREEN_RECORD, g_ui);

  ui_redraw(g_ui);
}

static void goQuery() {
  ui_screen(SCREEN_QUERY, g_ui);

  ui_redraw(g_ui);
}

static void goStat() {
  ui_screen(SCREEN_STAT, g_ui);

  ui_redraw(g_ui);
}

void uiInit(ui_t *u) {
  g_ui = u;

  memset(&state, 0, sizeof(state));

  ui_add(2, 2, 80, 20, SCREEN_MAIN, NULL, 0, drawMain, NULL, NULL, NULL, NULL, 0,
         u);

  /* 记录浏览页面：数据区先添加（先绘制，表头后绘制会覆盖顶部两行）*/
  ui_add(1, 3, 76, 35, SCREEN_RECORD, NULL, 0, drawRecord, NULL, NULL, NULL,
         NULL, 1, u);

  /* 固定表头（后绘制，覆盖数据区顶部）*/
  ui_add(1, 1, 76, 2, SCREEN_RECORD, NULL, 0, drawRecordHeader, NULL, NULL,
         NULL, NULL, 0, u);

  /* 滚动条 */
  ui_add(78, 3, 2, 35, SCREEN_RECORD, NULL, 0, drawScrollBar, NULL, NULL, NULL,
         NULL, 0, u);

  ui_add(1, 1, 80, 40, SCREEN_QUERY, NULL, 0, drawQuery, NULL, NULL, NULL, NULL,
         0, u);

  ui_add(1, 1, 80, 40, SCREEN_STAT, NULL, 0, drawStat, NULL, NULL, NULL, NULL, 0,
         u);

  ui_key("1", goRecord, u);
  ui_key("2", goQuery, u);
  ui_key("3", goStat, u);

  ui_key("j", scrollDown, u);
  ui_key("k", scrollUp, u);

  // ui_key("\x1b", quitApp, u);
  ui_key("q", quitApp, u);
}