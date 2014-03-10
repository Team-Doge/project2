#ifndef __3600DEBUG_H__
#define __3600DEBUG_H__
#include "3600fs.h"
#include "stdbool.h"
const int DEBUG = true;
#define debug(...) ((DEBUG) ? printf(__VA_ARGS__) : -1)


// Debugger functions
void debug_dirent(dirent* d);
void debug_direntry(direntry* e);
void debug_block(blocknum* b);
void debug_inode(inode* i);
void debug_dnode(dnode* d);
void debug_vcb(vcb* v);
void debug_freeb(freeb* f);

void debug_vcb(vcb* v) {
    debug("VCB information\n");
    debug("\tMagic number: %d\n", v->magic);
    debug("\tBlocksize: %d\n", v->blocksize);
    debug("\tRoot block:\n");
    debug_block(&v->root);
    debug("\tFirst free block:\n");
    debug_block(&v->free);
    debug("\tDisk name: %s", v->name);
}

void debug_dnode(dnode* d) {
    debug("Directory node metadata\n");
    debug("\tSize: %d\n", d->size);
    debug("\tUID: %d\n", d->user);
    debug("\tGID: %d\n", d->group);
    debug("\tMode: LATER\n");
    debug("\tAccess time: %lld.%.9ld\n", (long long)d->access_time.tv_sec, d->access_time.tv_nsec);
    debug("\tModify time: %lld.%.9ld\n", (long long)d->modify_time.tv_sec, d->modify_time.tv_nsec);
    debug("\tCreate time: %lld.%.9ld\n", (long long)d->create_time.tv_sec, d->create_time.tv_nsec);
    debug("\tDirect blocks:\n");
    for (int i = 0; i < 54; i++) {
        blocknum b = d->direct[i];
        if (b.valid == true) {
            debug("Direct block %d info:\n", i);
            debug_block(&b);
        }
    }
}


void debug_inode(inode* i) {
    debug("Inode metadata\n");
    debug("\tSize: %d\n", i->size);
    debug("\tUID: %d\n", i->user);
    debug("\tGID: %d\n", i->group);
    debug("\tMode: LATER\n");
    debug("\tAccess time: %lld.%.9ld\n", (long long)i->access_time.tv_sec, i->access_time.tv_nsec);
    debug("\tModify time: %lld.%.9ld\n", (long long)i->modify_time.tv_sec, i->modify_time.tv_nsec);
    debug("\tCreate time: %lld.%.9ld\n", (long long)i->create_time.tv_sec, i->create_time.tv_nsec);
    debug("\tDirect blocks:\n");
    for (int j = 0; j < 54; j++) {
        blocknum b = i->direct[j];
        if (b.valid == true) {
            debug("Direct block %d info:\n", j);
            debug_block(&b);
        }
    }
}

void debug_dirent(dirent* d) {
    for (int i = 0; i < 8; i++) {
        direntry e = d->entries[i];
        debug("Info for direntry %d\n", i);
        debug_direntry(&e);
    }
}

void debug_direntry(direntry* e) {
    debug("\tDirentry name: %s\n", e->name);
    debug("\tDirentry type: %c\n", e->type);
    debug("\tDirentry block: \n");
    debug_block(&e->block);
}

void debug_block(blocknum* b) {
    debug("\t\tIndex: %d\n", b->index);
    debug("\t\tValid: %d\n", b->valid);
}

void debug_freeb(freeb* f) {
    debug("Free block information about NEXT free block:\n");
    debug_block(&f->next);
}
#endif
