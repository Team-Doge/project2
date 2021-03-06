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

/**
 * Find the file entry for the file located at the given path
 * @param  blocks       The list of blocks to look in
 * @param  size         The size of the list of blocks
 * @param  path         The name of the file
 * @param  dir          The dirent to set if the file is found
 * @param  block_index  The index in the list of blocks the dirent was located at
 * @param  entry_index  The index in the dirent the file was located at
 * @return              Whether or not the file was found
 */
bool find_file_entry(blocknum *blocks, int size, const char *path, blocknum *dir_loc, dirent *dir, int *entry_index);
int write_to_file(inode *file, const char *buf, size_t size, off_t offset);
int read_from_file(inode *file, char *buf, size_t size, off_t offset);
int read_from_block(blocknum block, off_t *offset, char *buf, int buf_pos, size_t size);
int get_file_attr(direntry entry, struct stat *stbuf);

int write_data_to_block_list(blocknum *blocks, int list_size, int size, off_t *offset, const char *buf, int buf_pos);
int write_data_to_block(db *data, off_t *offset, int size, const char *buf, int buf_pos);

int read_from_block_list(blocknum *blocks, int list_size, off_t *offset, char *buf, int buf_pos, size_t size);

void add_block_list_to_free_list(blocknum *blocks, int size);
void add_block_to_free_list(blocknum block);
void truncate_file(inode *file, off_t offset);
int search_root_for_file(const char *path, blocknum *dirent_loc, dirent *dir, int *entry_index);
// Helper functions for dealing with dirents
// int find_file_attr(dirent *dir, const char *path, struct stat *stbuf);

// Helper functiongs dealing with file creation
// int create_file(dirent* dir, int dirent_index, int entry_index, const char *path, mode_t mode, struct fuse_file_info *fi);
// int create_dirent(dirent* dir, const char *path, mode_t mode, struct fuse_file_info *fi);
#endif
