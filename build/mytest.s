.constants:
0 S "main"
1 S "digits,"
2 S "characters total"
.start:
.functions:
0 0 0 1
.F0:
0   snew 1
1   ipush 0
2   ipush 0
3   loada 0, 0
4   cscan
5   istore
6   jmp 36
7   loada 0, 0
8   iload
9   iprint
10   printl
11   loada 0, 0
12   iload
13   bipush 48
14   icmp
15   jl 27
16   loada 0, 0
17   iload
18   bipush 57
19   icmp
20   jg 27
21   loada 0, 1
22   loada 0, 1
23   iload
24   ipush 1
25   iadd
26   istore
27   loada 0, 2
28   loada 0, 2
29   iload
30   ipush 1
31   iadd
32   istore
33   loada 0, 0
34   cscan
35   istore
36   loada 0, 0
37   iload
38   bipush 46
39   icmp
40   jne 7
41   loada 0, 1
42   iload
43   iprint
44   bipush 32
45   cprint
46   loadc 1
47   sprint
48   bipush 32
49   cprint
50   loada 0, 2
51   iload
52   iprint
53   bipush 32
54   cprint
55   loadc 2
56   sprint
57   printl
58   ipush 0
59   iret
