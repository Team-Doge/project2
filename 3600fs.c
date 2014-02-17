/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 * This file contains all of the basic functions that you will need 
 * to implement for this project.  Please see the project handout
 * for more details on any particular function, and ask on Piazza if
 * you get stuck.
 */

#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#define _POSIX_C_SOURCE 199309

#include <time.h>
#include <fuse.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <sys/statfs.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "3600fs.h"
#include "disk.h"

/*
 * Initialize filesystem. Read in file system metadata and initialize
 * memory structures. If there are inconsistencies, now would also be
 * a good time to deal with that. 
 *
 * HINT: You don't need to deal with the 'conn' parameter AND you may
 * just return NULL.
 *
 */
static void* vfs_mount(struct fuse_conn_info *conn) {
    fprintf(stderr, "vfs_mount called\n");

    // Do not touch or move this code; connects the disk
    dconnect();

    /* 3600: YOU SHOULD ADD CODE HERE TO CHECK THE CONSISTENCY OF YOUR DISK
       AND LOAD ANY DATA STRUCTURES INTO MEMORY */

    // Read in the VCB
    char* vcb_tmp = malloc(sizeof(vcb));
    if (dread(0, vcb_tmp) < 0) {
        perror("Error reading vcb from disk.");
    }

    // Read in the root dnode
    char* root_tmp = malloc(sizeof(dnode));
    if (dread(1, root_tmp) < 0) {
        perror("Error reading root directory from disk.");
    }

    // Read in the root directory entries
    char* root_dir_tmp = malloc(sizeof(dirent));
    if (dread(2, root_dir_tmp) < 0) {
        perror("Error reading root directory entries from disk.");
    }

    printf("\n\n\n******************\n   MOUNTING DISK   \n*******************\n");  

    printf("*** VCB INFO ***\n");
    vcb myvcb;
    memcpy(&myvcb, vcb_tmp, sizeof(vcb));

    printf("Magic number: %d\n", myvcb.magic);
    printf("Block size: %d\n", myvcb.blocksize);
    printf("Root:\n\tBlock: %d\n\tvalid: %d\n", myvcb.root.block, myvcb.root.valid);
    printf("Free:\n\tBlock: %d\n\tvalid: %d\n", myvcb.free.block, myvcb.free.valid);

    printf("\n\n*** ROOT INFO ***\n");
    dnode root;
    memcpy(&root, root_tmp, sizeof(dnode));

    printf("User id: %d\n", root.user);
    printf("Group id: %d\n", root.group);
    printf("Mode: %d\n", root.mode);

    printf("Create time: %lld.%.9ld\n", (long long)root.create_time.tv_sec, root.create_time.tv_nsec);
    printf("Modify time: %lld.%.9ld\n", (long long)root.modify_time.tv_sec, root.modify_time.tv_nsec);
    printf("Access time: %lld.%.9ld\n", (long long)root.access_time.tv_sec, root.access_time.tv_nsec);

    printf("\nFirst direct block:\n\tBlock: %d\n\tValid: %d\n", root.direct[0].block, root.direct[0].valid);

    printf("\n\n*** ROOT DIRECTORY INFO ***\n");
    dirent root_dir;
    memcpy(&root_dir, root_dir_tmp, sizeof(dirent));

    printf("First direntry:\n\tName: %s\n\tType: %c\n", root_dir.entries[0].name, root_dir.entries[0].type);
    printf("\tBlocknum:\n\t\tBlock: %d\n\t\tValid: %d\n", root_dir.entries[0].block.block, root_dir.entries[0].block.valid);

    printf("Second direntry:\n\tName: %s\n\tType: %c\n", root_dir.entries[1].name, root_dir.entries[1].type);
    printf("\tBlocknum:\n\t\tBlock: %d\n\t\tValid: %d\n", root_dir.entries[1].block.block, root_dir.entries[1].block.valid);

    printf("\n\n\n\n\n\n\n\n\n");

    return NULL;
}

/*
 * Called when your file system is unmounted.
 *
 */
static void vfs_unmount (void *private_data) {
    fprintf(stderr, "vfs_unmount called\n");

    /* 3600: YOU SHOULD ADD CODE HERE TO MAKE SURE YOUR ON-DISK STRUCTURES
       ARE IN-SYNC BEFORE THE DISK IS UNMOUNTED (ONLY NECESSARY IF YOU
       KEEP DATA CACHED THAT'S NOT ON DISK */

    // Do not touch or move this code; unconnects the disk
    dunconnect();
}

/* 
 *
 * Given an absolute path to a file/directory (i.e., /foo ---all
 * paths will start with the root directory of the CS3600 file
 * system, "/"), you need to return the file attributes that is
 * similar stat system call.
 *
 * HINT: You must implement stbuf->stmode, stbuf->st_size, and
 * stbuf->st_blocks correctly.
 *
 */
static int vfs_getattr(const char *path, struct stat *stbuf) {
    fprintf(stderr, "vfs_getattr called\n");

    // Do not mess with this code 
    stbuf->st_nlink = 1; // hard links
    stbuf->st_rdev  = 0;
    stbuf->st_blksize = BLOCKSIZE;

    /* 3600: YOU MUST UNCOMMENT BELOW AND IMPLEMENT THIS CORRECTLY */
    if (strcmp(path, "/") == 0) {
        printf("\nSUCCESS: Yay we are in the root directory!\n");
        stbuf->st_mode = 0777 | S_IFDIR;
    } else {
        printf("\nSUCCESS: We are in the directory: %s\n", path);
    }
    /*
       if (The path represents the root directory)
       stbuf->st_mode  = 0777 | S_IFDIR;
       else 
       stbuf->st_mode  = <<file mode>> | S_IFREG;

       stbuf->st_uid     = // file uid
       stbuf->st_gid     = // file gid
       stbuf->st_atime   = // access time 
       stbuf->st_mtime   = // modify time
       stbuf->st_ctime   = // create time
       stbuf->st_size    = // file size
       stbuf->st_blocks  = // file size in blocks
     */

    return 0;
}

/*
 * Given an absolute path to a directory (which may or may not end in
 * '/'), vfs_mkdir will create a new directory named dirname in that
 * directory, and will create it with the specified initial mode.
 *
 * HINT: Don't forget to create . and .. while creating a
 * directory.
 */
/*
 * NOTE: YOU CAN IGNORE THIS METHOD, UNLESS YOU ARE COMPLETING THE 
 *       EXTRA CREDIT PORTION OF THE PROJECT.  IF SO, YOU SHOULD
 *       UN-COMMENT THIS METHOD.
 static int vfs_mkdir(const char *path, mode_t mode) {

 return -1;
 } */

/** Read directory
 *
 * Given an absolute path to a directory, vfs_readdir will return 
 * all the files and directories in that directory.
 *
 * HINT:
 * Use the filler parameter to fill in, look at fusexmp.c to see an example
 * Prototype below
 *
 * Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *                                 const struct stat *stbuf, off_t off);
 *			   
 * Your solution should not need to touch fi
 *
 */
static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
    printf("\n\n\n\n");
    printf("Path to be read: '%s'\n", path);
    printf("Offset: %d\n", offset);

    if (strcmp("/", path) != 0) {
        perror("ERROR: Not listing '/'.\n");
        return -1;
    }

    // Read in the root dnode
    char* root_tmp = malloc(sizeof(dnode));
    if (dread(1, root_tmp) < 0) {
        perror("Error reading root directory from disk.");
    }
    dnode root;
    memcpy(&root, root_tmp, sizeof(dnode));

    // Step through the direct blocks
    for (int i = 0; i < 54; i++) {
        printf("GETTING FILES FROM DIRECTORY.\n");
        printf("Reading direct block %d\n", i);

        blocknum b = root.direct[i];
        printf("Checking validity of direct block %d...\n", i);
        if (b.valid == 0) {
            printf("\tDirect block %d was invalid.\n", i);
            continue;
        }
        printf("\tDirect block %d was valid!\n", i);

        int bnum = b.block;
        printf("Block number to read data from: %d\n", bnum);
        char* dir_tmp = malloc(sizeof(dirent));
        int result = dread(bnum, dir_tmp);
        if (result < 0) {
            printf("\n\nERROR: Could not read blocknum %d from disk.\n", bnum);
            return -1;
        }

        dirent dir;
        memcpy(&dir, dir_tmp, sizeof(dirent));

        for (int j = 0; j < 8; j++) {
            direntry entry = dir.entries[j];
            
            if (entry.block.block != 1) {
                continue;
            }

            printf("\nVALID ENTRY: %s\n", entry.name);
        }
    }

    // Step through the single-layer indirection
    // TODO

    // Step through the double-layer indirection
    // TODO

    return 0;
}

/*
 * Given an absolute path to a file (for example /a/b/myFile), vfs_create 
 * will create a new file named myFile in the /a/b directory.
 *
 */
static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    return 0;
}

/*
 * The function vfs_read provides the ability to read data from 
 * an absolute path 'path,' which should specify an existing file.
 * It will attempt to read 'size' bytes starting at the specified
 * offset (offset) from the specified file (path)
 * on your filesystem into the memory address 'buf'. The return 
 * value is the amount of bytes actually read; if the file is 
 * smaller than size, vfs_read will simply return the most amount
 * of bytes it could read. 
 *
 * HINT: You should be able to ignore 'fi'
 *
 */
static int vfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{

    return 0;
}

/*
 * The function vfs_write will attempt to write 'size' bytes from 
 * memory address 'buf' into a file specified by an absolute 'path'.
 * It should do so starting at the specified offset 'offset'.  If
 * offset is beyond the current size of the file, you should pad the
 * file with 0s until you reach the appropriate length.
 *
 * You should return the number of bytes written.
 *
 * HINT: Ignore 'fi'
 */
static int vfs_write(const char *path, const char *buf, size_t size,
        off_t offset, struct fuse_file_info *fi)
{

    /* 3600: NOTE THAT IF THE OFFSET+SIZE GOES OFF THE END OF THE FILE, YOU
       MAY HAVE TO EXTEND THE FILE (ALLOCATE MORE BLOCKS TO IT). */

    return 0;
}

/**
 * This function deletes the last component of the path (e.g., /a/b/c you 
 * need to remove the file 'c' from the directory /a/b).
 */
static int vfs_delete(const char *path)
{

    /* 3600: NOTE THAT THE BLOCKS CORRESPONDING TO THE FILE SHOULD BE MARKED
       AS FREE, AND YOU SHOULD MAKE THEM AVAILABLE TO BE USED WITH OTHER FILES */

    return 0;
}

/*
 * The function rename will rename a file or directory named by the
 * string 'oldpath' and rename it to the file name specified by 'newpath'.
 *
 * HINT: Renaming could also be moving in disguise
 *
 */
static int vfs_rename(const char *from, const char *to)
{

    return 0;
}


/*
 * This function will change the permissions on the file
 * to be mode.  This should only update the file's mode.  
 * Only the permission bits of mode should be examined 
 * (basically, the last 16 bits).  You should do something like
 * 
 * fcb->mode = (mode & 0x0000ffff);
 *
 */
static int vfs_chmod(const char *file, mode_t mode)
{

    return 0;
}

/*
 * This function will change the user and group of the file
 * to be uid and gid.  This should only update the file's owner
 * and group.
 */
static int vfs_chown(const char *file, uid_t uid, gid_t gid)
{

    return 0;
}

/*
 * This function will update the file's last accessed time to
 * be ts[0] and will update the file's last modified time to be ts[1].
 */
static int vfs_utimens(const char *file, const struct timespec ts[2])
{

    return 0;
}

/*
 * This function will truncate the file at the given offset
 * (essentially, it should shorten the file to only be offset
 * bytes long).
 */
static int vfs_truncate(const char *file, off_t offset)
{

    /* 3600: NOTE THAT ANY BLOCKS FREED BY THIS OPERATION SHOULD
       BE AVAILABLE FOR OTHER FILES TO USE. */

    return 0;
}


/*
 * You shouldn't mess with this; it sets up FUSE
 *
 * NOTE: If you're supporting multiple directories for extra credit,
 * you should add 
 *
 *     .mkdir	 = vfs_mkdir,
 */
static struct fuse_operations vfs_oper = {
    .init    = vfs_mount,
    .destroy = vfs_unmount,
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .create	 = vfs_create,
    .read	 = vfs_read,
    .write	 = vfs_write,
    .unlink	 = vfs_delete,
    .rename	 = vfs_rename,
    .chmod	 = vfs_chmod,
    .chown	 = vfs_chown,
    .utimens	 = vfs_utimens,
    .truncate	 = vfs_truncate,
};

int main(int argc, char *argv[]) {
    /* Do not modify this function */
    umask(0);
    if ((argc < 4) || (strcmp("-s", argv[1])) || (strcmp("-d", argv[2]))) {
        printf("Usage: ./3600fs -s -d <dir>\n");
        exit(-1);
    }
    return fuse_main(argc, argv, &vfs_oper, NULL);
}

