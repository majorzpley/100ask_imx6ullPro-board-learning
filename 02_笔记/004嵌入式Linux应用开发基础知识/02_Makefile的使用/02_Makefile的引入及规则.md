# Makefile规则与示例
- 参考文档:https://www.gnu.org/software/make/manual/make.html

- Makefile的核心---规则：
```Makefile
目标 : 依赖1 依赖2 ...
[TAB]命令
当"目标文件"不存在，
或
某个依赖文件比目标文件"新",
则：执行"命令"
```