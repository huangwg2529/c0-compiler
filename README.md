# c0-compiler

&emsp;&emsp;编译课程大作业，C0 编译器的实现。

## 1. 概述

&emsp;&emsp;这学期我们的编译实验进行了较大的改革，不再是以往直接在最后几周实现一个 C0 编译器，而是助教先为我们提供了一个 miniplc0 小型编译器的源代码，前三周由我们填补上挖空内容从而使它正常运行。从第四周到第七周，我们需要完整实现 C0 编译器，可以复用助教的 miniplc0 代码。

&emsp;&emsp;在实验要求里，两个编译器都需要在助教提供的 docker 环境里编译然后测试。

## 2. 指导书

&emsp;&emsp;不得不说助教提供的 miniplc0 对我们加深对编译器的认识很有帮助，有了它能快速结合课堂内容加深理解。并且指导书写得是真的非常详细。感谢 lazymio 和 hambaka 两位助教，带我快乐地做编译器（以及学习 C++. hhh

&emsp;&emsp;学习顺序：miniplc0 指导书 -> miniplc0 源码 -> 填补 miniplc0 -> 实现 c0.

下面是各个部分内容的链接：

[miniplc0 指导书](https://github.com/BUAA-SE-Compiling/miniplc0-handbook)

[miniplc0 待补源码](https://github.com/BUAA-SE-Compiling/miniplc0-compiler)

[miniplc0 补充完整，助教测试满分通过](https://github.com/huangwg2529/c0-compiler/tree/master/miniplc0)

[miniplc0 虚拟机标准](https://github.com/BUAA-SE-Compiling/miniplc0-vm-standards)

[C0 指导书](https://github.com/BUAA-SE-Compiling/c0-handbook)

[c0 栈式虚拟机标准](https://github.com/BUAA-SE-Compiling/c0-vm-standards)

[c0 虚拟机工具链](https://github.com/BUAA-SE-Compiling/c0-vm-cpp)

## 3. 具体实现

### 1. 词法分析

&emsp;&emsp;词法分析直接在助教提供的 miniplc0 里的实现基础上进行修改，维护了一个临时的自动机，很有意思。对于源文件，则是一次全部读入缓冲区，用指针指向下一个将要读取的字符，并设置了行列号。

&emsp;&emsp;这里使用了 c++17 的 ```std::any``` 来保存任意类型的 Token，使用 ```std::optional``` 来处理空对象的返回值。

### 2. 语法分析与语法制导翻译

&emsp;&emsp;所谓语法制导翻译，指的是一边进行语法分析一边进行语义分析和代码生成。

#### 1. 递归下降子程序

&emsp;&emsp;语法分析使用递归下降子程序来实现，某些地方文法条数过多则直接让它回溯，没有提取所有 FirstVT 集。为了减轻回溯带来的性能影响可以设计一个缓冲区，不过时间来不及没有设计。返回值依然使用 ```std::optional``` 来处理。

#### 2. 错误处理

&emsp;&emsp;由于词法分析过程处理行号和列号，所以错误处理可以输出错误位置、错误信息。当遇到错误时，各函数的返回值返回错误信息，编译器会直接终止。（助教说我错误处理信息不够详细，不过指导书并没有要求错误处理的形式，就比较偷懒了）

#### 3. 符号表管理

&emsp;&emsp;设计了一个 ```Symbol``` 类来保存每个符号的详细信息：

1. 标识符名字
2. 是否为函数
3. 数据类型
4. 是否初始化(对于变量而言)
5. 是否 const (对于变量而言)
6. 参数数量(对于函数而言)

&emsp;&emsp;然后设计了 ```SymTable``` 类作为符号表，使用 vector 保存各个符号，并在这里实现了符号表所需的各个函数。

&emsp;&emsp;由于汇编代码需要单独输出常量表，所以把常量 (函数名字、double、字符串字面量...) 单独设置了一个表，而全局变量则与局部变量一块放在一个 map 里，用 key 来标识是全局变量还是哪个函数的局部变量。

#### 4. 语义分析

1. 与符号表相关的语义，如标识符的重定义、变量是否初始化、变量是否为 const 等出现在整个语法分析的各个地方，此外还有函数参数数量、类型，函数返回值等，所以在每个递归下降子程序里设置了 funcIndex 参数标明这是哪个函数，从而查询全局符号表或局部符号表。
2. 与表达式相关的语义，例如表达式的数据类型、强制类型转换等，都需要在各层表达式子程序里传递数据类型，这里使用引用参数的方法获取表达式类型并在每个子程序里进行管理。
3. 为了确保语句块每个分支都有返回语句，这里同样使用引用参数标明该分支是否有返回语句。
   
#### 5. 代码生成

&emsp;&emsp;在进行语法分析的同时直接生成汇编指令。指令由一个 ```std::map<int32_t, std::vector<Instruction>>``` 来保存，key 标识这是哪个函数或全局变量的汇编指令。

### 3. 生成二进制目标代码

&emsp;&emsp;很大程度上参考了助教的代码。因为自己写的实在是太难看了，考虑到这不是主要的得分点，那还是直接参考助教虚拟机里的实现吧。

## 4. docker 的使用

&emsp;&emsp;在整个实验过程中，我全都在助教提供的 docker 环境里编译运行，可以避免别人出现的本地能跑测试出错的情况。

&emsp;&emsp;win10 家庭版整 docker 太麻烦了，于是我在 VMware 里的 ubuntu 安装 docker。为了能在 win10 进行 coding，我设置了 win10 与 ubuntu 的共享文件夹，然后在 docker 运行时挂载了 ubuntu 里那个共享文件夹，于是代码放在这里，win10 里进行 coding，进入 VMware 的 docker 编译测试。

&emsp;&emsp;docker 环境如下：

```sh
docker pull lazymio/compilers-env
```



