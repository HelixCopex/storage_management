#include <stdio.h>
#include <string.h>

#include "ui_data.h"

static void appendText(char *out, int maxlen, const char *text) {
  int len = strlen(out);

  if (len >= maxlen - 1) {
    return;
  }

  snprintf(out + len, maxlen - len, "%s", text);
}
void buildRecordTable(char *out, int maxlen) {
  Node *p = head;

  out[0] = '\0';

//  appendText(
//      out, maxlen,
//      "ID             编号     类别     名称       数量   日期         类型\n");
//
//  appendText(
//      out, maxlen,
//      "-----------------------------------------------------------------\n");

  while (p) {
    char line[256];

    snprintf(line, sizeof(line), "%-14lld %-8s %-8s %-10s %-6d %-12s %s\n",
             p->id, p->productId, p->category, p->name, p->quantity, p->date,
             p->flag ? "卖出" : "进货");

    appendText(out, maxlen, line);

    p = p->next;
  }
}

void buildRecordHeader(char *out, int maxlen)
{
    out[0] = '\0';

    appendText(
        out,
        maxlen,
        "ID             编号     类别     名称       数量   日期         类型\n");

    appendText(
        out,
        maxlen,
        "-----------------------------------------------------------------\n");
}

void buildQueryResultByProductId(const char *productId, char *out, int maxlen) {
  Node *p = head;

  int found = 0;

  out[0] = '\0';

  appendText(
      out, maxlen,
      "ID             编号     类别     名称       数量   日期         类型\n");

  appendText(
      out, maxlen,
      "-----------------------------------------------------------------\n");

  while (p) {
    if (strcmp(p->productId, productId) == 0) {
      char line[256];

      found = 1;

      snprintf(line, sizeof(line), "%-14lld %-8s %-8s %-10s %-6d %-12s %s\n",
               p->id, p->productId, p->category, p->name, p->quantity, p->date,
               p->flag ? "卖出" : "进货");

      appendText(out, maxlen, line);
    }

    p = p->next;
  }

  if (!found) {
    appendText(out, maxlen, "\n未找到匹配记录\n");
  }
}

void buildQueryResultByName(const char *name, char *out, int maxlen) {
  Node *p = head;

  int found = 0;

  out[0] = '\0';

  while (p) {
    if (strcmp(p->name, name) == 0) {
      char line[256];

      found = 1;

      snprintf(line, sizeof(line), "%lld %s %s %s %d %s %s\n", p->id,
               p->productId, p->category, p->name, p->quantity, p->date,
               p->flag ? "卖出" : "进货");

      appendText(out, maxlen, line);
    }

    p = p->next;
  }

  if (!found) {
    appendText(out, maxlen, "未找到匹配记录\n");
  }
}

void buildFuzzyQueryResult(const char *keyword, char *out, int maxlen) {
  Node *p = head;

  int found = 0;

  out[0] = '\0';

  while (p) {
    if (strstr(p->productId, keyword) || strstr(p->name, keyword)) {
      char line[256];

      found = 1;

      snprintf(line, sizeof(line), "%lld %s %s %s %d %s %s\n", p->id,
               p->productId, p->category, p->name, p->quantity, p->date,
               p->flag ? "卖出" : "进货");

      appendText(out, maxlen, line);
    }

    p = p->next;
  }

  if (!found) {
    appendText(out, maxlen, "未找到匹配记录\n");
  }
}

ProductStat buildStat(const char *productId);

void buildStatisticsTable(char *out, int maxlen) {
  Node *p = head;

  char printed[256][32];

  int count = 0;

  out[0] = '\0';

  appendText(out, maxlen,
             "类别     编号     商品     进货     卖出     库存\n");

  appendText(out, maxlen,
             "-------------------------------------------------\n");

  while (p) {
    int exist = 0;

    for (int i = 0; i < count; i++) {
      if (strcmp(printed[i], p->productId) == 0) {
        exist = 1;
        break;
      }
    }

    if (!exist) {
      ProductStat s = buildStat(p->productId);

      char line[256];

      snprintf(line, sizeof(line), "%-8s %-8s %-8s %-8d %-8d %-8d\n",
               s.category, s.productId, s.name, s.inQuantity, s.outQuantity,
               s.remainQuantity);

      appendText(out, maxlen, line);

      strcpy(printed[count++], p->productId);
    }

    p = p->next;
  }
}

int getRecordCount() {
  int cnt = 0;

  Node *p = head;

  while (p) {
    cnt++;
    p = p->next;
  }

  return cnt;
}