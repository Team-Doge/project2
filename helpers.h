/*
 * CS3600, Spring 2014
 * Project 2 Helper Functions
 * (c) 2014 Josh Caron, Michelle Suk
 *
 */

#ifndef __3600HELPER_H__
#define __3600HELPER_H__
#include "disk.h"
#include "debug.h"

/**
 * Wrapper function for writing data to disk
 * @param  blocknum The block number to write data to
 * @param  buf      The buffered data to write to
 * @return          0 on success or -1 on error.
 */
int bwrite(int blocknum, void *buf);
int bwrite(int blocknum, void *buf) {
    char buffer[BLOCKSIZE];
    memcpy(buffer, buf, BLOCKSIZE);
    int err = dwrite(blocknum, buffer);
    if (err < 0) {
        debug("Error writing block %d to disk.", blocknum);
        return -1;
    }

    return 0;
}


/**
 * Wrapper function for reading from disk
 * @param  blocknum The block number to read data from
 * @return          A char* represented the data that was read in to a buffer, or NULL on failure.
 *
 * NOTE: The char* is calloc'd in this function, make sure to free() it when done
 */
char* bread(int blocknum);
char* bread(int blocknum) {
    char* buffer;
    buffer = (char*) calloc(BLOCKSIZE, sizeof(char));
    if (dread(blocknum, buffer) < 0) {
        debug("Error reading root directory entries from disk.");
        return NULL;
    }

    return buffer;
}

// Helper functions for dealing with dirents
int find_file_attr(dirent *dir, const char *path, struct stat *stbuf);

// Helper functiongs dealing with file creation
// int create_file(dirent* dir, int dirent_index, int entry_index, const char *path, mode_t mode, struct fuse_file_info *fi);
// int create_dirent(dirent* dir, const char *path, mode_t mode, struct fuse_file_info *fi);
#endif
