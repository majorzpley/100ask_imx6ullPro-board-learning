# 定义编译器前缀
PREFIX = arm-buildroot-linux-gnueabihf-
# 定义编译器
CC = $(PREFIX)gcc

# 定义编译选项
CFLAGS = -Wall -g

# 定义目标文件
TARGET = hello

# 定义源文件
SRCS = hello.c

# 定义目标文件的生成规则
$(TARGET):$(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)
# 定义清理规则
.PHONY: clean
clean:
	rm -f $(TARGET)
