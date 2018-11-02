SRC_ROOT=../

include ../Makefile.config

LDFLAGS = -m elf_i386 -Ttext=0x8000

SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
PWD=$(shell pwd)
DEPSDIR=.deps

C_SOURCE=${wildcard *.c}
C_OBJS=${patsubst %.c, %.o, $(C_SOURCE)}
C_DEPS=$(C_SOURCE:.c=.d)
C_DEPS:=$(addprefix $(DEPSDIR)/,$(C_DEPS))


ASM_SOURCE=${wildcard *.asm}
ASM_OBJS=${patsubst %.asm, %.o, $(ASM_SOURCE)}
ASM_DEPS=$(ASM_SOURCE:.asm=.d)
ASM_DEPS:=$(addprefix $(DEPSDIR)/,$(ASM_DEPS))

OBJS = $(C_OBJS)
OBJS += $(ASM_OBJS)

loader: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o loader

.PHONY: clean
clean:
	rm -rf $(OBJS) $(DEPSDIR)
	rm -rf loader
%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif

ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif

$(DEPSDIR)/%.d:%.c
	@set -e;\
	mkdir -p $(DEPSDIR) ;\
	rm -rf $@.tmp;\
	$(CC) -E -MM $(CFLAGS) $(filter %.c,$^) > $@.tmp || exit "$$?" ;\
	sed 's,\(.*\)\.o[:]*,\1.o:,g' <$@.tmp >$@;\
	rm -rf $@.tmp
$(DEPSDIR)/%.d:%.asm
	@set -e;\
	mkdir -p $(DEPSDIR) ;\
	rm -rf $@.tmp;\
	$(ASM) -M $(ASMFLAGS) $(filter %.asm,$^) -o $(patsubst %.asm, %.o, $^) >$@.tmp || exit "$$?" ;\
	sed 's,\(.*\)\.o[:]*,\1.o:,g' <$@.tmp >$@;\
	rm -rf $@.tmp
