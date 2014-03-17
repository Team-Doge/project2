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
static int const DIRECT_SIZE = 54;
static int const INDIRECT_SIZE = 64;
vcb global_vcb;
dnode root;
dirent root_dir;
int read_files_in_dir(dirent *dir, void *buf, fuse_fill_dir_t filler);

int create_file(dirent *dir, blocknum dir_loc, int entry_index, const char *path, mode_t mode);
int create_file_in_dirent(dirent *dir, blocknum dir_loc, const char *path, mode_t mode);
int create_file_in_indirection(indirect *ind, blocknum ind_loc, const char *path, mode_t mode);

int create_dirent(dirent *dir, const char *path, mode_t mode);
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
        blocknum dirent_loc;
        dirent dir;
        int entry_index;
        int found = search_root_for_file(path, &dirent_loc, &dir, &entry_index);

        if (found >= 0) {
            debug("Getting attributes for file.\n");
            get_file_attr(dir.entries[entry_index], stbuf);
            return 0;
        }

        return -ENOENT;
    }

    return -1;
}

int search_root_for_file(const char *path, blocknum *dirent_loc, dirent *dir, int *entry_index) {
    bool found = false;
    found = find_file_entry(root.direct, DIRECT_SIZE, path, dirent_loc, dir, entry_index);
    if (found == true) {
        return 0;
    }

    // ----- Step through the first layer of indirection ----- //
    if (root.single_indirect.valid == true) {
        debug("Searching single indirect.\n");
        indirect* ind = (indirect*) bread(root.single_indirect.index);
        found = find_file_entry(ind->blocks, INDIRECT_SIZE, path, dirent_loc, dir, entry_index);
        free(ind);
        if (found == true) {
            return 1;
        }
    }

    // Step through the double-layer indirection if there are more blocks
    if (root.double_indirect.valid == true) {
        debug("Searching double indirect.\n");
        indirect* ind2 = (indirect*) bread(root.double_indirect.index);

        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b2 = ind2->blocks[i];
            if (b2.valid == false) {
                debug("File not found.\n");
                free(ind2);
                return -ENOENT;
            }

            debug("Searching %d/%d single indirect inside double.\n", i+1, INDIRECT_SIZE);
            indirect* ind1 = (indirect*) bread(b2.index);
            found = find_file_entry(ind1->blocks, INDIRECT_SIZE, path, dirent_loc, dir, entry_index);
            free(ind1);
            if (found == true) {
                free(ind2);
                return 2;
            }
        }
        free(ind2);
    }

    return -1;
}

bool find_file_entry(blocknum *blocks, int size, const char *path, blocknum *dirent_loc, dirent *dir, int *entry_index) {
    debug("Looking through %d blocks for '%s'.\n", size, path);
    for (int i = 0; i < size; i++) {
        blocknum b = blocks[i];
        debug("\tBlock %d/%d in list points to block %d.\n", i+1, size, b.index);
        if (b.valid == false) {
            debug("\tBlock %d is invalid.\n", b.index);
            return false;
        }

        dirent *d = (dirent *) bread(b.index);
        debug("\tLooking in dirent.\n")
        // debug_dirent(d);
        for (int j = 0; j < 8; j++) {
            direntry *e = &d->entries[j];
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
                        // debug("\tComparing '%s' to '%s'.\n", e->name, path+1);
                        if (strncmp(e->name, path+1, 55) == 0) {
                            *dirent_loc = b;
                            *dir = *d;
                            *entry_index = j;
                            free(d);
                            return true;
                        }
                        break;
                    default:
                        // Unknown entry type
                        break;
                }
            }
        }
        free(d);
    }
    
    return false;
}

int get_file_attr(direntry entry, struct stat *stbuf) {
    inode *file = (inode *) bread(entry.block.index);
    stbuf->st_mode = file->mode | S_IFREG;
    stbuf->st_uid = file->user;
    stbuf->st_gid = file->group;
    stbuf->st_atime = file->access_time.tv_sec;
    stbuf->st_mtime = file->modify_time.tv_sec;
    stbuf->st_ctime = file->create_time.tv_sec;
    stbuf->st_size = file->size;
    free(file);
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
    for (int i = 0; i < DIRECT_SIZE; i++) {
        blocknum b = root.direct[i];

        // ----- No more entries to read ----- //
        if (b.valid == false) {
            debug("\tDirect block %d was invalid.\n", i);
            return 0;
        }

        // ----- Read in all entries in the dirent ----- //
        dirent* dir = (dirent*) bread(b.index);
        int success = read_files_in_dir(dir, buf, filler);
        free(dir);
        if (success < 0) {
            return -1;
        }
    }

    // ----- Step through the first layer of indirection ----- //
    if (root.single_indirect.valid == true) {
        indirect* ind = (indirect*) bread(root.single_indirect.index);

        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b = ind->blocks[i];
            
            // ----- No more entries to read ----- //
            if (b.valid == false) {
                debug("\tDirect block %d was invalid.\n", i);
                free(ind);
                return 0;
            }

            // ----- Read in all entries in the dirent ----- //
            dirent* dir = (dirent*) bread(b.index);
            int success = read_files_in_dir(dir, buf, filler);
            free(dir);
            if (success < 0) {
                return -1;
            }
        }
        free(ind);
    }

    // Step through the double-layer indirection if there are more blocks
    if (root.double_indirect.valid == true) {
        indirect* ind2 = (indirect*) bread(root.double_indirect.index);

        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b2 = ind2->blocks[i];

            // ----- No more entries to read ----- //
            if (b2.valid == false) {
                debug("Double indirect block %d was invalid.\n", i);
                free(ind2);
                return 0;
            }

            // ----- Go throught the single indirection ----- //
            indirect* ind1 = (indirect*) bread(b2.index);
            for (int j = 0; j < INDIRECT_SIZE; j++) {
                blocknum b1 = ind1->blocks[j];
                // ----- No more entries to read ----- //
                if (b1.valid == false) {
                    debug("\tSingle indirect %d in double indirection block %d was invalid.\n", j, i);
                    free(ind1);
                    free(ind2);
                    return 0;
                }

                // ----- Read in all entries in the dirent ----- //
                dirent* dir = (dirent*) bread(b1.index);
                int success = read_files_in_dir(dir, buf, filler);
                if (success < 0) {
                    free(ind1);
                    free(ind2);
                    return -1;
                }
            }
            free(ind1);
        }
        free(ind2);
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
    fi = fi;
    // ----- Read the root dnode direct blocks to find entry space ----- //
    debug("In file creation.\n");
    for (int i = 0; i < DIRECT_SIZE; i++) {
        debug("\tReading direct block %d\n", i);
        blocknum b = root.direct[i];
        debug("\tDirect block is located at index %d\n", b.index);

        // ----- This means we need to create a dirent ----- //
        if (b.valid == false) {
            blocknum dirent_block;
            dirent_block.index = global_vcb.free.index;
            dirent_block.valid = true;

            dirent dir;

            int success = create_dirent(&dir, path, mode);
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
                int success = create_file(dir, b, j, path, mode); 
                if (success != 0) {
                    debug("ERROR: Could not create file. Error code %d\n", success);
                }
                free(dir);
                return success;
            }

            /*   If there's no free space for creating a new file, we check
                 to make sure that there isn't a file in this space with the
                 same name before moving on to the next entry space
             */
            char* name = entry.name;
            if (strcmp(name, path+1) == 0) {
                debug("ERROR: Tried to create the same file.");
                free(dir);
                return -EEXIST;
            }
        }
        free(dir);
    }

    // ----- Step through the first layer of indirection ----- //
    if (root.single_indirect.valid == true) {
        debug("\tReading in first layer of indirection.\n");
        indirect* ind = (indirect*) bread(root.single_indirect.index);
        int created = create_file_in_indirection(ind, root.single_indirect, path, mode);
        free(ind);
        if (created >= 0) {
            // Success
            return 0;
        } else if (created == -EEXIST) {
            return -EEXIST;
        }
        // Fall through and try and create in the double
    } else {
        debug("Creating the first layer of indirection.\n");
        freeb* f = (freeb*) bread(global_vcb.free.index);
        indirect ind;
        blocknum invalid_block;
        invalid_block.index = -1;
        invalid_block.valid = false;

        for (int i = 0; i < INDIRECT_SIZE; i++) {
            ind.blocks[i] = invalid_block;
        }

        blocknum ind_loc;
        ind_loc.valid = true;
        ind_loc.index = global_vcb.free.index;
        global_vcb.free = f->next;

            // ----- Write the updated VCB to disk ----- //
        if (bwrite(0, &global_vcb) < 0) {
            debug("\n\n\tError updating VCB on disk.\n\n");
        }
        free(f);
        int created = create_file_in_indirection(&ind, ind_loc, path, mode);
        if (created >= 0) {
            root.single_indirect = ind_loc;
            debug("Single indirect block on root.\n");
            debug_block(&ind_loc);
            // Write the updated root to disk
            if (bwrite(1, &root) < 0) {
                error("Error writing updated root to disk.\n");
            }
            return 0;
        } else if (created == -EEXIST) {
            return -EEXIST;
        }
        // Something went really wrong, but try and fall through to double
    }

    // Step through the double-layer indirection if there are more blocks
    if (root.double_indirect.valid == true) {
        debug("Reading in second layer of indirection.\n");
        indirect *double_ind = (indirect *) bread(root.double_indirect.index);
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b = double_ind->blocks[i];
            if (b.valid == true) {
                // Try and create the file in this single indirect block
                indirect *single = (indirect *) bread(b.index);
                int created = create_file_in_indirection(single, b, path, mode);
                free(single);
                if (created >= 0) {
                    // Success
                    free(double_ind);
                    return 0;
                } else if (created == -EEXIST) {
                    free(double_ind);
                    return -EEXIST;
                }
                // Fall through to next single indirect
            } else {
                // We need to create a new single indirect
                indirect single;
                blocknum invalid;
                invalid.index = -1;
                invalid.valid = false;
                for (int j = 0; j < INDIRECT_SIZE; j++) {
                    single.blocks[j] = invalid;
                }

                blocknum single_loc;
                single_loc.valid = true;
                single_loc.index = global_vcb.free.index;

                freeb *f = (freeb *) bread(global_vcb.free.index);
                global_vcb.free = f->next;
                if (bwrite(0, &global_vcb) < 0) {
                    error("Error writing updated VCB to disk.\n");
                }
                free(f);
                int created = create_file_in_indirection(&single, single_loc, path, mode);
                if (created >= 0) {
                    // Write the update double to disk
                    double_ind->blocks[i] = single_loc; 
                    if (bwrite(root.double_indirect.index, double_ind) < 0) {
                        error("Error writing updated double indirect to disk.\n");
                    }
                }
                free(double_ind);
                return created;
            }
        }
        free(double_ind);
        // Fell through the loop, which means there are no more double indirect
        // blocks. We're out of space.
    } else {
        // Create the indirection
        blocknum invalid_block;
        invalid_block.index = -1;
        invalid_block.valid = false;

        debug("Creating the second layer of indirection.\n");
        indirect double_ind;
        blocknum double_loc;
        double_loc.index = global_vcb.free.index;
        double_loc.valid = true;
        freeb* double_free = (freeb*) bread(global_vcb.free.index);
        global_vcb.free = double_free->next;

        // Initialize with empty blocks
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            double_ind.blocks[i] = invalid_block;
        }

        debug("Creating the first single indirection block for double indirection.\n");
        indirect single_ind;
        blocknum single_loc;
        single_loc.index = global_vcb.free.index;
        single_loc.valid = true;
        freeb* single_free = (freeb *) bread(global_vcb.free.index);
        global_vcb.free = single_free->next;

        // Initialize with empty blocks
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            single_ind.blocks[i] = invalid_block;
        }

        // Set the first single indirect in double
        double_ind.blocks[0] = single_loc;

        // Write the updated VCB to disk
        if (bwrite(0, &global_vcb) < 0) {
            error("Error updating the free block on the VCB on disk.\n");
        }
        free(double_free);
        free(single_free);

        int created = create_file_in_indirection(&single_ind, single_loc, path, mode);
        if (created >= 0) {
            // Success
            // Write the doduble indirect to disk
            if (bwrite(double_loc.index, &double_ind) < 0) {
                error("Error writing double indirect block to disk.\n");
            }

            // Write the single indirect to disk
            if (bwrite(single_loc.index, &single_ind) < 0) {
                error("Error writing single indirect block to disk.\n");
            }

            root.double_indirect = double_loc;
            // Write the updated root to disk
            if (bwrite(1, &root) < 0) {
                error("Error writing updated root to disk.\n");
            }

            return 0;
        } else {
            error("Error creating file in double indirect.\n");
            return created;
        }

    }
    return -ENOSPC;
}

int create_file_in_indirection(indirect *ind, blocknum indirect_loc, const char *path, mode_t mode) {

    for (int i = 0; i < INDIRECT_SIZE; i++) {
        blocknum b = ind->blocks[i];
        if (b.valid == true) {
            // See if there's room
            dirent *dir = (dirent *) bread(b.index);

            // Only creates if there is enough space in the dirent
            int file_created = create_file_in_dirent(dir, b, path, mode);
            free(dir);
            if (file_created >= 0) {
                return file_created;

            } else if (file_created == -EEXIST) {
                return -EEXIST;
            }
            // Otherwise fall through to the next dirent

        } else {
            // Pull off the free block for it
            blocknum dir_loc;
            dir_loc.index = global_vcb.free.index;
            dir_loc.valid = true;
            freeb *f = (freeb *) bread(global_vcb.free.index);
            global_vcb.free = f->next;

            // Write the updated VCB to disk
            if (bwrite(0, &global_vcb) < 0) {
                error("Error updating the free block on the VCB on disk.\n");
            }
            free(f);

            // Make a new dirent
            dirent dir;

            // Empty entry
            direntry empty_entry;
            strncpy(empty_entry.name, "", 55);
            empty_entry.type = ' ';
            empty_entry.block.valid = false;
            empty_entry.block.index = -1;

            // Fill the new direnbt with empty entries
            for (int i = 0; i < 8; i++) {
                dir.entries[i] = empty_entry;
            }
            int file_created = create_file_in_dirent(&dir, dir_loc, path, mode);
            ind->blocks[i] = dir_loc;
            // Write the updated indirect block to disk
            if (bwrite(indirect_loc.index, ind) < 0) {
                error("Error updating indirect block.\n");
            }


            debug("Created new dirent with file in indirection.\n");
            debug_block(&dir_loc);
            debug_dirent(&dir);
            return file_created;
        }
    }

    return -1;
}

int create_file_in_dirent(dirent *dir, blocknum dir_loc, const char *path, mode_t mode) {
    
    for (int i = 0; i < 8; i++) {
        direntry entry = dir->entries[i];
        if (entry.block.valid == false) {
            // Create file
            int created = create_file(dir, dir_loc, i, path, mode);
            if (created) {
                return 0;
            } else {
                error("Error creating file.\n");
                return created;
            }
        } else {
            // Something exists here, make sure we're not creating the same file
            if (strncmp(path, entry.name, 55) == 0) {
                return -EEXIST;
            }
        }
    }

    return -1;
}

int create_file(dirent *dir, blocknum dir_loc, int entry_index, const char *path, mode_t mode) {
    debug("Creating file in dirent:\n");
    debug_dirent(dir);
    // Time is annoying
    struct timespec current_time;
    struct timeval ctval;
    if (gettimeofday(&ctval, NULL) != 0) {
        printf("Error getting time.\n");
    }
    current_time.tv_sec = ctval.tv_sec;
    current_time.tv_nsec = ctval.tv_usec * 1000;

    // Set dumb data for new files
    blocknum invalid_block;
    invalid_block.index = -1;
    invalid_block.valid = false;

    // Create the inode
    debug("Creating file inode.\n");
    inode file;
    file.access_time = current_time;
    file.modify_time = current_time;
    file.create_time = current_time;
    file.size = 0;
    file.user = getuid();
    file.group = getgid();
    file.mode = mode;
    file.single_indirect = invalid_block;
    file.double_indirect = invalid_block;
    for (int i = 0; i < DIRECT_SIZE; i++) {
        file.direct[i] = invalid_block;
    }

    // Setup the file location
    blocknum file_loc;
    file_loc.index = global_vcb.free.index;
    file_loc.valid = true;

    // Pop off a free block
    freeb *f = (freeb *) bread(global_vcb.free.index);
    global_vcb.free = f->next;
    if (bwrite(0, &global_vcb) < 0) {
        error("Error updating VCB free block when creating file.\n");
        free(f);
        return -1;
    }
    free(f);
    debug("Updated VCB free block to %d.\n", global_vcb.free.index);

    // Write the file to disk
    if (bwrite(file_loc.index, &file) < 0) {
        error("Error writing file to disk.\n");
        free(f);
        return -1;
    }
    debug("Wrote file inode to %d.\n", file_loc.index);

    // Make the entry
    direntry entry;
    strncpy(entry.name, path+1, 55);
    // Make sure the name ends with a null byte
    entry.name[54] = '\0';
    entry.type = 'f';
    entry.block = file_loc;

    // Add the entry to the dirent
    dir->entries[entry_index] = entry;

    // Write the updated dir to disk
    if (bwrite(dir_loc.index, dir) < 0) {
        error("Could not update dirent on disk.\n");
    }
    // Successfully created file
    return 0;

}

int create_dirent(dirent *dir, const char *path, mode_t mode) {
    debug("Creating a new dirent.\n");
    // ----- Get the index of the first free block ----- //
    blocknum dir_loc;
    int free_block_index = global_vcb.free.index;
    dir_loc.index = free_block_index;
    dir_loc.valid = 1;

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
        free(free_block);
        return -1;
    }
    debug("success.\n");
    free(free_block);


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
    int success = create_file(dir, dir_loc, 0, path, mode); 
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
    fi = fi;

    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(path, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Reading data from file '%s'.\n", path);
        blocknum file_loc = dir.entries[entry_index].block;
        inode *file = (inode *) bread(file_loc.index);
        int read = read_from_file(file, buf, size, offset);
        free(file);
        return read;
    }

    return -ENOENT;
}

int read_from_file(inode *file, char *buf, size_t size, off_t offset) {
    int total_bytes_read = 0;
    debug("\tBytes to read: %d.\n", (int) size);
    // Go through the direct blocks
    int direct_read = read_from_block_list(file->direct, DIRECT_SIZE, &offset, buf, total_bytes_read, size);
    total_bytes_read += direct_read;
    if ((unsigned) total_bytes_read == size) {
        return total_bytes_read;
    }

    // Single indirection
    if (file->single_indirect.valid == true) {
        indirect *single = (indirect *) bread(file->single_indirect.index);
        int single_read = read_from_block_list(single->blocks, INDIRECT_SIZE, &offset, buf, total_bytes_read, size);
        free(single);
        total_bytes_read += single_read;
        if ((unsigned) total_bytes_read == size) {
            return total_bytes_read;
        }
    }

    // Double indirection
    if (file->double_indirect.valid == true) {
        indirect *double_ind = (indirect *) bread(file->double_indirect.index);
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b = double_ind->blocks[i];
            if (b.valid == true) {
                // Read in the single in the double
                indirect *single_ind = (indirect *) bread(b.index);
                int single_read = read_from_block_list(single_ind->blocks, INDIRECT_SIZE, &offset, buf, total_bytes_read, size);
                free(single_ind);
                total_bytes_read += single_read;
                if ((unsigned) total_bytes_read == size) {
                    free(double_ind);
                    return total_bytes_read;
                }
            }
        }
        free(double_ind);
    }

    return total_bytes_read;
}

int read_from_block_list(blocknum *blocks, int list_size, off_t *offset, char *buf, int buf_pos, size_t size) {
    int total_bytes_read = 0;
    for (int i = 0; i < list_size; i++) {
        // debug("Reading data block %d/%d.\n", i+1, list_size);
        blocknum b = blocks[i];
        if (b.valid == false) {
            // Nothing more to read
            return total_bytes_read;
        }
        int read = read_from_block(b, offset, buf, buf_pos + total_bytes_read, size);
        total_bytes_read += read;
        if ((unsigned) (total_bytes_read + buf_pos) == size) {
            return total_bytes_read;
        }
    }

    return total_bytes_read;
}

int read_from_block(blocknum block, off_t *offset, char *buf, int buf_pos, size_t size) {
    int total_bytes_read = 0;
    db *data = (db *) bread(block.index);
    for (int i = 0; i < BLOCKSIZE; i++) {
        if (*offset > 0) {
            *offset = *offset - 1;
            continue;
        }
        int position = i + buf_pos;
        buf[position] = data->data[i];
        total_bytes_read++;
        if ((unsigned) (total_bytes_read + buf_pos) == size) {
            free(data);
            return total_bytes_read;
        }
    }
    free(data);
    return total_bytes_read;
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

    fi = fi;

    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(path, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Writng to '%s'.\n", path);
        blocknum file_loc = dir.entries[entry_index].block;
        inode *file = (inode *) bread(file_loc.index);
        int total = write_to_file(file, buf, size, offset);
        free(file);
        // Update file size
        if (offset + total > file->size) {
            file->size = file->size + ((offset + total) - file->size);
        }

        // Update on disk
        if (bwrite(file_loc.index, file) < 0) {
            // Error writing to disk
            error("Error writing data for file to disk.\n");
            return -1;
        }

        return total;
    }

    return 0;
}

int write_to_file(inode *file, const char *buf, size_t size, off_t offset) {
    int total_bytes_written = 0;
    debug("\tBytes to write: %d.\n", (int) size);
    // Go through the direct blocks
    int direct_written = write_data_to_block_list(file->direct, DIRECT_SIZE, size, &offset, buf, total_bytes_written);
    total_bytes_written += direct_written;
    if (direct_written < 0) {
        // We ran out of space most likely
        return direct_written;
    } else if ((unsigned) total_bytes_written == size) {
        // We finished writing
        debug("Successfully finished writing.\n");
        return total_bytes_written;
    }

    // Invalid block for creating new indirects
    blocknum invalid;
    invalid.index = -1;
    invalid.valid = false;

    // Single indirect
    int single_written = 0;
    if (file->single_indirect.valid == true) {
        indirect *single = (indirect *) bread(file->single_indirect.index);
        int written = write_data_to_block_list(single->blocks, INDIRECT_SIZE, size, &offset, buf, total_bytes_written);
        single_written += written;
        free(single);
    } else {
        // Create the first indirect
        blocknum single_loc;
        single_loc.index = global_vcb.free.index;
        single_loc.valid = true;
        freeb *f = (freeb *) bread(global_vcb.free.index);
        global_vcb.free = f->next;
        if (bwrite(0, &global_vcb) < 0) {
            error("Error writing updated VCB to disk.\n");
        }

        indirect single;
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            single.blocks[i] = invalid;
        }
        int written = write_data_to_block_list(single.blocks, INDIRECT_SIZE, size, &offset, buf, total_bytes_written);
        single_written += written;
        file->single_indirect = single_loc;
        if (bwrite(single_loc.index, &single) < 0) {
            error("Error creating new indirect on disk.\n");
        }
        free(f);
    }
    if (single_written < 0) {
        // We ran out of space most likely
        return single_written;
    } else if ((unsigned) (total_bytes_written + single_written) == size) {
        // We finished writing
        return total_bytes_written + single_written;
    }
    total_bytes_written += single_written;

    // Double indirect
    int double_written = 0;
    if (file->double_indirect.valid == true) {
        indirect *double_ind = (indirect *) bread(file->double_indirect.index);
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b = double_ind->blocks[i];
            if (b.valid == true) {
                indirect *single_ind = (indirect *) bread(b.index);
                int written = write_data_to_block_list(single_ind->blocks, INDIRECT_SIZE, size, &offset, buf, total_bytes_written + double_written);
                free(single_ind);
                if (written < 0) {
                    // Ran out of space or error
                    free(double_ind);
                    return written;
                }
                double_written += written;
            } else {
                // Create a single indirect
                blocknum single_loc;
                single_loc.index = global_vcb.free.index;
                single_loc.valid = true;
                freeb *f = (freeb *) bread(global_vcb.free.index);
                global_vcb.free = f->next;
                if (bwrite(0, &global_vcb) < 0) {
                    error("Error writing updated VCB to disk.\n");
                }

                blocknum invalid;
                invalid.index = -1;
                invalid.valid = false;
                indirect single;
                for (int i = 0; i < INDIRECT_SIZE; i++) {
                    single.blocks[i] = invalid;
                }
                int written = write_data_to_block_list(single.blocks, INDIRECT_SIZE, size, &offset, buf, total_bytes_written + double_written);
                free(f);
                double_ind->blocks[i] = single_loc;
                if (bwrite(single_loc.index, &single) < 0) {
                    error("Error creating new indirect on disk.\n");
                }
                if (written < 0) {
                    // Ran out of space or error
                    // Save the double to disk first because of the new single
                    if (bwrite(file->double_indirect.index, double_ind) < 0) {
                        error("Error updating double indirect on disk.\n");
                    }
                    free(double_ind);
                    return written;
                } else if ((unsigned) (total_bytes_written + double_written + written) == size) {
                    // We're done writing
                    // Save the double to disk because of the new single
                    if (bwrite(file->double_indirect.index, double_ind) < 0) {
                        error("Error updating double indirect on disk.\n");
                    }
                    free(double_ind);
                    return total_bytes_written + double_written + written;
                }
                double_written += written;

            }
        }
        free(double_ind);
    } else {
        // Create the second indirect location
        blocknum double_loc;
        double_loc.index = global_vcb.free.index;
        double_loc.valid = true;
        freeb *double_free = (freeb *) bread(global_vcb.free.index);
        global_vcb.free = double_free->next;
        if (bwrite(0, &global_vcb) < 0) {
            error("Error writing updated VCB to disk.\n");
        }
        free(double_free);
        file->double_indirect = double_loc;

        // Create the second indirect block with invalid blocks
        indirect double_ind;
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            double_ind.blocks[i] = invalid;
        }

        // Writing data to the double indirect
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            debug("Writing to single indirect %d/%d in double indirect.\n", i+1, INDIRECT_SIZE);
            // Create a single indirect
            blocknum single_loc;
            single_loc.index = global_vcb.free.index;
            single_loc.valid = true;
            freeb *f = (freeb *) bread(global_vcb.free.index);
            global_vcb.free = f->next;
            if (bwrite(0, &global_vcb) < 0) {
                error("Error writing updated VCB to disk.\n");
            }
            free(f);
            blocknum invalid;
            invalid.index = -1;
            invalid.valid = false;
            indirect single;
            for (int i = 0; i < INDIRECT_SIZE; i++) {
                single.blocks[i] = invalid;
            }
            int written = write_data_to_block_list(single.blocks, INDIRECT_SIZE, size, &offset, buf, total_bytes_written + double_written);
            double_ind.blocks[i] = single_loc;
            if (bwrite(single_loc.index, &single) < 0) {
                error("Error creating new indirect on disk.\n");
            }
            if (written < 0) {
                // Ran out of space or error
                // Save the double to disk first because of the new single
                if (bwrite(file->double_indirect.index, &double_ind) < 0) {
                    error("Error updating double indirect on disk.\n");
                }
                return written;
            } else if ((unsigned) (total_bytes_written + double_written + written) == size) {
                // We're done writing
                // Save the double to disk first because of the new single
                if (bwrite(file->double_indirect.index, &double_ind) < 0) {
                    error("Error updating double indirect on disk.\n");
                }
                return total_bytes_written + double_written + written;
            }
            double_written += written;
        }
    }

    return total_bytes_written + double_written;
}

int write_data_to_block_list(blocknum *blocks, int list_size, int size, off_t *offset, const char *buf, int buf_pos) {
    int total_bytes_written = 0;
    debug("Writing to a potential %d blocks for file.\n", list_size);
    for (int i = 0; i < list_size; i++) {
        blocknum b = blocks[i];
        if (b.valid == true) {
            // Write data to existing block
            db *data = (db *) bread(b.index);
            size_t data_size = strnlen(data->data, BLOCKSIZE);
            if (data_size < (unsigned) BLOCKSIZE) {
                debug("\tWriting to existing block %d/%d.\n", i+1, list_size);
                int written = write_data_to_block(data, offset, size, buf, total_bytes_written + buf_pos);
                total_bytes_written += written;

                // Update the file on disk
                if (bwrite(b.index, data) < 0) {
                    error("Error updating data on disk.\n");
                }

                // Check to see if we're done writing
                if (total_bytes_written == size) {
                    free(data);
                    return total_bytes_written;
                }
            } else {
                *offset = *offset - BLOCKSIZE;
            } 
            free(data);
        } else {
            debug("Making new data block for file at %d/%d.\n", i+1, list_size);
            // Make sure there's space for more data
            if (global_vcb.free.valid == true) {
                return -ENOSPC;
            }

            // Write data to a new block
            blocknum data_loc;
            data_loc.valid = true;
            data_loc.index = global_vcb.free.index;
            freeb *f = (freeb *) bread(global_vcb.free.index);
            global_vcb.free = f->next;
            if (bwrite(0, &global_vcb) < 0) {
                error("Error updating VCB on disk.\n");
            }
            free(f);

            db data;
            for (int i = 0; i < BLOCKSIZE; i++) {
                data.data[i] = '\0';
            }
            int written = write_data_to_block(&data, offset, size, buf, total_bytes_written + buf_pos);
            total_bytes_written += written;
            if (written > 0) {
                // debug_data(&data);
            }
            printf("\n");
            if (bwrite(data_loc.index, &data) < 0) {
                error("Error writing new data to disk.\n");
            }
            blocks[i] = data_loc;
            if (total_bytes_written == size) {
                return total_bytes_written;
            }
        }
    }

    return total_bytes_written;
}

int write_data_to_block(db *data, off_t *offset, int size, const char *buf, int buf_pos) {
    int written = 0;
    for (int i = 0; i < BLOCKSIZE && (written + buf_pos) < size; i++) {
        if (*offset > 0) {
            if (data->data[i] == '\0') {
                data->data[i] = '\0';
            }
            *offset = *offset - 1;
            continue;
        }

        data->data[i] = buf[written+buf_pos];
        written++;
    }
    debug("\tWrote %d bytes.\n", written);
    return written;

}

/**
 * This function deletes the last component of the path (e.g., /a/b/c you 
 * need to remove the file 'c' from the directory /a/b).
 */
static int vfs_delete(const char *path)
{

    /* 3600: NOTE THAT THE BLOCKS CORRESPONDING TO THE FILE SHOULD BE MARKED
       AS FREE, AND YOU SHOULD MAKE THEM AVAILABLE TO BE USED WITH OTHER FILES */


    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(path, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        dir.entries[entry_index].block.valid = false;
        if (bwrite(dirent_loc.index, &dir) < 0) {
            // Error writing to disk
            error("Error writing deleted file in dirent to disk.\n");
            return -1;
        }

        // Reclaim blocks as free
        inode *file = (inode *) bread(dir.entries[entry_index].block.index);
        // Reclaim direct blocks
        add_block_list_to_free_list(file->direct, DIRECT_SIZE);
        if (file->single_indirect.valid) {
            indirect *single = (indirect *) bread(file->single_indirect.index);
            add_block_list_to_free_list(single->blocks, INDIRECT_SIZE);   
            free(single);         
        }
        if (file->double_indirect.valid) {
            indirect *double_ind = (indirect *) bread(file->double_indirect.index);
            for (int i = 0; i < INDIRECT_SIZE; i++) {
                blocknum b = double_ind->blocks[i];
                if (b.valid == true) {
                    indirect *single_ind = (indirect *) bread(b.index);
                    add_block_list_to_free_list(single_ind->blocks, INDIRECT_SIZE);
                    free(single_ind);
                }
            }
            free(double_ind);
        }

        add_block_to_free_list(dir.entries[entry_index].block);
        free(file);
        return 0;
    }

    return -ENOENT;
}

void add_block_list_to_free_list(blocknum *blocks, int size) {
    for (int i = 0; i < size; i++) {
        blocknum b = blocks[i];
        if (b.valid == true) {
            add_block_to_free_list(b);
        }
    }
}

void add_block_to_free_list(blocknum block) {
    freeb f;
    f.next = global_vcb.free;
    block.valid = false;
    global_vcb.free = block;
    if (bwrite(0, &global_vcb) < 0) {
        error("Error updating VCB on disk.\n");
    }
    if (bwrite(block.index, &f) < 0) {
        error("Error updating free block on disk.\n");
    }
    debug("Successfully reclaimed block %d.\n", block.index);
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

    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(from, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Renaming '%s' to '%s'.\n", from, to+1);
        // TODO
        // If we support multiple directories, this won't work
        strncpy(dir.entries[entry_index].name, to+1, 55);
        // Always make sure the file name ends with a null byte
        dir.entries[entry_index].name[54] = '\0';
        if (bwrite(dirent_loc.index, &dir) < 0) {
            // Error writing to disk
            error("Error writing renamed file in dirent to disk.\n");
            return -1;
        }

        return 0;
    }

    return -1;
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

    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(file, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Updating permissions for '%s'.\n", file);
        blocknum file_loc = dir.entries[entry_index].block;
        inode *file = (inode *) bread(file_loc.index);
        file->mode = (mode & 0x0000ffff);

        if (bwrite(file_loc.index, file) < 0) {
            // Error writing to disk
            error("Error writing modified permissions for file to disk.\n");
            free(file);
            return -1;
        }
        free(file);
        return 0;
    }

    return -1;
}

/*
 * This function will change the user and group of the file
 * to be uid and gid.  This should only update the file's owner
 * and group.
 */
static int vfs_chown(const char *file, uid_t uid, gid_t gid)
{

    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(file, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Updating owners for '%s'.\n", file);
        blocknum file_loc = dir.entries[entry_index].block;
        inode *file = (inode *) bread(file_loc.index);
        file->user = uid;
        file->group = gid;

        if (bwrite(file_loc.index, file) < 0) {
            // Error writing to disk
            error("Error writing modified owners for file to disk.\n");
            free(file);
            return -1;
        }
        free(file);
        return 0;
    }

    return -1;
}

/*
 * This function will update the file's last accessed time to
 * be ts[0] and will update the file's last modified time to be ts[1].
 */
static int vfs_utimens(const char *file, const struct timespec ts[2])
{


    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(file, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Updating time for '%s'.\n", file);
        blocknum file_loc = dir.entries[entry_index].block;
        inode *file = (inode *) bread(file_loc.index);
        file->access_time = ts[0];
        file->modify_time = ts[1];

        if (bwrite(file_loc.index, file) < 0) {
            // Error writing to disk
            error("Error writing modified permissions for file to disk.\n");
            free(file);
            return -1;
        }
        free(file);
        return 0;
    }

    return -1;
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

    blocknum dirent_loc;
    dirent dir;
    int entry_index;
    int found = search_root_for_file(file, &dirent_loc, &dir, &entry_index);

    if (found >= 0) {
        debug("Updating permissions for '%s'.\n", file);
        blocknum file_loc = dir.entries[entry_index].block;
        inode *f = (inode *) bread(file_loc.index);
        truncate_file(f, offset);
        f->size = offset;
        if (bwrite(file_loc.index, f) < 0) {
            // Error writing to disk
            error("Error writing modified permissions for file to disk.\n");
            free(f);
            return -1;
        }
        free(f);
        return 0;
    }


    return 0;
}

void truncate_file(inode *file, off_t offset) {
    // Cut in the direct blocks?
    for (int i = 0; i < DIRECT_SIZE; i++) {
        blocknum b = file->direct[i];
        if (b.valid == true) {
            if (offset <= 0) {
                // Delete the block
                file->direct[i].valid = false;
                add_block_to_free_list(b);
            } else {
                offset = offset - BLOCKSIZE;                
            }
        } else {
            // Invalid blocks, no more data
            return;
        }

    }

    // Single indirection
    if (file->single_indirect.valid == true) {
        indirect *single = (indirect *) bread(file->single_indirect.index);
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b = single->blocks[i];
            if (b.valid == true) {
                if (offset <= 0) {
                    // Delete the block
                    single->blocks[i].valid = false;
                    add_block_to_free_list(b);
                }  else {
                    offset = offset - BLOCKSIZE;
                }             
            } else {
                // Invalid blocks, no more data
                return;
            }
        }
        // Write the updated single indirect to disk
        if (bwrite(file->single_indirect.index, single) < 0) {
            error("Error deleting blocks in single indirect.\n");
        }
        free(single);
    }

    // Double indirection
    if (file->double_indirect.valid == true) {
        indirect *double_ind = (indirect *) bread(file->double_indirect.index);
        for (int i = 0; i < INDIRECT_SIZE; i++) {
            blocknum b = double_ind->blocks[i];
            if (b.valid == true) {
                indirect *single_ind = (indirect *) bread(b.index);
                for (int j = 0; j < INDIRECT_SIZE; j++) {
                    blocknum s = single_ind->blocks[j];
                    if (s.valid == true) {
                        if (offset <= 0) {
                            // Delete the block
                            single_ind->blocks[j].valid = false;
                        } else {
                            offset = offset - BLOCKSIZE;
                        }
                    } else {
                        // Invalid blocks, no more data
                        break;
                    }
                }
                if (bwrite(b.index, single_ind) < 0) {
                    error("Error deleting single indirect in double indirect.\n");
                }
                free(single_ind);
            }
        }
        free(double_ind);
    }
}


/*
 * You shouldn't mess with this; it sets up FUSE
 *
 * NOTE: If you're supporting multiple directories for extra credit,
 * you should add 
 *
 *     .mkdir    = vfs_mkdir,
 */
static struct fuse_operations vfs_oper = {
    .init    = vfs_mount,
    .destroy = vfs_unmount,
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .create  = vfs_create,
    .read    = vfs_read,
    .write   = vfs_write,
    .unlink  = vfs_delete,
    .rename  = vfs_rename,
    .chmod   = vfs_chmod,
    .chown   = vfs_chown,
    .utimens     = vfs_utimens,
    .truncate    = vfs_truncate,
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

