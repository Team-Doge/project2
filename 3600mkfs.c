/*
 * CS3600, Spring 2014
 * Project 2 Starter Code
 * (c) 2013 Alan Mislove
 *
 * This program is intended to format your disk file, and should be executed
 * BEFORE any attempt is made to mount your file system.  It will not, however
 * be called before every mount (you will call it manually when you format 
 * your disk file).
 */

#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "3600fs.h"

void myformat(int size) {
    // Do not touch or move this function
    dcreate_connect();

    /* 3600: FILL IN CODE HERE.  YOU SHOULD INITIALIZE ANY ON-DISK
       STRUCTURES TO THEIR INITIAL VALUE, AS YOU ARE FORMATTING
       A BLANK DISK.  YOUR DISK SHOULD BE size BLOCKS IN SIZE. */

    blocknum *blocks = calloc(size-1, BLOCKSIZE);

    for (int i = 1; i < size; i++) {
        blocknum b;
        b.index = i;
        b.valid = 0;
        blocks[i] = b;
    }

    freeb *free_blocks = calloc(size - 3, BLOCKSIZE);
    for (int i = 0; i < size - 3; i++) {
        freeb f;
        blocknum b = blocks[i+4];

        if ((i + 4) == size) {
            b.valid = 1;
            b.index = -1;
        } else {
            b.valid = 0;
        }

        f.next = b;
        free_blocks[i] = f;
        // printf("Free block created with next pointing to %d\n", b.index);
    }

    blocknum invalid_block;
    invalid_block.index = -1;
    invalid_block.valid = 0;

    vcb myvcb;
    myvcb.blocksize = BLOCKSIZE;
    myvcb.magic = 17;
    myvcb.root = blocks[1];
    myvcb.root.valid = 1;
    myvcb.free = blocks[3];
    strcpy(myvcb.name, "Josh and Mich's excellent file system.");

    dnode root;
    root.direct[0] = blocks[2];
    root.direct[0].valid = 1;
    root.size = sizeof(blocknum);
    root.user = getuid();
    root.group = getgid();
    root.mode = 0777;
    root.single_indirect = invalid_block;
    root.double_indirect = invalid_block;
 
    for (int i = 1; i < 54; i++) {
        root.direct[i] = invalid_block;
    }
    
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
    root.access_time = current_time;
    root.modify_time = current_time;
    root.create_time = current_time;

    direntry dot;
    dot.type = 'd';
    dot.block = blocks[1];
    dot.block.valid = 1;
    strncpy(dot.name, ".", 55);

    direntry dotdot;
    dotdot.type = 'd';
    dotdot.block = blocks[1];
    dotdot.block.valid = 1;
    strncpy(dotdot.name, "..", 55);

    dirent root_dir;
    root_dir.entries[0] = dot;
    root_dir.entries[1] = dotdot;

    direntry empty;
    empty.type = '\0';
    empty.block = blocks[size];
    strcpy(empty.name, "");

    for (int i = 2; i < 8; i++) {
        root_dir.entries[i] = empty;
    }

    //printf("The size of vcb is: %d\n", (int) sizeof(myvcb));


    // Write the vcb to memory at block 0
    if (bwrite(0, &myvcb) < 0) {
        perror("ERROR: Could not write vcb to disk.\n");
    }

   // printf("The size of root_dir is %d\n", (int) sizeof(root_dir));

    // Write the root dir to memory at block 1
    char* root_tmp = malloc(sizeof(root));
    memcpy(root_tmp, &root, sizeof(root));
    if (dwrite(1, root_tmp) < 0) {
        perror("Error writing root dir to disk.");
    }
    free(root_tmp);

    // Write the root dir entries to memory at block 2 
    char* root_dir_tmp = malloc(sizeof(root_dir));
    memcpy(root_dir_tmp, &root_dir, sizeof(root_dir));
    if (dwrite(2, root_dir_tmp) < 0) {
        perror("Error writing root dir entries to disk.");
    }
    free(root_dir_tmp);

    // Write the rest of the disk as free blocks
    for (int i = 3; i < size; i++) {
        freeb f = free_blocks[i-3];
        char* free_tmp = malloc(sizeof(f));
        memcpy(free_tmp, &f, sizeof(f));
        // printf("Writing free block %d to disk at block %d.\n", f.next.index, i);
        if (dwrite(i, free_tmp) < 0) {
            perror("Error writing free blocks to disk.");
        }
        free(free_tmp);
    }

    /* 3600: AN EXAMPLE OF READING/WRITING TO THE DISK IS BELOW - YOU'LL
       WANT TO REPLACE THE CODE BELOW WITH SOMETHING MEANINGFUL. */
/*
    // first, create a zero-ed out array of memory  
    char *tmp = (char *) malloc(BLOCKSIZE);
    memset(tmp, 0, BLOCKSIZE);

    // now, write that to every block
    for (int i=0; i<size; i++) 
        if (dwrite(i, tmp) < 0) 
            perror("Error while writing to disk");

    // voila! we now have a disk containing all zeros
*/
    // Do not touch or move this function
    dunconnect();
}

int main(int argc, char** argv) {
    // Do not touch this function
    if (argc != 2) {
        printf("Invalid number of arguments \n");
        printf("usage: %s diskSizeInBlockSize\n", argv[0]);
        return 1;
    }

    unsigned long size = atoi(argv[1]);
    printf("Formatting the disk with size %lu \n", size);
    myformat(size);
}
