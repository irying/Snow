### 1.基础，引入editline库最好

我们使用 `fputs` 打印提示信息。这个函数和前面介绍过的 `puts` 函数区别是 `fputs` 不会在末尾自动加换行符.

```c
#include <stdio.h>

/* Declare a buffer for user input of size 2048 */
static char input[2048];

int main(int argc, char** argv) {

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  /* In a never ending loop */
  while (1) {

    /* Output our prompt */
    fputs("lispy> ", stdout);

    /* Read a line of user input of maximum size 2048 */
    fgets(input, 2048, stdin);

    /* Echo input back to user */
    printf("No you're a %s", input);
  }

  return 0;
}
```
editline库

```C
 while (1) {

    /* Now in either case readline will be correctly defined */
    char* input = readline("lispy> ");
    add_history(input);

    printf("No you're a %s\n", input);
    free(input);

  }
```

### 2.波兰表达式，核心就是递归

| 普通表达式                | 波兰表达式                |
| -------------------- | -------------------- |
| `1 + 2 + 6`          | `+ 1 2 6`            |
| `6 + (2 * 9)`        | `+ 6 (* 2 9)`        |
| `(10 * 2) / (4 + 2)` | `/ (* 10 2) (+ 4 2)` |

现在，我们需要编写这种标记语言的语法规则。我们可以先用白话文来尝试描述它，而后再将其公式化。

我们观察到，波兰表达式总是以操作符开头，后面跟着操作数或其他的包裹在圆括号中的表达式。也就是说，“程序(`Program`)是由一个操作符(`Operator`)加上一个或多个表达式(`Expression`)组成的”，而 “表达式(`Expression`)可以是一个数字，或者是包裹在圆括号中的一个操作符(`Operator`)加上一个或多个表达式(`Expression`)”。

下面是一个更加规整的描述：

| 名称              | 定义                                       |
| --------------- | ---------------------------------------- |
| 程序(`Program`)   | `开始输入` --> `操作符` --> `一个或多个表达式(Expression)` --> `结束输入` |
| 表达式(Expression) | `数字、左括号 (、操作符` --> `一个或多个表达式` --> `右括号 )` |
| 操作符(Operator)   | `'+'、'-'、'*' 、 '/'`                      |
| 数字(`Number`)    | `可选的负号 -` --> `一个或多个 0 到 9 之间的字符`        |

```c
lispy> + 5 (* 2 2)
>
  regex
  operator|char:1:1 '+'
  expr|number|regex:1:3 '5'
  expr|>
    char:1:5 '('
    operator|char:1:6 '*'
    expr|number|regex:1:8 '2'
    expr|number|regex:1:10 '2'
    char:1:11 ')'
  regex
```

以上是怎么实现的

需要更改之前的 `while` 循环，使它不再只是简单的将用户输入的内容打印回去，而是传进我们的解析器进行解析。我们可以把之前的 `printf` 语句替换成下面的代码：

```c
/* Attempt to Parse the user Input */
mpc_result_t r;
if (mpc_parse("<stdin>", input, Lispy, &r)) {
  /* On Success Print the AST */
  mpc_ast_print(r.output);
  mpc_ast_delete(r.output);
} else {
  /* Otherwise Print the Error */
  mpc_err_print(r.error);
  mpc_err_delete(r.error);
}
```

我们调用了 `mpc_parse` 函数，并将 `Lispy` 解析器和用户输入 `input` 作为参数。它将解析的结果保存到 `&r` 中，如果解析成功，返回值为 `1`，失败为 `0`。对 `r`，我们使用了取地址符 `&`，关于这个符号我们会在后面的章节中讨论。

- 解析成功时会产生一个内部结构，并保存到 `r`的 `output` 字段中。我们可以使用 `mpc_ast_print` 将这个结构打印出来，使用 `mpc_ast_delete` 将其删除。
- 解析失败时则会将错误信息保存在 `r` 的 `error`字段中。我们可以使用 `mpc_err_print` 将这个结构打印出来，使用 `mpc_err_delete` 将其删除。



### 3.计算

- 首先我们注意到，有 `number` 标签的节点一定是一个数字，并且没有孩子节点。我们可以直接将其转换为一个数字。这将是递归函数中的基本情况。
- 如果一个节点有 `expr` 标签，但没有 `number` 标签，我们需要看他的第二个孩子节点是什么操作符(第一个孩子节点永远是 `(` 字符)。然后我们需要使用这个操作符来对后面的孩子节点进行求值。当然，也不包括最后的 `)` 节点。这就是所谓的递归的情况啦。

```C
long eval(mpc_ast_t* t) {

  /* If tagged as number return it directly. */ 
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x` */
  long x = eval(t->children[2]);

  /* Iterate the remaining children and combining. */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;  
}
```

#### 操作符

```c
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}
```

#### 结构体

```C
typedef struct mpc_ast_t {
  char* tag;
  char* contents;
  mpc_state_t state;
  int children_num;
  struct mpc_ast_t** children;
} mpc_ast_t;
```