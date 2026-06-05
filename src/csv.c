#include "csv.h"

/*
 * 保存链表
 */
int saveCSV(const char *filename) {
  FILE *fp = fopen(filename, "w");

  if (fp == NULL)
    return 0;

  fprintf(fp, "id,productId,category,name,quantity,date,flag\n");

  Node *p = head;

  while (p) {
    fprintf(fp, "%lld,%s,%s,%s,%d,%s,%d\n", p->id, p->productId, p->category,
            p->name, p->quantity, p->date, p->flag);

    p = p->next;
  }

  fclose(fp);

  return 1;
}

/*
 * 加载CSV
 */
int loadCSV(const char *filename) {
  FILE *fp = fopen(filename, "r");

  if (fp == NULL)
    return 0;

  char line[512];

  /*
   * 跳过表头
   */
  fgets(line, sizeof(line), fp);

  while (fgets(line, sizeof(line), fp)) {
    long long id;

    char productId[32];
    char category[32];
    char name[64];

    int quantity;

    char date[16];

    int flag;

    line[strcspn(line, "\r\n")] = '\0';

    int count = sscanf(line, "%lld,%31[^,],%31[^,],%63[^,],%d,%15[^,],%d", &id,
                       productId, category, name, &quantity, date, &flag);

    if (count != 7)
      continue;

    Node *node =
        createNodeWithId(id, productId, category, name, quantity, date, flag);

    if (node)
      insertNode(node);
  }

  fclose(fp);

  return 1;
}
