/*
 * CS3600, Spring 2014
 * Project 2 Helper Functions
 * (c) 2014 Josh Caron, Michelle Suk
 *
 */

// Helper functions for reading/writing
int bwrite(int blocknum, void *buf);
char* bread(int blocknum);

// Helper functions for dealing with dirents
int find_file_attr(dirent *dir, const char *path, struct stat *stbuf);

// Helper functiongs dealing with file creation
// int create_file(dirent* dir, int dirent_index, int entry_index, const char *path, mode_t mode, struct fuse_file_info *fi);
// int create_dirent(dirent* dir, const char *path, mode_t mode, struct fuse_file_info *fi);

// Debugger functions
void debug_dirent(dirent* d);
void debug_direntry(direntry* e);
void debug_block(blocknum* b);
void debug_inode(inode* i);
void debug_dnode(dnode* d);
void debug_vcb(vcb* v);
void debug_freeb(freeb* f);
