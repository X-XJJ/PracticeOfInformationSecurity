# PracticeOfInformationSecurity
---
- 信息安全工程实践——漏洞利用开发与检测对抗，三人小组的两周成果

- 红方利用CVE-2010-3333漏洞进行远程木马植入windows XP系统，实现反弹回联、文件上传下载功能、实现远程桌面和鼠标操作功能，实现进程隐藏、文件隐藏、窗口隐藏功能
- 蓝方为受害机的网关，进行网络传输过程的检测和拦截、CVE-2010-3333漏洞利用代码的运行监测和拦截、终端隐藏行为的检测等功能

* 看着两星期的实践，其实也整合了大半个学期的学习成果
* 含改造其他小组的轮子，比如文件隐藏，上传下载等
* 过程：
  - 红方向受害方的邮箱发送doc文件，受害方下载打开doc文件后，自动连接并下载红方服务器的木马程序并自动执行。木马程序对自己的文件和进程进行隐藏，回连红方服务器，使红方对受害方进行远程控制，进行一系列活动，包括运行程序，文件上传下载，cmd调用等
  - 蓝方为网关，使用Bro软件进行流量监测，进行抓包分析，
  - （怎么实验的有空再补……= =）
