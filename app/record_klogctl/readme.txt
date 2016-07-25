record_klogctl.c 
读syslog缓冲区，读取信息后会将缓冲区清空，也就是说dmesg将无法查看信息，但是cat /proc/kmsg可以查看到信息

编译：生成的可执行文件名一定要是record_klogctl！如果要改变编译执行文件的名字，对应的sendsignal中的宏也是
需要改的！


sendsignal.c

遍历/proc目录下status获取进程进程名和目标进程名字匹配，匹配上返回进程id，通过kill向目标进程发送信号，执行
对应操作，目前是将缓冲区的所有内容写入文件。
执行 ./sendsignal STORE  (保存不退出)   或者 ./sendsignal STORE_EXIT  （保存退出）
如果程序正常执行会输出目标程序的pid，否则提示无法获取pid



文件查看
在data/reckmsg目录下可以看到
1.txt 2.txt ...
errormsg.txt
.filename

其中.filename 保存的信息格式“2.txt--2” 意思是当前目录下记录的文件数是1，记录文件是1.txt  下次记录文件数将是2
写入的文件是2.txt,如果文件内容是“2.txt--5”说明当前记录了5个文件，上次覆盖的文件是1.txt 下次覆盖的文件将是2.txt
如果.filename的内容是“1.txt--1”且当前文件夹已经存在1.txt 且程序退出执行了，说明1.txt文件比较大，当前文件夹的容量
已经达到最大限额，.filename的记录是在程序退出时候更新的，程序执行时候查看.filename内容是当前写入的文件名和文件数。

