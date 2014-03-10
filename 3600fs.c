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
#include <sys/time.h>

#ifdef __APPLE__
 #include <sys/statvfs.h>
#else
 #include <sys/statfs.h>
#endif

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "3600fs.h"

// Global variables
vcb global_vcb;
dnode root;
dirent root_dir;
int read_files_in_dir(dirent *dir, void *buf, fuse_fill_dir_t filler);
int create_file(dirent* dir, int dirent_index, int entry_index, const char *path, mode_t mode, struct fuse_file_info *fi);
int create_dirent(dirent *dir, const char *path, mode_t mode, struct fuse_file_info *fi);


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

    // ----- Get rid of the unused conn warning ----- //
    conn = conn + 0;


    // ----- Loading the VCB into memory ----- //
    char* vcb_tmp = malloc(sizeof(vcb));
    if (dread(0, vcb_tmp) < 0) {
        perror("Error reading vcb from disk.");
    }
    memcpy(&global_vcb, vcb_tmp, sizeof(vcb));


    // ----- Loading the root into memory ----- //
    char* root_tmp = malloc(sizeof(dnode));
    if (dread(1, root_tmp) < 0) {
        perror("Error reading root directory from disk.");
    }
    memcpy(&root, root_tmp, sizeof(dnode));


    // ----- Loading the root directory into memory ----- //
    char* root_dir_tmp = malloc(sizeof(dirent));
    if (dread(2, root_dir_tmp) < 0) {
        perror("Error reading root directory entries from disk.");
    }
    memcpy(&root_dir, root_dir_tmp, sizeof(dirent));


    // freeb* free_block;
    // // for (int i = 3; i < 1000; i++) {
    // //     free_block = (freeb*) bread(i);
    // //     debug_freeb(free_block);
    // // }

    // free_block = (freeb*) bread(global_vcb.free.index);
    // while (free_block->next.valid == false) {
    //     printf("Free block at index %d\n", free_block->next.index);
    //     free(free_block);
    //     free_block = (freeb*) bread(free_block->next.index);
    // }

    // ----- DEBUG INFORMATION for mounting disk ----- //
    if (DEBUG) {
        printf("\n\n\n******************\n   MOUNTING DISK   \n*******************\n");
        printf("*** VCB INFO ***\n");
        printf("Magic number: %d\n", global_vcb.magic);
        printf("Block size: %d\n", global_vcb.blocksize);
        printf("Root:\n\tBlock: %d\n\tvalid: %d\n", global_vcb.root.index, global_vcb.root.valid);
        printf("Free:\n\tBlock: %d\n\tvalid: %d\n", global_vcb.free.index, global_vcb.free.valid);
	    printf("Disk name: %s\n", global_vcb.name);
        printf("\n\n*** ROOT INFO ***\n");
        printf("User id: %d\n", root.user);
        printf("Group id: %d\n", root.group);
        printf("Mode: %d\n", root.mode);
        printf("Create time: %lld.%.9ld\n", (long long)root.create_time.tv_sec, root.create_time.tv_nsec);
        printf("Modify time: %lld.%.9ld\n", (long long)root.modify_time.tv_sec, root.modify_time.tv_nsec);
        printf("Access time: %lld.%.9ld\n", (long long)root.access_time.tv_sec, root.access_time.tv_nsec);
        printf("\nFirst direct block:\n\tBlock: %d\n\tValid: %d\n", root.direct[0].index, root.direct[0].valid);

        printf("\n\n*** ROOT DIRECTORY INFO ***\n");
        printf("First direntry:\n\tName: %s\n\tType: %c\n", root_dir.entries[0].name, root_dir.entries[0].type);
        printf("\tBlocknum:\n\t\tBlock: %d\n\t\tValid: %d\n", root_dir.entries[0].block.index, root_dir.entries[0].block.valid);
        printf("Second direntry:\n\tName: %s\n\tType: %c\n", root_dir.entries[1].name, root_dir.entries[1].type);
        printf("\tBlocknum:\n\t\tBlock: %d\n\t\tValid: %d\n", root_dir.entries[1].block.index, root_dir.entries[1].block.valid);

        printf("\n\n\n\n\n\n\n\n\n");
    }

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

    // ----- Get rid of the private_data unsed warning ----- //
    private_data = (void*) private_data;

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

    // ----- Reading stat on the root directory ----- //
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = root.mode | S_IFDIR;
        stbuf->st_uid = root.user;
        stbuf->st_gid = root.group;
        stbuf->st_atime = root.access_time.tv_sec;
        stbuf->st_mtime = root.modify_time.tv_sec;
        stbuf->st_ctime = root.create_time.tv_sec;
        stbuf->st_size = root.size;
        return 0;
    } else {
        // ----- Reading stat on a file----- //
        debug("Searching for file: '%s'\n", path);

        // ----- Step through the direct blocks of root dnode ----- //
        debug("Searching direct blocks.\n");
        direntry entry;
        bool found = find_file_entry(&entry, root.direct, 54, path);
        if (found) {
            return get_file_attr(&entry, stbuf);
        }

        // ----- Step through the first layer of indirection ----- //
        if (root.single_indirect.valid == true) {
            debug("Searching single indirect.\n");
            indirect* ind = (indirect*) bread(root.single_indirect.index);
            direntry entry;
            bool found = find_file_entry(&entry, ind->blocks, 128, path);
            if (found) {
                return get_file_attr(&entry, stbuf);
            }
        }

        // Step through the double-layer indirection if there are more blocks
        if (root.double_indirect.valid == true) {
            debug("Searching double indirect.\n");
            indirect* ind2 = (indirect*) bread(root.double_indirect.index);

            for (int i = 0; i < 128; i++) {
                blocknum b2 = ind2->blocks[i];
                if (b2.valid == false) {
                    debug("File not found.\n");
                    return -ENOENT;
                }

                debug("Searching %d/128 single indirect inside double.\n", i+1);
                indirect* ind1 = (indirect*) bread(b2.index);
                direntry entry;
                bool found = find_file_entry(&entry, ind1->blocks, 128, path);
                if (found) {
                    return get_file_attr(&entry, stbuf);
                }
            }
        }

        return -ENOENT;
    }

    return -1;
}

bool find_file_entry(direntry *entry, blocknum* blocks, int size, const char* path) {
    debug("Looking through %d blocks for '%s'.\n", size, path);
    for (int i = 0; i < size; i++) {
        blocknum b = blocks[i];
        debug("\tBlock %d/%d in list points to block %d.\n", i+1, size, b.index);
        if (b.valid == false) {
            debug("\tBlock %d is invalid.\n", b.index);
            entry = NULL;
            return false;
        }

        dirent *dir = (dirent *) bread(b.index);
        for (int j = 0; j < 8; j++) {
            direntry *e = &dir->entries[j];
            // If it's not valid, skip it
            if (e->block.valid == true) {
                switch (e->type) {
                    case 'd':
                        // TODO
                        // If we support multiple directories, this
                        // is where we would start searching. For now,
                        // we don't, so if for some bizarre reason we come
                        // across this just skip it
                        break;
                    case 'f':
                        if (strncmp(e->name, path+1, 55) == 0) {
                            entry = e;
                            return true;
                        }
                        break;
                    default:
                        // Unknown entry type
                        break;
                }
            }

        }
    }
    
    return false;
}

int get_file_attr(direntry *entry, struct stat *stbuf) {
    // Sanity check
    if (entry->block.valid == true) {
        inode *file = (inode *) bread(entry->block.index);
        stbuf->st_mode = file->mode | S_IFREG;
        stbuf->st_uid = file->user;
        stbuf->st_gid = file->group;
        stbuf->st_atime = file->access_time.tv_sec;
        stbuf->st_mtime = file->modify_time.tv_sec;
        stbuf->st_ctime = file->create_time.tv_sec;
        stbuf->st_size = file->size;

        return 0;
    }

    error("Tried getting attributes on invalid entry.\n");
    return -ENOENT;
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
    // Get rid of compiler warnings about unused parameters
    fi = fi + 0;

    // ----- Check to see what directory is being read ----- //
    debug("Path to be read: '%s'\n", path);
    debug("Offset: %lld\n", (long long) offset);

    // ----- Only continue if we are reading the root directory ----- //
    // TODO:  If we actually support nested directories, remove this
    if (strcmp("/", path) != 0) {
        perror("ERROR: Not listing '/'.\n");
        return -1;
    }

    // TODO: We need to consider the offset, and jump to that block of data
    // ----- Step through the direct blocks of root dnode ----- //
    for (int i = 0; i < 54; i++) {
        blocknum b = root.direct[i];

        // ----- No more entries to read ----- //
        if (b.valid == false) {
            debug("\tDirect block %d was invalid.\n", i);
            return 0;
        }

        // ----- Read in all entries in the dirent ----- //
        dirent* dir = (dirent*) bread(b.index);
        int success = read_files_in_dir(dir, buf, filler);
        if (success < 0) {
            return -1;
        }
    }

    // ----- Step through the first layer of indirection ----- //
    if (root.single_indirect.valid == true) {
        indirect* ind = (indirect*) bread(root.single_indirect.index);

        for (int i = 0; i < 128; i++) {
            blocknum b = ind->blocks[i];
            
            // ----- No more entries to read ----- //
            if (b.valid == false) {
                debug("\tDirect block %d was invalid.\n", i);
                return 0;
            }

            // ----- Read in all entries in the dirent ----- //
            dirent* dir = (dirent*) bread(b.index);
            int success = read_files_in_dir(dir, buf, filler);
            if (success < 0) {
                return -1;
            }
        }
    }

    // Step through the double-layer indirection if there are more blocks
    if (root.double_indirect.valid == true) {
        indirect* ind2 = (indirect*) bread(root.double_indirect.index);

        for (int i = 0; i < 128; i++) {
            blocknum b2 = ind2->blocks[i];

            // ----- No more entries to read ----- //
            if (b2.valid == false) {
                debug("Double indirect block %d was invalid.\n", i);
                return 0;
            }

            // ----- Go throught the single indirection ----- //
            indirect* ind1 = (indirect*) bread(b2.index);
            for (int j = 0; j < 128; j++) {
                blocknum b1 = ind1->blocks[j];
                // ----- No more entries to read ----- //
                if (b1.valid == false) {
                    debug("\tSingle indirect %d in double indirection block %d was invalid.\n", j, i);
                    return 0;
                }

                // ----- Read in all entries in the dirent ----- //
                dirent* dir = (dirent*) bread(b1.index);
                int success = read_files_in_dir(dir, buf, filler);
                if (success < 0) {
                    return -1;
                }
            }
        }
    }

    return 0;
}

int read_files_in_dir(dirent* dir, void *buf, fuse_fill_dir_t filler) {
    // ----- Read through the entries of the dirent block ----- //
    for (int j = 0; j < 8; j++) {
        direntry entry = dir->entries[j];

        // ----- Skip over empty entires ----- //
        if (entry.block.valid == false) {
            continue;
        }
        
        // ----- Load the entry name into the buffer ----- //
        char* name = entry.name;
        if (filler(buf, name, NULL, 0) != 0) {
            debug("ERROR: Problem using filler function on entry '%s'\n", name);
            return -1;
        }
        debug("Successfully read entry '%s'\n", name);
    }

    return 0;
}



/*
 * Given an absolute path to a file (for example /a/b/myFile), vfs_create 
 * will create a new file named myFile in the /a/b directory.
 *
 */
static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

    // ----- Read the root dnode direct blocks to find entry space ----- //
    debug("In file creation.\n");
    for (int i = 0; i < 54; i++) {
        debug("\tReading direct block %d\n", i);
        blocknum b = root.direct[i];
        debug("\tDirect block is located at index %d\n", b.index);

        // ----- This means we need to create a dirent ----- //
        if (b.valid == false) {
            blocknum dirent_block;
            dirent_block.index = global_vcb.free.index;
            dirent_block.valid = true;

            dirent dir;

            int success = create_dirent(&dir, path, mode, fi);
            if (success != 0) {
                // Error
                return success;
            }

            // ----- Rewrite the dnode to contain the new dirent ----- //
            
            root.direct[i] = dirent_block;

            char root_tmp[BLOCKSIZE];
            memcpy(root_tmp, &root, BLOCKSIZE);
            if (bwrite(1, &root) < 0) {
                debug("ERROR: Could not rewrite root dnode to disk.\n");
                return -1;
            }

            return 0;
        }

        // ----- Read in the block's data ----- //
        dirent* dir = (dirent*) bread(b.index);

        /*  Within the dirent, check each entry to see if the name matches
            If there is a match, we error. Otherwise, loop through to find
            an empty entry spot. If there is no empty spot (the loop finishes)
            then loop through to the next direct block to read in the next
            dirent block
         */
        for (int j = 0; j < 8; j++) {
            direntry entry = dir->entries[j];

            // ----- Found a free spot for a new file ----- //
            if (entry.block.valid == false) {
                int success = create_file(dir, b.index, j, path, mode, fi); 
                if (success != 0) {
                    debug("ERROR: Could not create file. Error code %d\n", success);
                }

                return success;
            }

            /*   If there's no free space for creating a new file, we check
                 to make sure that there isn't a file in this space with the
                 same name before moving on to the next entry space
             */
            char* name = entry.name;
            if (strcmp(name, path+1) == 0) {
                debug("ERROR: Tried to create the same file.");
                return -EEXIST;
            }
        }
    }

    // ----- Step through the first layer of indirection ----- //
    if (root.single_indirect.valid == true) {
        debug("\tReading in first layer of indirection.\n");
        indirect* ind = (indirect*) bread(root.single_indirect.index);

        for (int i = 0; i < 128; i++) {
            debug("\tReading indirect block %d\n", i);
            blocknum b = ind->blocks[i];
            
            // ----- This means we need to create a dirent ----- //
            if (b.valid == false) {
                debug("\tCreating new dirent block in indirection.\n");
                blocknum dirent_block;
                dirent_block.index = global_vcb.free.index;
                dirent_block.valid = true;

                dirent dir;

                int success = create_dirent(&dir, path, mode, fi);
                if (success != 0) {
                    // Error
                    return success;
                }

                // ----- Write the updated indirect block to point to the new dirent ----- //
                ind->blocks[i] = dirent_block;
                debug("\tWriting indirect block to disk at index %d poiting to dirent at block %d.\n", root.single_indirect.index, dirent_block.index);
                if (bwrite(root.single_indirect.index, ind) < 0) {
                    debug("ERROR: Could not write updated single indirect to disk.\n");
                    return -1;
                }

                return 0;
            }

            // ----- Read in the block's data ----- //
            dirent* dir = (dirent*) bread(b.index);

            /*  Within the dirent, check each entry to see if the name matches
                If there is a match, we error. Otherwise, loop through to find
                an empty entry spot. If there is no empty spot (the loop finishes)
                then loop through to the next direct block to read in the next
                dirent block
             */
            for (int j = 0; j < 8; j++) {
                direntry entry = dir->entries[j];

                // ----- Found a free spot for a new file ----- //
                if (entry.block.valid == false) {
                    int success = create_file(dir, b.index, j, path, mode, fi); 
                    if (success != 0) {
                        debug("ERROR: Could not create file. Error code %d\n", success);
                    }

                    return success;
                }

                /*   If there's no free space for creating a new file, we check
                     to make sure that there isn't a file in this space with the
                     same name before moving on to the next entry space
                 */
                char* name = entry.name;
                if (strcmp(name, path+1) == 0) {
                    debug("ERROR: Tried to create the same file.");
                    return -EEXIST;
                }
            }
        }
    } else {
        debug("Creating the first layer of indirection.\n");
        freeb* f = (freeb*) bread(global_vcb.free.index);
        indirect ind;
        blocknum invalid_block;
        invalid_block.index = -1;
        invalid_block.valid = 0;

        for (int i = 0; i < 128; i++) {
            ind.blocks[i] = invalid_block;
        }

        blocknum indirect_block;
        indirect_block.valid = 1;
        indirect_block.index = global_vcb.free.index;

        // ----- Update the VCB's free block ----- //
        debug("\tVCB's current free block index: %d\n", global_vcb.free.index);
        global_vcb.free = f->next;
        debug("\tVCB's new free block index: %d\n", global_vcb.free.index);

        // ----- Write the updated VCB to disk ----- //
        debug("\tWriting the updated VCB to disk...");
        if (bwrite(0, &global_vcb) < 0) {
            debug("\n\n\tError updating VCB on disk.\n\n");
            return -1;
        } else {
            debug("success.\n");
        }

        root.single_indirect = indirect_block;
        // Write the updated root to disk
        debug("\tWriting the updated root with a single indirect...");
        if (bwrite(1, &root) < 0) {
            debug("\n\n\tError updating root dir on disk.\n\n");
            return -1;
        } else {
            debug("success.\n");
        }

        // Create the dirent and file
        blocknum dirent_block;
        dirent_block.index = global_vcb.free.index;
        dirent_block.valid = true;

        dirent dir;

        int success = create_dirent(&dir, path, mode, fi);
        if (success != 0) {
            // Error
            return success;
        }

        // ----- Write the new indirect block to point to the new dirent ----- //
        ind.blocks[0] = dirent_block;

        if (bwrite(root.single_indirect.index, &ind) < 0) {
            debug("ERROR: Could not write updated single indirect to disk.\n");
            return -1;
        }

        return 0;
    }

    // Step through the double-layer indirection if there are more blocks
    // if (root.double_indirect.valid == true) {
    //     indirect* ind2 = (indirect*) bread(root.double_indirect.index);

    //     for (int i = 0; i < 128; i++) {
    //         blocknum b2 = ind2->blocks[i];

    //         // ----- No more entries to read ----- //
    //         if (b2.valid == false) {
    //             debug("Double indirect block %d was invalid.\n", i);
    //             return 0;
    //         }

    //         // ----- Go throught the single indirection ----- //
    //         indirect* ind1 = (indirect*) bread(b2.index);
    //         for (int j = 0; j < 128; j++) {
    //             blocknum b = ind1->blocks[j];

    //             // ----- This means we need to create a dirent ----- //
    //             if (b.valid == false) {
    //                 blocknum dirent_block;
    //                 dirent_block.index = global_vcb.free.index;
    //                 dirent_block.valid = true;

    //                 dirent dir;

    //                 int success = create_dirent(&dir, path, mode, fi);
    //                 if (success != 0) {
    //                     // Error
    //                     return success;
    //                 }

    //                 // ----- Rewrite the dnode to contain the new dirent ----- //
                    
    //                 root.direct[i] = dirent_block;

    //                 char root_tmp[BLOCKSIZE];
    //                 memcpy(root_tmp, &root, BLOCKSIZE);
    //                 if (bwrite(1, &root) < 0) {
    //                     debug("ERROR: Could not rewrite root dnode to disk.\n");
    //                     return -1;
    //                 }

    //                 return 0;
    //             }

    //             // ----- Read in the block's data ----- //
    //             dirent* dir = (dirent*) bread(b.index);

    //             /*  Within the dirent, check each entry to see if the name matches
    //                 If there is a match, we error. Otherwise, loop through to find
    //                 an empty entry spot. If there is no empty spot (the loop finishes)
    //                 then loop through to the next direct block to read in the next
    //                 dirent block
    //              */
    //             for (int j = 0; j < 8; j++) {
    //                 direntry entry = dir->entries[j];

    //                 // ----- Found a free spot for a new file ----- //
    //                 if (entry.block.valid == false) {
    //                     int success = create_file(dir, b.index, j, path, mode, fi); 
    //                     if (success != 0) {
    //                         debug("ERROR: Could not create file. Error code %d\n", success);
    //                     }

    //                     return success;
    //                 }

    //                 /*   If there's no free space for creating a new file, we check
    //                      to make sure that there isn't a file in this space with the
    //                      same name before moving on to the next entry space
    //                  */
    //                 char* name = entry.name;
    //                 if (strcmp(name, path+1) == 0) {
    //                     debug("ERROR: Tried to create the same file.");
    //                     return -EEXIST;
    //                 }
    //             }
    //         }
    //     }
    // }

    return 0;
}

int create_file(dirent* dir, int dirent_index, int entry_index, const char *path, mode_t mode, struct fuse_file_info *fi) {
    debug("Creating new file.\n");
    // Get rid of compiler warnings of unused parameters
    fi = fi + 0;
    mode = mode + 0;

    // ----- Get the index of the first free block ----- //
    int free_block_index = global_vcb.free.index;

    // ----- Read the free block into memory so we can update for the next one ----- //
    debug("\tReading in free block at %d\n", free_block_index);
    freeb* free_block = (freeb*) bread(free_block_index);

    // ----- Update the VCB's free block ----- //
    debug("\tVCB's current free block index: %d\n", global_vcb.free.index);
    global_vcb.free = free_block->next;
    debug("\tVCB's new free block index: %d\n", global_vcb.free.index);

    // ----- Write the updated VCB to disk ----- //
    debug("\tWriting the updated VCB to disk...");
    if (bwrite(0, &global_vcb) < 0) {
        debug("\n\n\tError updating VCB on disk.\n\n");
        return -1;
    } else {
        debug("success.\n");
    }

    // ----- Create a new entry for the file ----- //
    blocknum entry_blocknum;
    entry_blocknum.index = free_block_index;
    entry_blocknum.valid = true;

    direntry new_file_entry;
    strncpy(new_file_entry.name, path+1, 54);
    new_file_entry.name[54] = '\0'; // Terminate name with null byte
    new_file_entry.type = 'f';
    new_file_entry.block = entry_blocknum;
    new_file_entry.block.valid = true;
    debug("\tNew file name: %s\n", new_file_entry.name);

    // ----- Update the dirent with the new entry ----- //
    dir->entries[entry_index] = new_file_entry;

    // ----- Write the updated dirent to disk ----- //
    debug("\tWriting dirent to block %d\n", dirent_index);
    if (bwrite(dirent_index, dir) < 0) {
        debug("ERROR: Could not write updated dirent");
    }

    // ----- Set the file's stats ----- //
    inode new_file;
    struct timespec current_time;
    struct timeval ctval;
    if (gettimeofday(&ctval, NULL) != 0) {
        printf("Error getting time.\n");
    }
    current_time.tv_sec = ctval.tv_sec;
    current_time.tv_nsec = ctval.tv_usec * 1000;

    // if ((clock_gettime(CLOCK_REALTIME, &current_time)) != 0) {
    //     printf("ERROR: Cannot get time. \n");
    // }
    new_file.access_time = current_time;
    new_file.modify_time = current_time;
    new_file.create_time = current_time;
    new_file.size = sizeof(blocknum);
    new_file.user = getuid();
    new_file.group = getgid();
    new_file.mode = 0644;

    blocknum invalid_block;
    invalid_block.index = -1;
    invalid_block.valid = false;

    for (int k = 0; k < 54; k++) {
        new_file.direct[k] = invalid_block;
    }

    // ----- Write the new file stats to disk ----- //
    if (bwrite(free_block_index, &new_file) < 0) {
        debug("Error writing file to disk at block %d\n", free_block_index);
    }

    return 0;
}

int create_dirent(dirent *dir, const char *path, mode_t mode, struct fuse_file_info *fi) {
    debug("Creating a new dirent.\n");
    // ----- Get the index of the first free block ----- //
    int free_block_index = global_vcb.free.index;

    // ----- Make sure there is enough memory for creating files ----- //
    if (global_vcb.free.valid == true) {
        return -ENOMEM;
    }
    // ----- Read the free block into memory so we can update for the next one ----- //
    freeb* free_block = (freeb*) bread(free_block_index);

    // ----- Update the free block of the VCB ----- //
    debug("\tPrevious free block index: %d\n", global_vcb.free.index);
    global_vcb.free = free_block->next;
    debug("\tSet new free block on VCB at %d\n", global_vcb.free.index);
    debug("\tWriting updated VCB to disk...");
    // ----- Write the updated VCB to disk ----- //
    if (bwrite(0, &global_vcb) < 0) {
        debug("Error updating VCB on disk.\n");
        return -1;
    }
    debug("success.\n");

    // ----- Fill the new dirent with empty entries ----- //

    // Create an empty entry
    direntry empty_entry;

    // Create an invalid block for empty entry
    blocknum invalid_block;
    invalid_block.index = -1;
    invalid_block.valid = false;

    // Set the fields on the empty entry
    strncpy(empty_entry.name, "", 55);
    empty_entry.type = '\0';
    empty_entry.block = invalid_block;

    // Set the dirent to have all empty entries
    for (int j = 0; j < 8; j++) {
        dir->entries[j] = empty_entry;
    }

    // ----- Create a new file using the new dirent ----- //
    int success = create_file(dir, free_block_index, 0, path, mode, fi); 
    if (success != 0) {
        debug("ERROR: Could not create file. Error code %d\n", success);
        return success;
    }

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

    path = path + 0;
    buf = buf + 0;
    size = size + 0;
    offset = offset + 0;
    fi = fi + 0;
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

    path = path + 0;
    buf = buf + 0;
    size = size + 0;
    offset = offset + 0;
    fi = fi + 0;
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

    //----- Extract file name -----//
    // TODO 
    // When we support multiple level directories, fix this
    char* filename = (char*) malloc(sizeof(path));
    strncpy(filename, path + 1, sizeof(path));
    debug("Deleting file: '%s'\n", path);
    //----- Cycle through direct blocks to find the entry -----//
    for (int i = 0; i < 54; i++) {
        debug("\tChecking direct block %d for entry.\n", i);
        blocknum b = root.direct[i];
        if (b.valid == true) {
            debug("\tReading in dirent at block %d\n", b.index);
            dirent* d;
            d = (dirent*) bread(b.index);

            // ----- Read through the entries -----//
            for (int j = 0; j < 8; j++) {
                direntry* entry = &d->entries[j];
                if (entry->block.valid == true) {
                    debug("\tComparing %s with %s\n", entry->name, filename);
                    if (strcmp(entry->name, filename) == 0) {
                        // Delete!
                        debug("\tMatch! Setting the entry to be invalid.\n");
                        entry->block.valid = false;
                        debug("\tNew dirent:\n");
                        debug_dirent(d);
                        debug("\tWriting new dirent to disk.\n");
                        bwrite(b.index, d);
                        //TODO update free
                        free(filename);
                        return 0;
                    }
                }
            }
        } else {
            debug("\tFile not found.\n");
            free(filename);
            return -ENOENT;
        }
    }

    // TODO
    // Cycle through the single indirection and double indirection


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

    from = from + 0;
    to = to + 0;
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

    file = file + 0;
    mode = mode + 0;

    return 0;
}

/*
 * This function will change the user and group of the file
 * to be uid and gid.  This should only update the file's owner
 * and group.
 */
static int vfs_chown(const char *file, uid_t uid, gid_t gid)
{

    file = file + 0;
    uid = uid + 0;
    gid = gid + 0;
    return 0;
}

/*
 * This function will update the file's last accessed time to
 * be ts[0] and will update the file's last modified time to be ts[1].
 */
static int vfs_utimens(const char *file, const struct timespec ts[2])
{

    file = file + 0;
    ts = ts + 0;
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
    file = file + 0;
    offset = offset + 0;

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

