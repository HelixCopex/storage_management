这个 `tuibox.h` 和前面的 `vt100utils.h` 其实是天然配套的。

如果说：

* `vt100utils.h` = **终端富文本渲染器**
* `tuibox.h` = **终端GUI框架（TUI Framework）**

那么二者组合起来就能做出类似：

```text
┌──────────────────────┐
│      文件管理器      │
├──────────────────────┤
│ file1.txt            │
│ file2.txt            │
│ file3.txt            │
├──────────────────────┤
│ 状态：已连接         │
└──────────────────────┘
```

这种带鼠标交互的终端程序。

---

# 这个库到底是什么？

作者自己写的：

```c
/*
 * tuibox.h: simple tui library
 */
```

即：

> 一个基于 ANSI Terminal 的极简 TUI（Text User Interface）库。

类似于：

* ncurses
* notcurses
* termbox
* ftxui

但实现规模只有几百行。

---

# 一、核心思想

整个库围绕一个概念：

```c
ui_box_t
```

展开。

作者把所有界面元素都看成：

```text
一个矩形(Box)
```

例如：

```text
按钮
文本
菜单
状态栏
输入框
```

全部统一为：

```c
ui_box_t
```

---

# 二、UI对象 ui_t

整个界面：

```c
typedef struct ui_t
{
    ...
} ui_t;
```

相当于：

```text
整个窗口管理器
```

里面保存：

```c
vec_box_t b;
```

所有控件。

---

```c
vec_evt_t e;
```

所有键盘事件。

---

```c
ui_box_t *click;
```

当前鼠标按下对象。

---

```c
int screen;
```

当前页面。

类似：

```text
screen 0 -> 主菜单
screen 1 -> 设置
screen 2 -> 关于
```

---

```c
int scroll;
```

滚动偏移。

---

# 三、Box组件

## ui_box_t

```c
typedef struct ui_box_t
{
  int x,y;
  int w,h;

  func draw;
  func onclick;
  func onhover;

} ui_box_t;
```

实际上就是：

```text
位置
大小
绘制函数
事件函数
```

---

例如：

```c
按钮
```

可以表示：

```c
x=10
y=5

w=20
h=1

draw=draw_button
onclick=button_clicked
```

---

# 四、绘制机制

## draw回调

每个组件必须实现：

```c
draw(box,out)
```

例如：

```c
void draw_button(
    ui_box_t *b,
    char *out
)
{
    sprintf(out,"[ OK ]");
}
```

---

库会调用：

```c
draw(...)
```

获得字符串：

```text
[ OK ]
```

然后输出到终端。

---

# 五、缓存机制

这是整个库比较聪明的地方。

每个Box都有：

```c
char *cache;
```

---

第一次：

```c
draw(...)
```

得到：

```text
CPU: 10%
```

保存到：

```c
cache
```

---

之后刷新时：

如果数据没变：

```c
watch == last
```

就直接使用：

```c
cache
```

而不重新调用：

```c
draw()
```

---

这相当于：

```text
React Virtual DOM
```

的极简版本。

---

# 六、Watch机制

```c
char *watch;
char last;
```

用于自动刷新。

例如：

```c
char status;
```

绑定：

```c
watch=&status
```

---

当：

```c
status='A'
```

变成：

```c
status='B'
```

时：

```c
draw()
```

自动重新执行。

---

# 七、鼠标支持

这部分其实很厉害。

初始化时：

```c
printf(
"\x1b[?1003h"
"\x1b[?1015h"
"\x1b[?1006h"
);
```

开启：

```text
SGR Mouse Reporting
```

---

终端会发送：

```text
ESC[<0;20;10M
```

表示：

```text
鼠标按下
x=20
y=10
```

---

库解析：

```c
_ui_update()
```

中的：

```c
COORDINATE_DECODE()
```

---

得到：

```c
x
y
```

---

然后判断：

```c
box_contains()
```

即：

```c
鼠标是否在Box内
```

---

如果在：

```c
onclick()
```

触发。

---

# 八、键盘支持

注册：

```c
ui_key(
    "q",
    quit,
    &ui
);
```

---

内部保存：

```c
ui_evt_t
```

---

收到输入：

```c
q
```

时：

```c
quit()
```

执行。

---

相当于：

```text
事件监听器
```

---

# 九、多页面机制

```c
screen
```

非常类似：

```text
网页路由
```

---

例如：

```c
screen=0
```

主菜单

---

```c
screen=1
```

设置页面

---

切换：

```c
ui_screen(1,&ui);
```

---

所有：

```c
box->screen==1
```

的组件出现。

---

其它组件隐藏。

---

# 十、绘制过程

核心：

```c
ui_draw()
```

---

执行：

```text
清屏
 ↓
遍历所有Box
 ↓
draw()
 ↓
定位光标
 ↓
输出内容
```

---

定位使用：

```c
printf(
"\x1b[%i;%iH",
row,
col
);
```

即：

```text
CSI row ; col H
```

---

例如：

```c
\x1b[10;20H
```

表示：

```text
光标移动到
第10行第20列
```

---

# 十一、程序运行模式

典型代码：

```c
ui_t ui;

ui_new(0,&ui);

ui_text(
    10,
    5,
    "Hello",
    0,
    NULL,
    NULL,
    &ui
);

ui_draw(&ui);

ui_loop(&ui){
    ui_update(&ui);
}

ui_free(&ui);
```

展开后实际类似：

```c
while(read(...))
{
    _ui_update(...);
}
```

---

# 十二、与 ncurses 对比

| 项目   | tuibox | ncurses    |
| ---- | ------ | ---------- |
| 代码量  | 约1000行 | 几十万行       |
| 学习成本 | 极低     | 较高         |
| 鼠标支持 | 有      | 有          |
| 颜色支持 | 依赖ANSI | 完整         |
| 窗口系统 | 简易     | 完整         |
| 输入框  | 无      | 有          |
| 菜单   | 无      | 有          |
| 性能   | 一般     | 高          |
| 依赖   | 无      | 需要ncurses库 |

---

# 十三、设计风格

这个库最像什么？

实际上非常像网页开发：

```text
HTML DIV
↓
ui_box_t

CSS Position
↓
x,y,w,h

React Component
↓
draw()

onClick
↓
onclick()

onMouseMove
↓
onhover()
```

因此可以把它理解为：

> 一个“终端版的迷你 React + DOM 框架”，使用 ANSI 转义序列直接控制终端，支持文本组件、页面切换、鼠标点击、鼠标悬停、滚动和键盘事件。相比 ncurses 更轻量，但功能也更基础，适合快速编写文件管理器、监控面板、终端小游戏、串口调试界面等小型 TUI 应用。
