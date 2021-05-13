#我不是很懂写makefile makefile太难写了，还是cmake好懂一点
SHELL = /bin/sh
CC = g++

CFLAG = -W -Wall -O3 -Wno-reorder -Wno-unused-parameter -Wno-format
#下面是CFLAG的说明， 
#https://blog.csdn.net/zhaoyun_zzz/article/details/82466031?utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-1.vipsorttest&depth_
#1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-1.vipsorttest

INCLUDE_PATH = -I.
HTTP_TARGET = tinyhttp
HTTP_SRC = http.cpp tinyhttp.cpp

all:
		$(CC) $(CFLAG) $(INCLUDE_PATH) $(HTTP_SRC) -o $(HTTP_TARGET) -lpthread

clean:
		rm -f *.o $(HTTP_TARGET)