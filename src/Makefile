#引用全局变量
-include ../Makefile.env

#获得当前目录的所有子目录（.和..除外）。
SUBDIRS = $(shell ls -l| grep ^d |awk '{print $$9}' )

all:subdirs

#执行所有子目录的makefile
subdirs: $(SUBDIRS)
	@for dir in $(SUBDIRS);do $(MAKE) -C $$dir all||exit 1;done

.PHONY: all