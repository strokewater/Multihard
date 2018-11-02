#include "sched.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "ctype.h"
#include "config.h"
#include "elf.h"
#include "printk.h"
#include "process.h"
#include "asm/asm.h"
#include "asm/pm.h"
#include "asm/int_content.h"
#include "mm/mm.h"
#include "mm/vzone.h"
#include "mm/varea.h"
#include "gfs/gfs.h"
#include "gfs/gfs_utils.h"
#include "gfs/gfslevel.h"
#include "gfs/syslevel.h"
#include "gfs/stat.h"

#define SH_LINE_MAX_CHARS	50

static void FreeAllVZones()
{
		int i;
		for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
		{
			if (Current->vzones[i] && Current->vzones[i]->vm_start != (void *)PROCESS_ARG_ENV_TMP_LOWER)
			{
				DetachVZone(Current, Current->vzones[i]);
				Current->vzones[i] = NULL;
			}
		}
		Current->brk_offset_in_vzones = -1;
}

static int ExecvReadSHWord(char word[], struct File *fp)
{
	char buf[10];
	int nbuf;
	int i;
	
	char *old_word = word;
	while ((nbuf = GFSRead(fp, buf, SH_LINE_MAX_CHARS)) > 0)
	{
		i = 0;
		while (isspace(buf[i]))
			++i;
		for (i = 0; i < nbuf && buf[i] != '\n' && !isspace(buf[i]) ; ++i)
			*word++ = buf[i];
		if (isspace(buf[i]) || buf[i] == '\n' || nbuf < SH_LINE_MAX_CHARS)
			break;
	}
	*word = '\0';
		
	if (i < 10)
		fp->pos -= (10 - i - 1);
	
	if (nbuf < 0)
		return nbuf;
	else
		return word - old_word;
}

static int ExecveGetPrio(struct File *fp, Elf32_Ehdr *head)
{
	int ret;
	int pos = fp->pos;
	Elf32_Shdr *shdr;
	char *name;
	int core_dep[2];
	
	shdr = KMalloc(sizeof(*shdr));
	fp->pos = head->e_shoff + head->e_shstrndx * sizeof(Elf32_Shdr);
	GFSRead(fp, shdr, sizeof(*shdr));
	fp->pos = shdr->sh_offset;
	name = KMalloc(shdr->sh_size);
	GFSRead(fp, name, shdr->sh_size);
	
	int i;
	for (i = 0; i < head->e_shnum; ++i)
	{
		fp->pos = head->e_shoff + i * sizeof(Elf32_Shdr);
		GFSRead(fp, shdr, sizeof(*shdr));
		if (strcmp(name + shdr->sh_name, ".core_dep") == 0)
			break;
	}
	
	if (i == head->e_shnum)
		ret = SCHED_DEFAULT_PRIORITY;
	else
	{
		fp->pos = shdr->sh_offset;
		GFSRead(fp, core_dep, 2 * 4);
		if (DaemonInterfereVer != core_dep[0])
			ret = -1;
		ret = core_dep[1];
	}
	
	fp->pos = pos;
	KFree(name, shdr->sh_size);
	KFree(shdr, 0);

	
	return ret;
}

struct ArgEnvTmpArea * const tmp_area = (struct ArgEnvTmpArea *)(PROCESS_ARG_ENV_TMP_LOWER);

int CopyToArgEnvTmpArea(char *interp[], int interp_i, const char *argvs[], const char *envps[])
{
	char *tmp_data_ptr = &(tmp_area->data[0]);
	int i, j;
	struct VZone *tmp_vz;
	
	tmp_vz = AllocVZone((void *)PROCESS_ARG_ENV_TMP_LOWER, NR_PG_PROCESS_ARG_ENV_TMP * PAGE_SIZE, VZONE_CTYPE_DATA,
								VZONE_MAPPED_FILE_ANONYMOUSE_MEM, 0, 0);
	if (tmp_vz == NULL)
		return -ENOMEM;
	AttachVZone(Current, tmp_vz);
	
	int envc = 0;
	int argc = 0;
	tmp_area->argc = 0;

	for (i = 0; i <= interp_i; ++i)
	{
		tmp_area->argvs[argc++] = tmp_data_ptr;
		for (j = 0; interp[i][j]; ++j)
			*tmp_data_ptr++ = interp[i][j];
		*tmp_data_ptr++ = '\0';
	}
	
	for (i = 1; argvs[i]; ++i)
	{
		tmp_area->argvs[argc++] = tmp_data_ptr;
		for (j = 0; argvs[i][j]; ++j)
			*tmp_data_ptr++ = argvs[i][j];
		*tmp_data_ptr++ = '\0';
	}
	tmp_area->argvs[argc] = NULL;
	
	for (i = 0; envps[i]; ++i)
	{
		tmp_area->envps[envc++] = tmp_data_ptr;
		for (j = 0; envps[i][j]; ++j)
			*tmp_data_ptr++ = envps[i][j];
		*tmp_data_ptr++ = '\0';
	}
	
	tmp_area->argc = argc;
	tmp_area->envps[envc] = NULL;
	tmp_area->data_ptr = tmp_data_ptr;
	return tmp_data_ptr - &(tmp_area->data[0]);
}

#define TMP_AREA_TO_STACK(x)	(PROCESS_STACK_UPPER - ( PROCESS_ARG_ENV_TMP_UPPER - ((int)x)))

void RenameProcess(char *filename)
{
	int i, j;
	for (i = 0, j = -1; filename[i]; ++i)
		if (filename[i] == '/')
			j = i;
			
	// [j + 1, i - 1]
	 int len = i - j - 1;
	KFree(Current->name, 0);
	Current->name = KMalloc(len + 1);
	strcpy(Current->name, filename + j + 1);
}

void *CopyArgEnvTmpToStack()
{
	void *stk_ptr = (void *)(PROCESS_STACK_UPPER + 1 - NR_PG_PROCESS_ARG_ENV_TMP * PAGE_SIZE);
	void *ret = stk_ptr;
	*(int *)stk_ptr = tmp_area->argc;
	stk_ptr += 4;
	
	int i;
	for (i = 0; tmp_area->argvs[i]; ++i)
	{
		*(char **)stk_ptr = (char *)TMP_AREA_TO_STACK(tmp_area->argvs[i]);
		stk_ptr += 4;
	}
	*(char **)stk_ptr = NULL;
	stk_ptr += 4;
	
	for (i = 0; tmp_area->envps[i]; ++i)
	{
		*(char **)stk_ptr = (char *)TMP_AREA_TO_STACK(tmp_area->envps[i]);
		stk_ptr += 4;
	}
	*(char **)stk_ptr = NULL;
	stk_ptr += 4;
	
	memcpy((void *)(TMP_AREA_TO_STACK(&(tmp_area->data))), 
			tmp_area->data, 
			tmp_area->data_ptr - tmp_area->data);
	for (i = 0; i < NR_VZONES_PER_PROCESS; ++i)
	{
		if (Current->vzones[i] && Current->vzones[i]->vm_start == (void *)PROCESS_ARG_ENV_TMP_LOWER)
			DetachVZone(Current, Current->vzones[i]);
	}
	return ret;
}

#undef TMP_AREA_TO_STACK

struct File *OpenExecvFile(char *filename)
{
	struct MInode *inode;
	struct File *fp;

	if (*filename == '/')
		inode = GFSReadMInode(Current->root, filename);
	else
		inode = GFSReadMInode(Current->pwd, filename);
		
	if (inode == NULL)
		return NULL;
		
	if (!S_ISREG(inode->mode))
	{
		Current->errno = -EACCES;
		return NULL;
	}
	fp = GFSOpenFileByMInode(inode, MAY_READ | MAY_EXEC);
	return fp;	
}

int SysExecv(char *filename, const char *argvs[], const char *envps[])
{
	int prio = 5;
	int retval = 0;
	void *esp;
	int i;
	struct File *fp;
	char *program_name;
	int len;
	char *interp[10];
	int interp_i = 0;
	int is_ugid_set = 0;
	int is_prio_set = 0;
	uid_t euid = 0;
	gid_t egid = 0;
	Elf32_Ehdr *program_head = NULL;
	Elf32_Phdr *program_phs = NULL;
	int sh_bang = 0;
	int pos;
	debug_printk("[FS]Process %s(PID = %d) Do SysExecv(%s) Start.", Current->name, Current->pid, filename);
		
	len = strlen(filename);
	program_name = KMalloc(len + 1);
	strcpy(program_name, filename);
	
	for (i = 0; i < 10; ++i)
		interp[i] = NULL;
	interp[0] = filename;
	
	program_head = KMalloc(sizeof(*program_head));
Repeat:	
	fp = OpenExecvFile(filename);
	struct VZone *vp;
	int attr;
	int flags;
	
	if (fp == NULL)
	{
		retval = Current->errno;
		goto ErrorRet;
	}
	if (!S_ISREG(fp->inode->mode))
	{
		retval = -EACCES;
		goto ErrorRet;
	}
	if (fp->inode->mode & S_ISUID && !is_ugid_set)
		euid = fp->inode->uid;
	else
		euid = Current->euid;
	
	if (fp->inode->mode & S_ISGID && !is_ugid_set)
		egid = fp->inode->gid;
	else
		egid = Current->egid;
	is_ugid_set = 1;
	GFSRead(fp, program_head, sizeof(*program_head));
	if (memcmp(program_head->e_ident, "#!", 2) == 0)
	{
		fp->pos = 2;
		
		char *old_filename = interp[interp_i];
		--interp_i;
		while (1)
		{
			++interp_i;
			interp[interp_i] = KMalloc(SH_LINE_MAX_CHARS + 1);
			retval = ExecvReadSHWord(interp[interp_i], fp);
			if (retval < 0)
				goto ErrorRet;
			else if (retval == 0)
				break;
			else
			{
				if (!sh_bang)
				{
					filename = interp[interp_i];
					sh_bang = 1;
				}
			}
		}
		interp[++interp_i] = old_filename;
		
		GFSCloseFileByFp(fp);
		goto Repeat;
	} else if (memcmp(program_head->e_ident, ELFMAG, SELFMAG) != 0)
	{
		GFSCloseFileByFp(fp);
		return -ENOEXEC;	// error.
	}
	if (program_head->e_type != ET_EXEC && program_head->e_type != ET_DYN && program_head->e_type != ET_NONE)
	{
		GFSCloseFileByFp(fp);
		return -ENOEXEC;	// error.
	}
	fp->pos = program_head->e_phoff;
	program_phs = KMalloc(sizeof(*program_phs) * program_head->e_phnum);
	fp->pos = program_head->e_phoff;
	GFSRead(fp, program_phs, sizeof(*program_phs) * program_head->e_phnum);
	if (!is_prio_set)
		prio = ExecveGetPrio(fp, program_head);
	is_prio_set = 1;
	for (i = 0; i < program_head->e_phnum; ++i)
	{
		if (program_phs[i].p_type == PT_INTERP)
		{
			pos = fp->pos;
			interp[++interp_i] = KMalloc(program_phs->p_filesz);
			fp->pos = program_phs->p_offset;
			GFSRead(fp, interp[interp_i], program_phs->p_filesz);
			fp->pos = pos;
			GFSCloseFileByFp(fp);
			KFree(program_phs, 0);
			filename = interp[interp_i];
			goto Repeat;
		}
	}
	retval = CopyToArgEnvTmpArea(interp, interp_i, argvs, envps);
	if (retval == 0)
		goto ErrorRet;
	FreeAllVZones();
	vp = AllocVZone((void *)PROCESS_STACK_UPPER, PAGE_SIZE * (NR_PG_PROCESS_ARG_ENV_TMP + NR_PG_PROCESS_STACK), VZONE_CTYPE_STACK, 
					VZONE_MAPPED_FILE_ANONYMOUSE_MEM, 0, 0);
	AttachVZone(Current, vp);
	esp = CopyArgEnvTmpToStack();
	for (i = 0; i < program_head->e_phnum; ++i)
	{
		if (program_phs[i].p_type == PT_LOAD)
		{
			attr = 0;
			flags = program_phs[i].p_flags;
			if (flags & PF_W)
				attr = VZONE_CTYPE_DATA;
			else if((flags & PF_R) && (flags & PF_X))
				attr = VZONE_CTYPE_TEXT;
			else if((flags & PF_R) && !(flags & PF_X))
				attr = VZONE_CTYPE_RODATA;
				
			if (program_phs[i].p_filesz == 0)
			{
				vp = AllocVZone((void *)(program_phs[i].p_vaddr), program_phs[i].p_memsz, attr, 
								fp->inode, program_phs[i].p_offset, program_phs[i].p_filesz);
				if (vp == NULL)
				{
					retval = -ENOMEM;
					goto ErrorRet;
				}
			} else {
				vp = AllocVZone((void *)(program_phs[i].p_vaddr), program_phs[i].p_filesz, attr, 
								fp->inode, program_phs[i].p_offset, program_phs[i].p_filesz);
				if (vp == NULL)
				{
					retval = -ENOMEM;
					goto ErrorRet;
				}
				if (program_phs[i].p_filesz != program_phs[i].p_memsz)
					GrowVZone(vp, program_phs[i].p_memsz - program_phs[i].p_filesz);
			}
			
			AttachVZone(Current, vp);
		}
	}
	CurIntContent->esp = (u32)esp;
	CurIntContent->eip = (u32)program_head->e_entry;
	Current->priority = prio;

	if (prio == SCHED_DEFAULT_PRIORITY)
	{
		CurIntContent->cs = SelectorFlatC3 | SA_RPL3;
		CurIntContent->ss = SelectorFlat3 | SA_RPL3;
		CurIntContent->ds = SelectorFlat3 | SA_RPL3;
		CurIntContent->es = SelectorFlat3 | SA_RPL3;
	}else
	{
		CurIntContent->cs = SelectorFlatC;
		CurIntContent->ss = SelectorFlat;
		CurIntContent->ds = SelectorFlat;
		CurIntContent->es = SelectorFlat;	
	}
	CurIntContent->ebx = 0;
	CurIntContent->ecx = 0;
	CurIntContent->edx = 0;
	CurIntContent->ebp = 0;
	CurIntContent->esi = 0;
	CurIntContent->edi = 0;
	
	if (Current->executable)
		FreeMInode(Current->executable);
	Current->executable = fp->inode;
	
	Current->euid = euid;
	Current->egid = egid;
		

	RenameProcess(program_name);
	GFSCloseFileByFp(fp);
	fp = NULL;

	for (i = 0; i < NR_OPEN; ++i)
	{
		if (Current->close_on_exec & (1 << i))
			SysClose(i);
	}
	retval = 0;

ErrorRet:
	if (fp)
		GFSCloseFileByFp(fp);
	if (program_head)
		KFree(program_head, 0);
	if (program_phs)
		KFree(program_phs, 0);
	for (; interp_i >= 0; --interp_i)
	{
		if (IS_VAREA_MEM(interp[interp_i]))
			KFree(interp[interp_i], 0);
	}
	debug_printk("[FS] Proccess(PID = %d) Do SysExecv finish. return = %d", Current->pid, retval);
	return retval;
}
