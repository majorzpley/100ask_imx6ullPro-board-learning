A = a b c
B = $(foreach f,$(A),$(f).o)

C = a b c d/

D = $(filter %/, $(C))
E = $(filter-out %/, $(C))

files2 = a.c b.c c.c d.c e.c abc
# 在file2中寻找存在的文件
files3 = $(wildcard $(files2))
# 在file2中将.c文件转换为.d文件
depend_files = $(patsubst %.c,%.d,$(files2))
all:
	@echo B = $(B)
	@echo D = $(D)
	@echo E = $(E)
	@echo files = $(files)
	@echo files2 = $(files2)
	@echo files3 = $(files3)
	@echo depend_files = $(depend_files)