驱动编译方法：
	1. 切换到Android源代码根目录下
	2. 然后在Android源代码目录下执行命令：$ source build/envsetup.sh
	3. 再执行lunch命令来选择命令
	4. 然后切换到驱动程序所在目录下，修改Makefile文件中的ANDROIDSRC，使得
		ANDROIDSRC的值指向Android源代码的跟目录。
	5. 执行make命令进行编译驱动源代码
	6. 最终生成对应的.ko文件，此文件为驱动的目标文件。

	
	
	具体用法：

比如android源码目录为/media/m8803/m8803_2.3_2306_20110812
编绎步骤：
1，切到源码目录：cd /media/m8803/m8803_2.3_2306_20110812
2，source build/envsetup.sh
3，lunch 相应目标平台
4，	编绎驱动时修改Makefile：ANDROIDSRC := /media/m8803/m8803_2.3_2306_20110812
　　修改为对应的平台下的android源码目录
5，cd flysemdriver
6，make
7，生成的.ko文件为目标驱动文件，通过开机脚本安装到系统中
8，	API调用方法可参考目录下的测试文件，注意包含Semcmd.h头文件
9，	编绎测试程序：修改Android.mk里的源码目录，
　　并cd /media/m8803/m8803_2.3_2306_20110812，再mmm flysemdriver
　　
测试程序：
　　Android.mk	：makefile
　　Testsem.c		：测试程序主程序源码
　　
驱动源码：
　　Makefile	
　　Flysemdriver.c：驱动源码
　　Semcmd.h	：驱动依赖的头文件
　　
　　驱动功能接口的说明见semcmd.h

驱动实现：
　　加载驱动时初始化三对信号量
　　之后通过ioctl接口与驱动进行交互
　　
SEM_LOCK://用户加锁
SEM_UNLOCK://用户解锁
第一对锁，相当于文件锁，可独立使用以同步和互斥各个进、线程。

另外两对接口是两个传输数据的通道，四字节的数据，利用内核提供的信号量API实现了一个典型的无缓冲的生产者、消费者模型。
