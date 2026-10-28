## 构建方法

在lr-grep文件夹下打开wsl

```bash
gcc ./src/grep.c -o ./tests/lrg
cd tests
```

## 使用说明

通用格式：

```bash
./lrg PATTERN FILE               # 基本匹配
./lrg -n PATTERN FILE            # 单个文件
./lrg -ni PATTERN FILE FILE2     # 选项可以合并，等价于 -n -i
./lrg PATTERN                    # 省略 FILE 时读取标准输入
```

**读取标准输入时在空行按Ctrl+D（或在行结尾按两次Ctrl+D）结束输入，若在行尾连按两次Ctrl+D结束输入，则输出无法换行（经测试grep也是）**
选项含义：
|选项|含义|
|---|---|
|-n|在每行前输出行号（从 1 开始，每个文件独立计数）|
|-i|匹配时忽略 ASCII 字母的大小写|
|-c|只输出匹配行的数量，不输出行内容（与 -n 同时出现时忽略 -n）|

## 运行示例

sample.log:

```
boot ok
disk full at 03:00
ERROR: retry later
boot ok again
error: give up
```

notes.txt:

```
error handling rules
all good
```

1. 单文件读取，输出行号

```bash
$ ./lrg -n error sample.log
5:error: give up
```

2. 双文件读取，显示行号，忽略大小写，选项合并

```bash
$ ./lrg -ni ErRoR sample.log notes.txt
sample.log:3:ERROR: retry later
sample.log:5:error: give up
notes.txt:1:error handling rules
```

3. 读取标准输入，只输出匹配行数量，有-c时忽略-n

```bash
$ ./lrg -cn Abc
Abc
AAbc
AbAbc
abc
asdasd #Enter+(Ctrl+D)
3
```

## 设计说明

### 函数功能

```c
int min(int a,int b);//返回a,b的最小值
void reportError(char *filename,char *err);//向STDERR写入异常
int searchFile(char *filename,char *cnt);//在指定文件filename中寻找字符串cnt，找到了返回0，没找到返回1，出错返回2
int equal(char *p1,char *p2);//判断两个字符是否相等，内置根据是否有-i决定是否判断大小写，相同返回1，不相同返回0
int printline(char *filename,int j);//打印filename的第j行，打印成功返回0，出错返回2
int main(int argc,char *argv[]);//主函数，负责参数识别、调用其他函数、读取和处理来自STDIN的输入
```

### 如何做到行长不限与内存不随文件增长

对于读取文件模式，使用每次阅读1个字节并与目标比对的方案，当正确时累计`i`，错误时回溯`i=border[i-1]`避免出现漏匹配，达到目标字符串的长度时累计`count`并调用`printline`函数，重新按字节读取文件并输出当前行，并将`hasprint`置为1，表示当行已经输出，读取到`\n`时行号j自增，重置`hasprint`标志，每次读取1字节使占用内存与文件大小无关，代价是速度很慢，处理大文件的效果不佳。
对于读取标准输入，无法做到每次取1字节的同时回滚打印当前行，因此使用了分配128字节空间储存当前行，若到达上限则申请翻倍内存的方案，其他细节基本类似。因此对于标准输入，无法做到内存为定值，与输入长度有关。

### 参数解析逻辑

从argv[1]开始按次序读取argv[]，读取到-开头时开始识别-参数，读取到非-开头时关闭读取-参数的模式，读取到PATTERN参数后计算剩余参数数量，若大于等于2则说明需要输出文件名。这个逻辑存在一个问题：在不填-参数的情况下无法查找形如“---”等以-开头的PATTERN，但经过实测，grep也有同样问题，并且语义不明造成的问题过于主观，无法解决。

### 本程序使用的变量及其含义表

| 变量名        | 含义&作用                                                |
| ------------- | -------------------------------------------------------- |
| moden         | -n是否打开                                               |
| modei         | -i是否打开                                               |
| modec         | -c是否打开                                               |
| needFileName  | 是否需要文件名前缀                                       |
| j             | 行号                                                     |
| msg           | 字符串常量（char \*）                                    |
| fd            | 文件描述符                                               |
| readsuccess   | 读文件是否成功                                           |
| head          | 该行是否已有前缀                                         |
| c             | 指向单个字符的指针                                       |
| ch            | 指向单个字符的指针                                       |
| border[i]     | kmp，PATTERN[0]-PATTERN[i]的最长真前缀也是后缀的子串长度 |
| count         | 记录该行是否有吻合的串                                   |
| hasprint      | 该行是否已经打印                                         |
| matchline     | 存在吻合串的行的数量                                     |
| str           | PATTERN参数                                              |
| argnum        | 剩余未读入参数数量                                       |
| inputMode     | 是否应输入-参数                                          |
| inputPattern  | 是否应输入PATTERN                                        |
| inputFileName | 是否应输入文件名                                         |
| returnCode    | 主函数返回值                                             |
| buf           | 存储STDIN输入的一行                                      |
| word          | buf的字符数                                              |
| cap           | buf的大小                                                |

### 其他说明

其实本程序的模块化程度还可以改进，目前有关STDIN的读取和输出都由main中的一个if分支完成，可以分离成单独模块或合并进searchFile和printline。

## 测试样例与错误处理

### 题目中的样例测试

样例与预期结果：

```bash
$ ./lrg boot sample.log
boot ok
boot ok again

$ ./lrg -in error sample.log
3:ERROR: retry later
5:error: give up

$ ./lrg -ci error sample.log notes.txt
sample.log:2
notes.txt:1

$ ./lrg zzz sample.log; echo "exit=$?"
exit=1

$ cat sample.log | ./lrg -n boot
1:boot ok
4:boot ok again

$ ./lrg error sample.log no-such-file; echo "exit=$?"
lrg: no-such-file: No such file or directory      # stderr
error: give up
exit=2

$ ./lrg; echo "exit=$?"
usage: lrg [-nic] PATTERN [FILE...]               # stderr
exit=2
```

实际运行结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot sample.log
boot ok
boot ok again
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -in error sample.log
3:ERROR: retry later
5:error: give up
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -ci error sample.log notes.txt
sample.log:2
notes.txt:1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg zzz sample.log; echo "exit=$?"
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ cat sample.log | ./lrg -n boot
1:boot ok
4:boot ok again
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg error sample.log no-such-file; echo "exit=$?"
sample.log:error: give up
lrg: no-such-file: No such file or directory
exit=2
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg; echo "exit=$?"
usage: lrg [-nic] PATTERN [FILE...]
exit=2
```

这里第6个输出和题目要求输出略有不同，但是实测与grep输出相同，可以认为是合格的。

### ai设计的测试用例与预期结果与实际运行结果

下面 8 个用例都按 WSL/bash 编写，文件一律用 `printf` 创建（只产生 LF，避免 CRLF 干扰）。每个用例给出：建文件的命令、运行命令、期望输出与退出码。

---

#### 用例 1：基本匹配与多文件前缀

```bash
printf 'boot ok\ndisk full at 03:00\nboot again\n' > a.log
printf 'no entry\nboot loader\n' > b.log

./lrg boot a.log; echo "exit=$?"
# exit 0
# boot ok
# boot again

./lrg 'boot again' a.log; echo "exit=$?"        # 模式含空格
# exit 0
# boot again

./lrg boot a.log b.log; echo "exit=$?"          # 多文件才加 "文件名:" 前缀
# exit 0
# a.log:boot ok
# a.log:boot again
# b.log:boot loader
```

#### 用例1结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ gcc ../src/grep.c -o lrg
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'boot ok\ndisk full at 03:00\nboot again\n' > a.log
printf 'no entry\nboot loader\n' > b.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot a.log; echo "exit=$?"
boot ok
boot again
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg 'boot again' a.log; echo "exit=$?"
boot again
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot a.log b.log; echo "exit=$?"
a.log:boot ok
a.log:boot again
b.log:boot loader
exit=0
```

#### 用例 2：`-n` 行号（单文件 / 多文件）

```bash
./lrg -n boot a.log; echo "exit=$?"
# exit 0
# 1:boot ok
# 3:boot again

./lrg -n boot a.log b.log; echo "exit=$?"       # 前缀顺序：文件 → 行号 → 内容
# exit 0
# a.log:1:boot ok
# a.log:3:boot again
# b.log:2:boot loader
```

#### 用例2结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -n boot a.log; echo "exit=$?"
1:boot ok
3:boot again
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -n boot a.log b.log; echo "exit=$?"
a.log:1:boot ok
a.log:3:boot again
b.log:2:boot loader
exit=0
```

#### 用例 3：`-c` 按行计数、同一行只打印一次

```bash
printf 'boot boot boot\nboot\nnope\n' > c.log

./lrg boot c.log; echo "exit=$?"                # 一行命中 3 次也只打印一次
# exit 0
# boot boot boot
# boot

./lrg -c boot c.log; echo "exit=$?"             # 计的是"匹配行数"，不是出现次数
# exit 0
# 2

./lrg -c boot a.log c.log; echo "exit=$?"
# exit 0
# a.log:2
# c.log:2

./lrg -cn boot c.log; echo "exit=$?"            # -c 覆盖 -n，不出行号
# exit 0
# 2
```

#### 用例3结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'boot boot boot\nboot\nnope\n' > c.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot c.log; echo "exit=$?"
boot boot boot
boot
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -c boot c.log; echo "exit=$?"
2
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -c boot a.log c.log; echo "exit=$?"
a.log:2
c.log:2
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -cn boot c.log; echo "exit=$?"
2
exit=0
```

#### 用例 4：`-i` 大小写不敏感，且不误伤非字母

```bash
printf 'HELLO World\nhello world\nHeLLo\nPORT 0X30\nhelp me\n' > d.log

./lrg hello d.log; echo "exit=$?"               # 区分大小写
# exit 0
# hello world

./lrg -i hello d.log; echo "exit=$?"
# exit 0
# HELLO World
# hello world
# HeLLo

./lrg 0x30 d.log; echo "exit=$?"                # 只有 'x' 大小写不同 → 无输出
# exit 1

./lrg -i 0x30 d.log; echo "exit=$?"             # -i 只折叠字母，数字不受影响
# exit 0
# PORT 0X30
```

#用例4结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'HELLO World\nhello world\nHeLLo\nPORT 0X30\nhelp me\n' > d.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg hello d.log; echo "exit=$?"
hello world
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -i hello d.log; echo "exit=$?"
HELLO World
hello world
HeLLo
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg 0x30 d.log; echo "exit=$?"
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -i 0x30 d.log; echo "exit=$?"
PORT 0X30
exit=0
```

#### 用例 5：stdin（管道）+ 末行没有换行

```bash
printf 'boot ok\ndisk full\nboot again' | ./lrg -n boot; echo "exit=$?"
# exit 0
# 1:boot ok
# 3:boot again        ← 最后一行没有 '\n'，内容也必须完整输出

printf 'boot\nx\nboot\n' | ./lrg -c boot; echo "exit=$?"
# exit 0
# 2

printf 'boot ok' | ./lrg boot; echo "exit=$?"
# exit 0
# boot ok             ← 内容不能丢，且要补一个换行
```

#### 用例5结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'boot ok\ndisk full\nboot again' | ./lrg -n boot; echo "exit=$?"
1:boot ok
3:boot again
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'boot\nx\nboot\n' | ./lrg -c boot; echo "exit=$?"
2
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'boot ok' | ./lrg boot; echo "exit=$?"
boot ok
exit=0
```

#### 用例 6：无匹配 / 空文件 / 空 stdin（退出码）

```bash
: > empty.log

./lrg zzz a.log; echo "exit=$?"
# exit 1
# （无输出）

./lrg -c zzz a.log; echo "exit=$?"
# exit 1
# 0

./lrg boot empty.log; echo "exit=$?"
# exit 1
# （无输出）

./lrg -c boot empty.log; echo "exit=$?"
# exit 1
# 0

printf '' | ./lrg -n boot; echo "exit=$?"
# exit 1
# （无输出，不能多冒出一个空行）
```

#### 用例6结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ : > empty.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg zzz a.log; echo "exit=$?"
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -c zzz a.log; echo "exit=$?"
0
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot empty.log; echo "exit=$?"
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg -c boot empty.log; echo "exit=$?"
0
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf '' | ./lrg -n boot; echo "exit=$?"
exit=1
```

#### 用例 7：文件不存在——只报一次错，其余文件照常处理

```bash
./lrg boot missing.log; echo "exit=$?"          # 只给一个文件
# exit 2
# stdout: （无）
# stderr: lrg: missing.log: No such file or directory      ← 只有 1 行

./lrg boot missing.log a.log; echo "exit=$?"    # 坏文件在前，好文件在后
# exit 2
# stdout:
# a.log:boot ok
# a.log:boot again
# stderr: 1 行错误
```

#### 用例7结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot missing.log; echo "exit=$?"
lrg: missing.log: No such file or directory
exit=2
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg boot missing.log a.log; echo "exit=$?"
lrg: missing.log: No such file or directory
a.log:boot ok
a.log:boot again
exit=2
```

#### 用例 8：行隔离与重叠匹配（回溯）

```bash
printf 'a\nb\n' > e.log
./lrg ab e.log; echo "exit=$?"                  # 'a' + 'b' 分在两行，不能拼成 "ab"
# exit 1
# （无输出）

printf 'aab\n' > f.log
./lrg ab f.log; echo "exit=$?"                  # 命中位置在下标 1，需要失配后回溯
# exit 0
# aab

printf 'aaab\n' > g.log
./lrg aab g.log; echo "exit=$?"                 # 'aa' 先匹配成功再失败，需要回退到前缀
# exit 0
# aaab
```

#### 用例8结果：

```bash
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'a\nb\n' > e.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg ab e.log; echo "exit=$?"
exit=1
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'aab\n' > f.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg ab f.log; echo "exit=$?"
aab
exit=0
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ printf 'aaab\n' > g.log
MasqRd@LAPTOP-EurO:~/lr-grep/tests$ ./lrg aab g.log; echo "exit=$?"
aaab
exit=0
```
