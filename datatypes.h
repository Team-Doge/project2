
#ifndef __3600DATATYPES_H__
#define __3600DATATYPES_H__
/**
 * Represents the location of a block on disk.
 *
 * int  index        The block number location on disk
 * bool valid    Whether or not there is data at the location on disk
 *                      false - Free to write to, no data
 *                      true  - Data exists
 */
typedef struct blocknum_t {
    int index;
    bool valid;
} blocknum;


/**
 * Represents the VCB of the file system. Viewed as the 'master' block that has the location
 * of the root directory on disk and where the free blocks are
 *
 * int          magic       The magic number of our disk. Checked on disk mount to make sure the disk is
 *                              formatted properly
 * int          blocksize   The number of bytes each block takes up on disk
 * blocknum     root        The block location of the root directory on disk
 * blocknum     free        The block location of where the free blocks begin on disk
 * char        *name        The name of the disk
 */
typedef struct vcb_s {
    int magic;
    int blocksize;
    blocknum root;
    blocknum free;
    char name[488];
} vcb;


/**
 * Represents a free block on disk that can be written to.
 *
 * blocknum     next    The block location of the next available free block following this one
 * char*        junk    Unused data to make sure the free block takes up a full BLOCKSIZE
 */
typedef struct free_t {
    blocknum next;
    char junk[504];
} freeb;


/**
 * Represents a indirection block on disk.
 *
 * blocknum*    blocks      The list of block locations that the indirection points to
 *
 * NOTES
 * For DNODE:
 *     For a directory node using single indirection, each blocknum points to the location on disk
 *     of a dirent. For double indirection, each blocknum points to a single indirection.
 * For INODE:
 *     For a file using indirection, each blocknum points to the location on disk of a data block. For
 *     double indirection, each blocknum points to a single indirection.
 */
typedef struct indirect_t {
    blocknum blocks[64];
} indirect;


/**
 * Represents a file on disk, and contain's the file's metadata.
 *
 * unsigned int size    How big the file is in bytes
 * uid_t        user    The id of the user who owns the file
 * gid_t        group   The id of the group who owns the file
 * mode_t       mode    The permissions associated with this file
 * timespec     access_time     The last time the file was accessed
 * timespec     modify_time     The last time the file was modified
 * timespec     create_time     The time when the file was created
 * blocknum    *direct  A listing of blocknums that point to blocks on disk that contain data blocks
 * blocknum     single_indirect The block location of the single indirect block
 * blocknum     double_indirect The block location of the double indirect block
 */
typedef struct inode_t {
    unsigned int size;
    uid_t user;
    gid_t group;
    mode_t mode;
    struct timespec access_time;
    struct timespec modify_time;
    struct timespec create_time;
    blocknum direct[54];
    blocknum single_indirect;
    blocknum double_indirect;
} inode;


/**
 * Represents a directory node in the file system, and contains the directory's metadata
 *
 * unsigned int size    How big the directory is in bytes
 * uid_t        user    The id of the user who owns the directory
 * gid_t        group   The id of the group who owns the directory
 * mode_t       mode    The permissions associated with this directory
 * timespec     access_time     The last time the directory was accessed
 * timespec     modify_time     The last time the directory was modified
 * timespec     create_time     The time when the directory was created
 * blocknum    *direct          A listing of blocknums that point to blocks on disk that contain dirents
 * blocknum     single_indirect The block location of the single indirect block
 * blocknum     double_indirect The block location of the double indirect block
 */
typedef struct dnode_t {
    unsigned int size;
    uid_t user;
    gid_t group;
    mode_t mode;
    struct timespec access_time;
    struct timespec modify_time;
    struct timespec create_time;
    blocknum direct[54];
    blocknum single_indirect;
    blocknum double_indirect;
} dnode;


/**
 * Represents a entry in the filesystem.
 *
 * char    *name   The name of the file on disk
 * char     type    The type of file on disk
 *                      f - Represents a file
 *                      d - Represents a directory
 * blocknum block   The block location of the metadata for the entry, which is either an inode or
 *                  a dnode
 */
typedef struct direntry_t {
    char name[55];
    char type;
    blocknum block;
} direntry;


/**
 * Represents a list of entries in the filesystem.
 *
 * direntry    *entries     A list of direntry's
 */
typedef struct dirent_t {
    direntry entries[8];
} dirent;


/**
 * Represents a block of data for a file
 *
 * char     *data   The data in the file
 */
typedef struct db_t {
    char data[512];
} db;

#endif
