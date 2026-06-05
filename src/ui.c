#include <stdio.h>
#include <string.h>

#include "ui.h"

#include "csv.h"
#include "ui_data.h"

static UIState state;

/* 计算 UTF-8 字符串的显示宽度（ASCII=1, 中文/全角=2）*/
static int strVisualWidth(const char *s) {
  int w = 0;
  while (*s) {
    unsigned char c = *s;
    if (c < 0x80)      { w++;  s++; }           // ASCII
    else if (c < 0xE0) { w++;  s += 2; }         // 2-byte (Latin 拡張, 按1列)
    else if (c < 0xF0) { w += 2; s += 3; }       // 3-byte (CJK, 2列)
    else               { w += 2; s += 4; }       // 4-byte (emoji, 2列)
  }
  return w;
}

/* 将 src 以显示宽度 visualWidth 写入 dst（截断或补空格，确保列对齐）*/
static void fmtVisual(char *dst, const char *src, int visualWidth) {
  int w = 0;
  const char *s = src;
  char *d = dst;

  while (*s && w < visualWidth) {
    unsigned char c = *s;
    int cw, cb;
    if (c < 0x80)      { cw = 1; cb = 1; }
    else if (c < 0xE0) { cw = 1; cb = 2; }
    else if (c < 0xF0) { cw = 2; cb = 3; }
    else               { cw = 2; cb = 4; }

    if (w + cw > visualWidth) break;

    for (int i = 0; i < cb && *s; i++) *d++ = *s++;
    w += cw;
  }
  while (w < visualWidth) { *d++ = ' '; w++; }
  *d = '\0';
}

static void drawMain(ui_box_t *b, char *out) {
  sprintf(out, "超市管理系统\n\n"
               "[1] 浏览记录\n"
               "[2] 查询商品\n"
               "[3] 分类统计\n"
               "[4] 新增记录\n\n"
               "[Q] 退出\n");
}

static void drawRecordHeader(ui_box_t *b, char *out) {
  char id[32], pid[32], cat[32], name[32], qty[32], date[32];

  fmtVisual(id,   "ID",   14);
  fmtVisual(pid,  "编号", 8);
  fmtVisual(cat,  "类别", 8);
  fmtVisual(name, "名称", 10);
  fmtVisual(qty,  "数量", 6);
  fmtVisual(date, "日期", 12);

  int len = sprintf(out, "%s %s %s %s %s %s %s\n",
                    id, pid, cat, name, qty, date, "类型");

  /* 分隔线与标题行等长 */
  memset(out + len, '-', len - 1);
  out[len + len - 1] = '\n';
  out[len + len] = '\0';
}

static void drawRecord(ui_box_t *b, char *out) {
  Node *p = head;

  out[0] = '\0';

  while (p) {
    char line[256];
    char cat[32], name[32];

    fmtVisual(cat, p->category, 8);
    fmtVisual(name, p->name, 10);

    snprintf(
        line,
        sizeof(line),
        "%-14lld %-8s %s %s %-6d %-12s %s\n",
        p->id,
        p->productId,
        cat,
        name,
        p->quantity,
        p->date,
        p->flag ? "卖出" : "进货");

    strcat(out, line);

    p = p->next;
  }
}

static void drawRecordHint(ui_box_t *b, char *out) {
  sprintf(out,
          "---------------------------------------------------------------------------"
          "-\n"
          " [滚轮]滚动  [j/k]上下  [q]退出  [1]浏览  [2]查询  [3]统计  [4]新增");
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
    /* 数据不足一页，显示空轨道（无滑块）*/
    char *p = out;
    for (int i = 0; i < height; i++) {
      p += sprintf(p, "│\n");
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

  /* 固定操作提示（底部两行：分隔线 + 快捷键，不随滚动）*/
  ui_add(1, u->ws.ws_row - 2, 76, 2, SCREEN_RECORD, NULL, 0, drawRecordHint,
         NULL, NULL, NULL, NULL, 0, u);

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