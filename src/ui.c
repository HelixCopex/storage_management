#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ui.h"

#include "csv.h"
#include "query.h"
#include "ui_data.h"

static UIState state;
static ui_t *g_ui;

/* 前向声明 */
static void goMain(void);

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
               "[F1] 浏览记录\n"
               "[F2] 查询商品\n"
               "[F3] 分类统计\n"
               "[F4] 新增记录\n\n"
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
          " [滚轮]滚动  [j/k]上下  [q]退出  [F1]浏览  [F2]查询  [F3]统计  [F4]新增");
}

static void drawQueryHeader(ui_box_t *b, char *out) {
  /* 第一行：查询输入，第二行：表头，第三行：分隔线 */
  char id[32], pid[32], cat[32], name[32], qty[32], date[32];
  fmtVisual(id,   "ID",   14);
  fmtVisual(pid,  "编号", 8);
  fmtVisual(cat,  "类别", 8);
  fmtVisual(name, "名称", 10);
  fmtVisual(qty,  "数量", 6);
  fmtVisual(date, "日期", 12);

  int len = sprintf(out, "查询: %s█\n%s %s %s %s %s %s %s\n",
                    g_ui->input, id, pid, cat, name, qty, date, "类型");

  /* 分隔线匹配表头行长度 */
  int hdrStart = len; /* 第二行起始在 out 中的偏移 */
  while (out[hdrStart] != '\n' && out[hdrStart] != '\0') hdrStart++;
  if (out[hdrStart] == '\n') hdrStart++;
  int hdrLen = strlen(out + hdrStart);
  if (hdrLen > 0 && out[hdrStart + hdrLen - 1] == '\n') hdrLen--;

  memset(out + len, '-', hdrLen);
  out[len + hdrLen] = '\n';
  out[len + hdrLen + 1] = '\0';
}

static void drawQueryBody(ui_box_t *b, char *out) {
  if (state.queryResult[0]) {
    strcpy(out, state.queryResult);
  } else {
    sprintf(out, "\n  输入编号或名称，按 Enter 查询");
  }
}

static void drawQueryHint(ui_box_t *b, char *out) {
  sprintf(out,
          "---------------------------------------------------------------------------"
          "-\n"
          " [Enter]查询  [Esc]清空  [F1-F4]切换  [j/k]滚动  [q]退出");
}

/* 查询结果行数 */
static int countQueryLines(void) {
  if (!state.queryResult[0]) return 0;
  int n = 0;
  for (const char *p = state.queryResult; *p; p++)
    if (*p == '\n') n++;
  return n;
}

static void drawQueryScrollBar(ui_box_t *b, char *out) {
  int total = countQueryLines();
  int height = b->h;
  int visible = height;
  int termRemaining = g_ui->ws.ws_row - b->y;
  if (visible > termRemaining) visible = termRemaining;
  if (visible < 1) visible = 1;

  if (total <= visible) {
    char *p = out;
    for (int i = 0; i < height; i++)
      p += sprintf(p, "│\n");
    return;
  }
  int maxScroll = total - visible;
  if (maxScroll < 1) maxScroll = 1;
  int pos = (-g_ui->scroll) * (visible - 1) / maxScroll;
  if (pos < 0) pos = 0;
  if (pos >= visible) pos = visible - 1;
  char *p = out;
  for (int i = 0; i < height; i++) {
    if (i < visible)
      p += sprintf(p, "%s\n", i == pos ? "█" : "│");
    else
      p += sprintf(p, " \n");
  }
}

/* ---- 统计页 ---- */
static void drawStatHeader(ui_box_t *b, char *out) {
  char cat[32], pid[32], name[32], in[32], out_[32], remain[32];
  fmtVisual(cat,    "类别", 8);
  fmtVisual(pid,    "编号", 8);
  fmtVisual(name,   "商品", 8);
  fmtVisual(in,     "进货", 8);
  fmtVisual(out_,   "卖出", 8);
  fmtVisual(remain, "库存", 8);

  int len = sprintf(out, "统计汇总                                    \n%s %s %s %s %s %s\n",
                    cat, pid, name, in, out_, remain);
  memset(out + len, '-', 48);
  out[len + 48] = '\n';
  out[len + 49] = '\0';
}

static void drawStatBody(ui_box_t *b, char *out) {
  buildStatisticsTable(state.tableBuf, sizeof(state.tableBuf));
  /* 跳过 buildStatisticsTable 自带的表头两行 */
  char *p = state.tableBuf;
  int lines = 0;
  while (*p && lines < 2) {
    if (*p == '\n') lines++;
    p++;
  }
  strcpy(out, p);
}

static void drawStatHint(ui_box_t *b, char *out) {
  sprintf(out,
          "---------------------------------------------------------------------------"
          "-\n"
          " [j/k]滚动  [F1-F4]切换  [q]退出");
}

/* 统计行数（去重商品数）*/
static int countStatLines(void) {
  Node *p = head;
  int n = 0;
  char seen[256][32];
  int seenCount = 0;
  while (p) {
    int found = 0;
    for (int i = 0; i < seenCount; i++)
      if (strcmp(seen[i], p->productId) == 0) { found = 1; break; }
    if (!found) {
      if (seenCount < 256) strcpy(seen[seenCount++], p->productId);
      n++;
    }
    p = p->next;
  }
  return n;
}

static void drawStatScrollBar(ui_box_t *b, char *out) {
  int total = countStatLines();
  int height = b->h;
  int visible = height;
  int termRemaining = g_ui->ws.ws_row - b->y;
  if (visible > termRemaining) visible = termRemaining;
  if (visible < 1) visible = 1;

  if (total <= visible) {
    char *p = out;
    for (int i = 0; i < height; i++)
      p += sprintf(p, "│\n");
    return;
  }
  int maxScroll = total - visible;
  if (maxScroll < 1) maxScroll = 1;
  int pos = (-g_ui->scroll) * (visible - 1) / maxScroll;
  if (pos < 0) pos = 0;
  if (pos >= visible) pos = visible - 1;
  char *p = out;
  for (int i = 0; i < height; i++) {
    if (i < visible)
      p += sprintf(p, "%s\n", i == pos ? "█" : "│");
    else
      p += sprintf(p, " \n");
  }
}

static void drawAdd(ui_box_t *b, char *out) {
  const char *labels[] = {"商品编号", "类别", "名称", "数量", "日期"};
  const char *values[] = {state.addProductId, state.addCategory, state.addName,
                          state.addQuantity, state.addDate};
  char buf[1024];
  int pos = 0;

  pos += sprintf(buf + pos, "新增记录\n\n");

  for (int i = 0; i < 5; i++) {
    pos += sprintf(buf + pos, " %s %s: %s%s\n",
                   i == state.addField ? ">" : " ",
                   labels[i],
                   g_ui->inputMode == SCREEN_ADD && i == state.addField
                       ? g_ui->input
                       : values[i],
                   g_ui->inputMode == SCREEN_ADD && i == state.addField ? "█" : "");
  }

  pos += sprintf(buf + pos, "\n %s 类型: [%s] [%s]\n",
                 state.addField == 5 ? ">" : " ",
                 state.addFlag == 0 ? ">进货" : " 进货",
                 state.addFlag == 1 ? ">卖出" : " 卖出");

  if (state.addMsg[0])
    pos += sprintf(buf + pos, "\n %s", state.addMsg);

  strcpy(out, buf);
}

static void drawAddHint(ui_box_t *b, char *out) {
  sprintf(out,
          "---------------------------------------------------------------------------"
          "-\n"
          " [Enter]确认  [Tab]下一项  [左/右]切换类型  [Esc]返回");
}

/* ---- 查询处理 ---- */
static void doQuery() {
  strcpy(state.queryInput, g_ui->input);
  state.queryResult[0] = '\0';

  if (state.queryInput[0] == '\0') {
    strcpy(state.queryResult, "  请输入关键字。");
    return;
  }

  /* 使用 ui_data 的模糊查询构建结果 */
  buildFuzzyQueryResult(state.queryInput, state.queryResult,
                        sizeof(state.queryResult));
}

static void clearQuery() {
  if (g_ui->screen != SCREEN_QUERY) {
    goMain();
    return;
  }
  g_ui->keyConsumed = 1;
  g_ui->input[0] = '\0';
  g_ui->inputLen = 0;
  state.queryInput[0] = '\0';
  state.queryResult[0] = '\0';
  ui_draw(g_ui);
}

/* 查询页 Enter 回调 */
static void onQueryEnter() {
  if (g_ui->screen != SCREEN_QUERY) return;
  g_ui->keyConsumed = 1;
  strcpy(state.queryInput, g_ui->input);
  g_ui->scroll = 0;
  doQuery();
  ui_draw(g_ui);
}

/* ---- 新增记录处理 ---- */
static void addNextField() {
  if (g_ui->screen != SCREEN_ADD) return;
  g_ui->keyConsumed = 1;

  /* 保存当前输入到对应字段 */
  switch (state.addField) {
  case 0: strcpy(state.addProductId, g_ui->input); break;
  case 1: strcpy(state.addCategory, g_ui->input); break;
  case 2: strcpy(state.addName, g_ui->input); break;
  case 3: strcpy(state.addQuantity, g_ui->input); break;
  case 4: strcpy(state.addDate, g_ui->input); break;
  }

  state.addField++;
  if (state.addField > 5) {
    /* 所有字段填完，提交 */
    int qty = atoi(state.addQuantity);
    if (qty <= 0) {
      snprintf(state.addMsg, sizeof(state.addMsg),
               "  数量无效，请重新输入。");
      state.addField = 3;
      strcpy(g_ui->input, "");
      g_ui->inputLen = 0;
      ui_draw(g_ui);
      return;
    }

    Node *node = createNode(state.addProductId, state.addCategory,
                            state.addName, qty, state.addDate, state.addFlag);
    if (node == NULL) {
      snprintf(state.addMsg, sizeof(state.addMsg), "    创建记录失败。");
    } else {
      insertNode(node);
      snprintf(state.addMsg, sizeof(state.addMsg),
               "  ✓ 记录添加成功！(ID: %lld)", node->id);

      /* 保留编号/类别/名称以便连续录入，清空数量和日期 */
      state.addQuantity[0] = '\0';
      state.addDate[0] = '\0';
      state.addField = 3;
    }
    strcpy(g_ui->input, "");
    g_ui->inputLen = 0;
    ui_draw(g_ui);
    return;
  }

  /* 切换到下一字段的当前值 */
  switch (state.addField) {
  case 0: strcpy(g_ui->input, state.addProductId); break;
  case 1: strcpy(g_ui->input, state.addCategory); break;
  case 2: strcpy(g_ui->input, state.addName); break;
  case 3: strcpy(g_ui->input, state.addQuantity); break;
  case 4: strcpy(g_ui->input, state.addDate); break;
  case 5: g_ui->input[0] = '\0'; break;
  }
  g_ui->inputLen = strlen(g_ui->input);
  state.addMsg[0] = '\0';
  ui_draw(g_ui);
}

static void addToggleFlag() {
  if (g_ui->screen != SCREEN_ADD) return;
  g_ui->keyConsumed = 1;
  state.addFlag = !state.addFlag;
  state.addMsg[0] = '\0';
  ui_draw(g_ui);
}

/* 初始化新增表单 */
static void initAddForm() {
  /* 自动填入今天日期 */
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  strftime(state.addDate, sizeof(state.addDate), "%Y-%m-%d", tm);

  state.addProductId[0] = '\0';
  state.addCategory[0] = '\0';
  state.addName[0] = '\0';
  state.addQuantity[0] = '\0';
  state.addFlag = 0;
  state.addField = 0;
  state.addMsg[0] = '\0';

  strcpy(g_ui->input, state.addProductId);
  g_ui->inputLen = 0;
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
  g_ui->keyConsumed = 1;
  saveCSV("data.csv");
  ui_free(g_ui);
  exit(0);
}

static void scrollDown() {
  if (g_ui->inputMode) return;
  if (g_ui->screen != SCREEN_RECORD && g_ui->screen != SCREEN_QUERY &&
      g_ui->screen != SCREEN_STAT)
    return;
  g_ui->keyConsumed = 1;
  g_ui->scroll -= 2;
  if (g_ui->scroll < -10000)
    g_ui->scroll = -10000;
  ui_draw(g_ui);
}

static void scrollUp() {
  if (g_ui->inputMode) return;
  if (g_ui->screen != SCREEN_RECORD && g_ui->screen != SCREEN_QUERY &&
      g_ui->screen != SCREEN_STAT)
    return;
  g_ui->keyConsumed = 1;
  g_ui->scroll += 2;
  if (g_ui->scroll > 0)
    g_ui->scroll = 0;
  ui_draw(g_ui);
}

static void goMain() {
  g_ui->keyConsumed = 1;
  ui_screen(SCREEN_MAIN, g_ui);
  g_ui->inputMode = 0;
  ui_redraw(g_ui);
}

static void goRecord() {
  g_ui->keyConsumed = 1;
  ui_screen(SCREEN_RECORD, g_ui);
  g_ui->inputMode = 0;
  ui_redraw(g_ui);
}

static void goQuery() {
  g_ui->keyConsumed = 1;
  ui_screen(SCREEN_QUERY, g_ui);
  g_ui->inputMode = SCREEN_QUERY;
  g_ui->input[0] = '\0';
  g_ui->inputLen = 0;
  state.queryInput[0] = '\0';
  state.queryResult[0] = '\0';
  ui_redraw(g_ui);
}

static void goStat() {
  g_ui->keyConsumed = 1;
  ui_screen(SCREEN_STAT, g_ui);
  g_ui->inputMode = 0;
  ui_redraw(g_ui);
}

static void goAdd() {
  g_ui->keyConsumed = 1;
  ui_screen(SCREEN_ADD, g_ui);
  g_ui->inputMode = SCREEN_ADD;
  initAddForm();
  ui_redraw(g_ui);
}

void uiInit(ui_t *u) {
  g_ui = u;

  memset(&state, 0, sizeof(state));

  int termRows = u->ws.ws_row;
  int recDataH = termRows - 5;  /* 表头2行, 提示2行, 间隔 */
  int qryDataH = termRows - 6;  /* 查询头3行, 提示2行, 间隔 */
  int statDataH = termRows - 6;
  if (recDataH < 5)  recDataH = 5;
  if (qryDataH < 5)  qryDataH = 5;
  if (statDataH < 5) statDataH = 5;

  ui_add(2, 2, 80, 20, SCREEN_MAIN, NULL, 0, drawMain, NULL, NULL, NULL, NULL, 0,
         u);

  /* ========== 记录浏览页 ========== */
  ui_add(1, 3, 76, recDataH, SCREEN_RECORD, NULL, 0, drawRecord, NULL, NULL,
         NULL, NULL, 1, u);
  ui_add(1, 1, 76, 2, SCREEN_RECORD, NULL, 0, drawRecordHeader, NULL, NULL,
         NULL, NULL, 0, u);
  ui_add(78, 3, 2, recDataH, SCREEN_RECORD, NULL, 0, drawScrollBar, NULL, NULL,
         NULL, NULL, 0, u);
  ui_add(1, termRows - 2, 76, 2, SCREEN_RECORD, NULL, 0, drawRecordHint, NULL,
         NULL, NULL, NULL, 0, u);

  /* ========== 查询页 ========== */
  ui_add(1, 4, 76, qryDataH, SCREEN_QUERY, NULL, 0, drawQueryBody, NULL, NULL,
         NULL, NULL, 1, u);
  ui_add(1, 1, 76, 3, SCREEN_QUERY, NULL, 0, drawQueryHeader, NULL, NULL, NULL,
         NULL, 0, u);
  ui_add(78, 4, 2, qryDataH, SCREEN_QUERY, NULL, 0, drawQueryScrollBar, NULL,
         NULL, NULL, NULL, 0, u);
  ui_add(1, termRows - 2, 76, 2, SCREEN_QUERY, NULL, 0, drawQueryHint, NULL,
         NULL, NULL, NULL, 0, u);

  /* ========== 新增记录页 ========== */
  ui_add(1, 1, 76, termRows - 3, SCREEN_ADD, NULL, 0, drawAdd, NULL, NULL,
         NULL, NULL, 0, u);
  ui_add(1, termRows - 2, 76, 2, SCREEN_ADD, NULL, 0, drawAddHint, NULL, NULL,
         NULL, NULL, 0, u);

  /* ========== 统计页 ========== */
  ui_add(1, 4, 76, statDataH, SCREEN_STAT, NULL, 0, drawStatBody, NULL, NULL,
         NULL, NULL, 1, u);
  ui_add(1, 1, 76, 3, SCREEN_STAT, NULL, 0, drawStatHeader, NULL, NULL, NULL,
         NULL, 0, u);
  ui_add(78, 4, 2, statDataH, SCREEN_STAT, NULL, 0, drawStatScrollBar, NULL,
         NULL, NULL, NULL, 0, u);
  ui_add(1, termRows - 2, 76, 2, SCREEN_STAT, NULL, 0, drawStatHint, NULL,
         NULL, NULL, NULL, 0, u);

  /* F1-F4 切换页面（兼容两种终端转义序列）*/
  ui_key("\x1bOP", goRecord, u);
  ui_key("\x1b[11~", goRecord, u);
  ui_key("\x1bOQ", goQuery, u);
  ui_key("\x1b[12~", goQuery, u);
  ui_key("\x1bOR", goStat, u);
  ui_key("\x1b[13~", goStat, u);
  ui_key("\x1bOS", goAdd, u);
  ui_key("\x1b[14~", goAdd, u);

  ui_key("j", scrollDown, u);
  ui_key("k", scrollUp, u);

  /* 查询/新增通用：Enter 提交，Esc 返回（\r 和 \n 双保险）*/
  ui_key("\r", onQueryEnter, u);
  ui_key("\n", onQueryEnter, u);
  ui_key("\r", addNextField, u);
  ui_key("\n", addNextField, u);
  ui_key("\t", addNextField, u);
  ui_key("\x1b", clearQuery, u);

  /* 新增：左右箭头切换进货/卖出 */
  ui_key("\x1b[C", addToggleFlag, u);
  ui_key("\x1b[D", addToggleFlag, u);

  // ui_key("\x1b", quitApp, u);
  ui_key("q", quitApp, u);
}