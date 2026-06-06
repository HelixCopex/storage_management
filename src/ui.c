#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ui.h"

#include "csv.h"
#include "modify.h"
#include "query.h"
#include "ui_data.h"

static UIState state;
static ui_t *g_ui;

/* 前向声明 */
static void goMain(void);
static void goQuery(void);

/* ---- 青-橙配色转义序列 ---- */
#define C_CYAN   "\x1b[1;36m"
#define C_ORANGE "\x1b[1;33m"
#define C_RESET  "\x1b[0m"
#define C_DIM    "\x1b[2m"

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

/* 将文本居中填入宽度为 w 的缓冲区 */
static int fmtCenter(char *dst, const char *text, int w) {
  int vw = strVisualWidth(text);
  int padL = (w - vw) / 2;
  if (padL < 0) padL = 0;
  int pos = 0;
  for (int i = 0; i < padL; i++) dst[pos++] = ' ';
  pos += sprintf(dst + pos, "%s", text);
  while (pos < w) dst[pos++] = ' ';
  dst[pos] = '\0';
  return pos;
}

static void drawMain(ui_box_t *b, char *out) {
  char line[128];
  int pos = 0;

  pos += fmtCenter(line, C_CYAN "TerMark 仓买管理终端" C_RESET, b->w);
  pos += sprintf(line + pos, "\n\n");
  sprintf(out, "%s", line);

  pos = strlen(out);
  pos += sprintf(out + pos, "     %s[F1]%s 浏览记录\n", C_ORANGE, C_RESET);
  pos += sprintf(out + pos, "     %s[F2]%s 查询商品\n", C_ORANGE, C_RESET);
  pos += sprintf(out + pos, "     %s[F3]%s 分类统计\n", C_ORANGE, C_RESET);
  pos += sprintf(out + pos, "     %s[F4]%s 新增记录\n", C_ORANGE, C_RESET);
  pos += sprintf(out + pos, "     %s[F5]%s 提示\n\n", C_ORANGE, C_RESET);
  pos += sprintf(out + pos, "     %s[Q]%s  退出\n\n", C_ORANGE, C_RESET);

  fmtCenter(line, C_DIM "v1.0.0 @ HelixCopex" C_RESET, b->w);
  pos += sprintf(out + pos, "%s", line);
}

static void drawRecordHeader(ui_box_t *b, char *out) {
  char id[32], pid[32], cat[32], name[32], qty[32], date[32];
  fmtVisual(id,   "ID",   14);
  fmtVisual(pid,  "编号", 8);
  fmtVisual(cat,  "类别", 8);
  fmtVisual(name, "名称", 10);
  fmtVisual(qty,  "数量", 6);
  fmtVisual(date, "日期", 12);

  int len = sprintf(out, C_CYAN "%s %s %s %s %s %s %s\n" C_RESET,
                    id, pid, cat, name, qty, date, "类型");

  /* 分隔线长度匹配表头可见宽度（跳过颜色转义序列）*/
  int sepLen = len - 1;  /* 去掉换行符 */
  if (sepLen > 76) sepLen = 76;
  if (sepLen < 1) sepLen = 1;
  memset(out + len, '-', sepLen);
  out[len + sepLen] = '\n';
  out[len + sepLen + 1] = '\0';
}

static void drawRecord(ui_box_t *b, char *out) {
  Node *p = head;
  int outmax = 60000;  /* 安全上限，远小于 MAXCACHESIZE */
  int outlen = 0;
  out[0] = '\0';

  while (p && outlen < outmax) {
    char line[256];
    char cat[32], name[32];
    fmtVisual(cat, p->category, 8);
    fmtVisual(name, p->name, 10);

    int n = snprintf(line, sizeof(line),
             "%-14lld %-8s %s %s %-6d %-12s %s%s%s\n",
             p->id, p->productId, cat, name, p->quantity, p->date,
             p->flag ? C_ORANGE : C_RESET,
             p->flag ? "卖出" : "进货",
             p->flag ? C_RESET : "");
    if (outlen + n >= outmax) break;
    strcat(out, line);
    outlen += n;
    p = p->next;
  }
}

static void drawRecordHint(ui_box_t *b, char *out) {
  char line[128];
  fmtCenter(line, C_DIM "[滚轮]滚动  [j/k]上下  [q]退出  [F5]主界面" C_RESET, b->w);
  sprintf(out, "---------------------------------------------------------------------------"
          "-\n%s", line);
}

static void drawQueryHeader(ui_box_t *b, char *out) {
  char id[32], pid[32], cat[32], name[32], qty[32], date[32];
  fmtVisual(id,   "ID",   14);
  fmtVisual(pid,  "编号", 8);
  fmtVisual(cat,  "类别", 8);
  fmtVisual(name, "名称", 10);
  fmtVisual(qty,  "数量", 6);
  fmtVisual(date, "日期", 12);

  int len = sprintf(out, C_CYAN "查询: %s" C_RESET "█\n" C_CYAN "%s %s %s %s %s %s %s\n" C_RESET,
                    g_ui->input, id, pid, cat, name, qty, date, "类型");

  int hdrStart = len;
  while (out[hdrStart] != '\n' && out[hdrStart] != '\0') hdrStart++;
  if (out[hdrStart] == '\n') hdrStart++;
  int hdrLen = strlen(out + hdrStart);
  if (hdrLen > 0 && out[hdrStart + hdrLen - 1] == '\n') hdrLen--;

  memset(out + len, '-', hdrLen);
  out[len + hdrLen] = '\n';
  out[len + hdrLen + 1] = '\0';
}

static void drawQueryBody(ui_box_t *b, char *out) {
  if (!state.queryResult[0]) {
    char line[128];
    fmtCenter(line, C_DIM "输入编号或名称，按 Enter 查询" C_RESET, b->w);
    sprintf(out, "\n%s", line);
    return;
  }

  char tmp[8192];
  strcpy(tmp, state.queryResult);
  char *line = strtok(tmp, "\n");
  int row = 0;
  int outmax = 60000;
  int outlen = 0;
  out[0] = '\0';

  while (line && outlen < outmax) {
    int n;
    if (row == state.querySelRow)
      n = sprintf(out + outlen, "\x1b[7m%s\x1b[0m\n", line);
    else
      n = sprintf(out + outlen, "%s\n", line);
    outlen += n;
    row++;
    line = strtok(NULL, "\n");
  }
}

static void drawQueryHint(ui_box_t *b, char *out) {
  const char *confirmLine = "";
  if (state.confirmMode == 1)
    confirmLine = C_ORANGE ">> 确认删除该记录？[y]确认 [n]取消" C_ORANGE;

  char line[128];
  fmtCenter(line, confirmLine[0] ? confirmLine :
            C_DIM "[单击]选择  [d]删除  [e]编辑  [Enter]查询  [Esc]清空  [F5]主界面" C_RESET, b->w);
  sprintf(out,
          "---------------------------------------------------------------------------"
          "-\n%s", line);
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

  char title[128];
  fmtCenter(title, C_CYAN "统计汇总" C_RESET, b->w);
  int len = sprintf(out, "%s\n%s %s %s %s %s %s\n",
                    title, cat, pid, name, in, out_, remain);
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
  char line[128];
  fmtCenter(line, C_DIM "[j/k]滚动  [鼠标滚轮]滚动  [F5]主界面  [q]退出" C_RESET, b->w);
  sprintf(out, "---------------------------------------------------------------------------"
          "-\n%s", line);
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

  char title[128];
  fmtCenter(title, state.editMode ? C_CYAN "编辑记录" C_RESET : C_CYAN "新增记录" C_RESET, b->w);
  pos += sprintf(buf + pos, "%s\n\n", title);

  for (int i = 0; i < 5; i++) {
    pos += sprintf(buf + pos, " %s %s%s%s: %s%s\n",
                   i == state.addField ? C_ORANGE ">>" C_RESET : " ",
                   C_CYAN, labels[i], C_RESET,
                   g_ui->inputMode == SCREEN_ADD && i == state.addField
                       ? g_ui->input
                       : values[i],
                   g_ui->inputMode == SCREEN_ADD && i == state.addField ? "█" : "");
  }

  pos += sprintf(buf + pos, "\n %s %s类型%s: [%s] [%s]\n",
                 state.addField == 5 ? C_ORANGE ">>" C_RESET : " ",
                 C_CYAN, C_RESET,
                 state.addFlag == 0 ? C_ORANGE ">进货" C_RESET : " 进货",
                 state.addFlag == 1 ? C_ORANGE ">卖出" C_RESET : " 卖出");

  if (state.addMsg[0])
    pos += sprintf(buf + pos, "\n %s", state.addMsg);

  strcpy(out, buf);
}

static void drawAddHint(ui_box_t *b, char *out) {
  char line[128], text[128];
  snprintf(text, sizeof(text),
           C_DIM "[Enter]确认  [Tab]下一项  [←→]切换类型  [Esc]" C_RESET
           C_DIM "%s  [F5]主界面" C_RESET,
           state.editMode ? "取消" : "返回");
  fmtCenter(line, text, b->w);
  sprintf(out, "---------------------------------------------------------------------------"
          "-\n%s", line);
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
  /* 编辑模式下 Esc 取消编辑，返回查询页 */
  if (g_ui->screen == SCREEN_ADD && state.editMode) {
    g_ui->keyConsumed = 1;
    state.editMode = 0;
    goQuery();
    return;
  }
  if (g_ui->screen != SCREEN_QUERY) {
    goMain();
    return;
  }
  /* 确认模式下 Esc 取消确认 */
  if (state.confirmMode) {
    g_ui->keyConsumed = 1;
    state.confirmMode = 0;
    ui_draw(g_ui);
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
  state.querySelRow = -1;
  state.confirmMode = 0;
  doQuery();
  ui_draw(g_ui);
}

/* 查询结果点击：选中行 */
static void onQueryClick(ui_box_t *b, int x, int y, int mouse) {
  (void)x; (void)mouse;
  if (g_ui->screen != SCREEN_QUERY) return;
  if (!state.queryResult[0]) return;

  int row = y - b->y;
  if (row < 0) return;

  /* 从 queryResult 中取出第 row 行，提取 ID */
  char tmp[8192];
  strcpy(tmp, state.queryResult);
  char *line = strtok(tmp, "\n");
  for (int i = 0; i < row && line; i++)
    line = strtok(NULL, "\n");
  if (!line) return;

  long long id = atoll(line);
  if (id == 0) return;

  state.querySelRow = row;
  state.querySelId  = id;
  state.confirmMode = 0;
  ui_draw(g_ui);
}

/* d 键：删除确认 */
static void onQueryDelete() {
  if (g_ui->screen != SCREEN_QUERY) return;
  if (state.querySelRow < 0) return;
  if (state.confirmMode == 1) return; /* 已在确认中 */
  g_ui->keyConsumed = 1;
  state.confirmMode = 1;
  ui_draw(g_ui);
}

/* e 键：打开编辑表单 */
static void onQueryEdit() {
  if (g_ui->screen != SCREEN_QUERY) return;
  if (state.querySelRow < 0) return;

  Node *node = findById(state.querySelId);
  if (!node) return;

  g_ui->keyConsumed = 1;

  /* 预填表单 */
  strcpy(state.addProductId, node->productId);
  strcpy(state.addCategory,  node->category);
  strcpy(state.addName,      node->name);
  snprintf(state.addQuantity, sizeof(state.addQuantity), "%d", node->quantity);
  strcpy(state.addDate,      node->date);
  state.addFlag  = node->flag;
  state.addField = 0;
  state.addMsg[0] = '\0';
  state.editMode = 1;
  state.editId   = node->id;
  state.confirmMode = 0;

  /* 切换到新增/编辑页面 */
  g_ui->keyConsumed = 1;
  ui_screen(SCREEN_ADD, g_ui);
  g_ui->inputMode = SCREEN_ADD;
  strcpy(g_ui->input, state.addProductId);
  g_ui->inputLen = strlen(g_ui->input);
  ui_redraw(g_ui);
}

/* y / n 确认与取消 */
static void onQueryConfirmYes() {
  if (g_ui->screen != SCREEN_QUERY) return;
  if (!state.confirmMode) return;
  g_ui->keyConsumed = 1;

  if (state.confirmMode == 1) {
    deleteById(state.querySelId);
  }

  state.confirmMode = 0;
  state.querySelRow = -1;
  g_ui->scroll = 0;
  doQuery();
  ui_draw(g_ui);
}

static void onQueryConfirmNo() {
  if (g_ui->screen != SCREEN_QUERY) return;
  if (!state.confirmMode) return;
  g_ui->keyConsumed = 1;
  state.confirmMode = 0;
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

    if (state.editMode) {
      /* 编辑模式：更新已有记录 */
      modifyRecordById(state.editId, state.addProductId,
                       state.addCategory, state.addName,
                       qty, state.addDate, state.addFlag);
      state.editMode = 0;
      /* 返回查询页并刷新结果 */
      g_ui->inputMode = SCREEN_QUERY;
      g_ui->input[0] = '\0';
      g_ui->inputLen = 0;
      ui_screen(SCREEN_QUERY, g_ui);
      doQuery();
      ui_redraw(g_ui);
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
  state.editMode = 0;
  state.editId = 0;
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
  ui_add(1, 4, 76, qryDataH, SCREEN_QUERY, NULL, 0, drawQueryBody,
         (func)onQueryClick, NULL, NULL, NULL, 1, u);
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
  ui_key("\x1b[15~", goMain, u);   /* F5 */
  ui_key("\x1bOE", goMain, u);     /* F5 (备用) */

  ui_key("j", scrollDown, u);
  ui_key("k", scrollUp, u);

  /* 查询结果操作 */
  ui_key("d", onQueryDelete, u);
  ui_key("e", onQueryEdit, u);
  ui_key("y", onQueryConfirmYes, u);
  ui_key("n", onQueryConfirmNo, u);

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