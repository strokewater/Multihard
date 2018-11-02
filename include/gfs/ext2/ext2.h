#ifndef FS_EXT2_EXT2_FS_H
#define FS_EXT2_EXT2_FS_H

#include "types.h"

/*
 * Special inode numbers
 */
#define	EXT2_BAD_INO	 1	/* Bad blocks inode */
#define EXT2_ROOT_INO	 2	/* Root inode */
#define EXT2_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define EXT2_UNDEL_DIR_INO	 6	/* Undelete directory inode */
/*
 * The second extended file system magic number
 */
#define EXT2_SUPER_MAGIC	0xEF53

/*
 * Macro-instructions used to manage several block sizes
 */
# define EXT2_BLOCK_SIZE(s)	(1024 << (s)->s_log_block_size)

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	u32	bg_block_bitmap;	/* Blocks bitmap block */
	u32	bg_inode_bitmap;	/* Inodes bitmap block */
	u32 bg_inode_table;	/* Inodes table block */
	u16 bg_free_blocks_count;	/* Free blocks count */
	u16 bg_free_inodes_count;	/* Free inodes count */
	u16 bg_used_dirs_count;	/* Directories count */
	u16 bg_pad;
	u32 bg_reserved[3];
};

#define	EXT2_NDIR_BLOCKS	12
#define	EXT2_IND_BLOCK		EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK	(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK	(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS		(EXT2_TIND_BLOCK + 1)

/*
 * Inode flags
 */
#define	EXT2_SECRM_FL	0x00000001 /* Secure deletion */
#define	EXT2_UNRM_FL	0x00000002 /* Undelete */
#define	EXT2_COMPR_FL	0x00000004 /* Compress file */
#define EXT2_SYNC_FL	0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE_FL	0x00000010 /* Immutable file */
#define EXT2_APPEND_FL	0x00000020 /* writes to file may only append */
#define EXT2_NODUMP_FL	0x00000040 /* do not dump file */
#define EXT2_NOATIME_FL	0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT2_DIRTY_FL	0x00000100
#define EXT2_COMPRBLK_FL	0x00000200 /* One or more compressed clusters */
#define EXT2_NOCOMP_FL	0x00000400 /* Don't compress */
#define EXT2_ECOMPR_FL	0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
#define EXT2_BTREE_FL	0x00001000 /* btree format dir */
#define EXT2_RESERVED_FL	0x80000000 /* reserved for ext2 lib */
#define EXT2_FL_USER_VISIBLE	0x00001FFF /* User visible flags */
#define EXT2_FL_USER_MODIFIABLE	0x000000FF /* User modifiable flags */

/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
	u16	i_mode;	/* File mode */
	u16 i_uid;	/* Low 16 bits of Owner Uid */
	u32	i_size;	/* Size in bytes */
	u32 i_atime;	/* Access time */
	u32	i_ctime;	/* Creation time */
	u32 i_mtime;	/* Modification time */
	u32 i_dtime;	/* Deletion Time */
	u16 i_gid;	/* Low 16 bits of Group Id */
	u16 i_links_count;	/* Links count */
	u32	i_blocks;	/* Blocks count */
	u32 i_flags;	/* File flags */
	
	union {
		struct {
			u32  l_i_reserved1;
		} linux1;
		struct {
			u32  h_i_translator;
		} hurd1;
		struct {
			u32  m_i_reserved1;
		} masix1;
	} osd1;	/* OS dependent 1 */
	
	u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	u32	i_generation;	/* File version (for NFS) */
	u32	i_file_acl;	/* File ACL */
	u32	i_dir_acl;	/* Directory ACL */
	u32	i_faddr;	/* Fragment address */
	union {
		struct {
			u8	l_i_frag;	/* Fragment number */
			u8	l_i_fsize;	/* Fragment size */
			u16	i_pad1;
			u16	l_i_uid_high;	/* these 2 fields    */
			u16	l_i_gid_high;	/* were reserved2[0] */
			u32 l_i_reserved2;
		} linux2;
		struct {
			u8	h_i_frag;	/* Fragment number */
			u8	h_i_fsize;	/* Fragment size */
			u16	h_i_mode_high;
			u16	h_i_uid_high;
			u16	h_i_gid_high;
			u32	h_i_author;
		} hurd2;
		struct {
			u8	m_i_frag;	/* Fragment number */
			u8	m_i_fsize;	/* Fragment size */
			u16	m_pad1;
			u32	m_i_reserved2[2];
		} masix2;
	} osd2;	/* OS dependent 2 */
};
/*
 * File system states
 */
#define	EXT2_VALID_FS	0x0001	/* Unmounted cleanly */
#define	EXT2_ERROR_FS	0x0002	/* Errors detected */
/*
 * Mount flags
 */
#define EXT2_MOUNT_CHECK	0x0001	/* Do mount-time checks */
#define EXT2_MOUNT_GRPID	0x0004	/* Create files with directory's group */
#define EXT2_MOUNT_DEBUG	0x0008	/* Some debugging messages */
#define EXT2_MOUNT_ERRORS_CONT	0x0010	/* Continue on errors */
#define EXT2_MOUNT_ERRORS_RO	0x0020	/* Remount fs ro on errors */
#define EXT2_MOUNT_ERRORS_PANIC	0x0040	/* Panic on errors */
#define EXT2_MOUNT_MINIX_DF	0x0080	/* Mimics the Minix statfs */
#define EXT2_MOUNT_NO_UID32	0x0200  /* Disable 32-bit UIDs */

/*
 * Behaviour when detecting errors
 */
#define EXT2_ERRORS_CONTINUE	1	/* Continue execution */
#define EXT2_ERRORS_RO	2	/* Remount fs read-only */
#define EXT2_ERRORS_PANIC	3	/* Panic */
#define EXT2_ERRORS_DEFAULT	EXT2_ERRORS_CONTINUE

/*
 * Structure of the super block
 */
struct ext2_super_block {
	u32	s_inodes_count;	/* Inodes count */
	u32	s_blocks_count;	/* Blocks count */
	u32	s_r_blocks_count;	/* Reserved blocks count */
	u32	s_free_blocks_count;	/* Free blocks count */
	u32	s_free_inodes_count;	/* Free inodes count */
	u32	s_first_data_block;	/* First Data Block */
	u32	s_log_block_size;	/* Block size */
	s32 s_log_frag_size;	/* Fragment size */
	u32	s_blocks_per_group;	/* # Blocks per group */
	u32	s_frags_per_group;	/* # Fragments per group */
	u32	s_inodes_per_group;	/* # Inodes per group */
	u32	s_mtime;	/* Mount time */
	u32	s_wtime;	/* Write time */
	u16	s_mnt_count;	/* Mount count */
	s16	s_max_mnt_count;	/* Maximal mount count */
	u16	s_magic;	/* Magic signature */
	u16	s_state;	/* File system state */
	u16	s_errors;	/* Behaviour when detecting errors */
	u16	s_minor_rev_level; 	/* minor revision level */
	u32	s_lastcheck;	/* time of last check */
	u32	s_checkinterval;	/* max. time between checks */
	u32	s_creator_os;	/* OS */
	u32	s_rev_level;	/* Revision level */
	u16	s_def_resuid;	/* Default uid for reserved blocks */
	u16	s_def_resgid;	/* Default gid for reserved blocks */
/*
 * These fields are for EXT2_DYNAMIC_REV superblocks only.
 *
 * Note: the difference between the compatible feature set and
 * the incompatible feature set is that if there is a bit set
 * in the incompatible feature set that the kernel doesn't
 * know about, it should refuse to mount the filesystem.
 * 
 * e2fsck's requirements are more strict; if it doesn't know
 * about a feature in either the compatible or incompatible
 * feature set, it must abort and not try to meddle with
 * things it doesn't understand...
 */
	u32	s_first_ino; 	/* First non-reserved inode */
	u16 s_inode_size; 	/* size of inode structure */
	u16	s_block_group_nr; 	/* block group # of this superblock */
	u32	s_feature_compat; 	/* compatible feature set */
	u32	s_feature_incompat; 	/* incompatible feature set */
	u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
	u8	s_uuid[16];	/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	u32	s_algorithm_usage_bitmap; /* For compression */
/*
 * Performance hints.  Directory preallocation should only
 * happen if the EXT2_COMPAT_PREALLOC flag is on.
 */
	u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	u16	s_padding1;
	u32	s_reserved[204];	/* Padding to the end of the block */
};

/*
 * Codes for operating systems
 */
#define EXT2_OS_LINUX	0
#define EXT2_OS_HURD	1
#define EXT2_OS_MASIX	2
#define EXT2_OS_FREEBSD	3
#define EXT2_OS_LITES	4
/*
 * Revision levels
 */
#define EXT2_GOOD_OLD_REV	0	/* The good old (original) format */
#define EXT2_DYNAMIC_REV	1 	/* V2 format w/ dynamic inode sizes */
#define EXT2_CURRENT_REV	EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV	EXT2_DYNAMIC_REV
#define EXT2_GOOD_OLD_INODE_SIZE 128

 /*
 * Structure of a directory entry
 */
#define EXT2_NAME_LEN 255

struct ext2_dir_entry {
	u32	inode;	/* Inode number */
	u16	rec_len;	/* Directory entry length */
	u8	name_len;	/* Name length */
	u8	file_type;
	char	name[EXT2_NAME_LEN];	/* File name */
};


enum {
	EXT2_FT_UNKNOWN		= 0,
	EXT2_FT_REG_FILE	= 1,
	EXT2_FT_DIR			= 2,
	EXT2_FT_CHRDEV		= 3,
	EXT2_FT_BLKDEV		= 4,
	EXT2_FT_FIFO		= 5,
	EXT2_FT_SOCK		= 6,
	EXT2_FT_SYMLINK		= 7,
	EXT2_FT_MAX
};

#define EXT2_FILE_MODE_READ		0
#define EXT2_FILE_MODE_WRITE	1
#define EXT2_FILE_MODE_READA	2

void Ext2Init();
extern int Ext2FSTypeNo;

#endif
