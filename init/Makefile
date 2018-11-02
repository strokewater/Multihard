CC = gcc
LD = ld

CFLAGS = -ffreestanding -nostdinc -m32 -Wall -g -I../include/
LDFLAGS = -m elf_i386 -Ttext=0x9048000

SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
PWD=$(shell pwd)
DEPSDIR=.deps

C_SOURCE=${wildcard *.c}
C_OBJS=${patsubst %.c, %.o, $(C_SOURCE)}
C_DEPS=$(C_SOURCE:.c=.d)
C_DEPS:=$(addprefix $(DEPSDIR)/,$(C_DEPS))


OBJS = $(C_OBJS)

init: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o init

.PHONY: clean
clean:
	rm -rf $(OBJS) $(DEPSDIR) init

ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif

$(DEPSDIR)/%.d:%.c
	@set -e;\
	mkdir -p $(DEPSDIR) ;\
	rm -rf $@.tmp;\
	$(CC) -E -MM $(CFLAGS) $(filter %.c,$^) > $@.tmp || exit "$$?" ;\
	sed 's,\(.*\)\.o[:]*,\1.o:,g' <$@.tmp >$@;\
	rm -rf $@.tmp
