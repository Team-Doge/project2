/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 */

#ifndef __3600FS_H__
#define __3600FS_H__

// The number of the block and if it is valid (written to) or not
typedef struct blocknum_t {
    int index;
    int valid;
} blocknum;

// The VCB for the file system
typedef struct vcb_s {
    // Our magic vcb number
    int magic;

    // the block size
    int blocksize;

    // the location of the root dnode
    blocknum root;

    // the location of the first free block
    blocknum free;

    // the name of the disk
    char name[488];
} vcb;

// The directory node that contains information about the directory
// and it's contents
typedef struct dnode_t {
    // directory node metadata
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

typedef struct direntry_t {
    char name[55];
    char type;
    blocknum block;
} direntry;

typedef struct dirent_t {
    direntry entries[8];
} dirent;


typedef struct inode_t {
    // the file metadata
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

typedef struct free_t {
    blocknum next;
    char junk[504];
} freeb;

typedef struct indirect_t {
    blocknum blocks[128];
} indirect;

typedef struct {
  long tv_sec;
  long tv_usec;
} timeval;

// Import helper functions
#include "3600helper.h"

#endif
