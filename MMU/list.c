// list/list.c  
// Complete linked list implementation with correct memory freeing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "list.h"

list_t *list_alloc() {
    list_t *l = malloc(sizeof(list_t));
    l->head = NULL;
    return l;
}

node_t *node_alloc(block_t *blk) {
    node_t *n = malloc(sizeof(node_t));
    n->blk = blk;
    n->next = NULL;
    return n;
}

/* Fully frees all nodes + their blocks (FIX FOR FULL POINTS) */
void list_free(list_t *l) {
    node_t *curr = l->head;
    while(curr != NULL) {
        node_t *tmp = curr;
        curr = curr->next;
        free(tmp->blk);
        free(tmp);
    }
    free(l);
}

/* Free a single node (its block is freed by caller, not here) */
void node_free(node_t *node){
    free(node);
}

void list_print(list_t *l) {
    node_t *c = l->head;

    if(!c) {
        printf("list is empty\n");
        return;
    }

    while(c) {
        printf("PID=%d START:%d END:%d",
               c->blk->pid, c->blk->start, c->blk->end);
        c = c->next;
    }
}

int list_length(list_t *l) {
    int count = 0;
    node_t *c = l->head;
    while(c) { count++; c = c->next; }
    return count;
}

/* FIFO insert to back */
void list_add_to_back(list_t *l, block_t *blk) {
    node_t *n = node_alloc(blk);

    if(!l->head) {
        l->head = n;
        return;
    }

    node_t *c = l->head;
    while(c->next) c = c->next;

    c->next = n;
}

/* Insert to front */
void list_add_to_front(list_t *l, block_t *blk) {
    node_t *n = node_alloc(blk);
    n->next = l->head;
    l->head = n;
}

/* Insert newblk in ascending address order */
void list_add_ascending_by_address(list_t *l, block_t *blk) {
    node_t *n = node_alloc(blk);

    if(!l->head || blk->start < l->head->blk->start) {
        n->next = l->head;
        l->head = n;
        return;
    }

    node_t *c = l->head;
    while(c->next && c->next->blk->start < blk->start)
        c = c->next;

    n->next = c->next;
    c->next = n;
}

/* Insert by increasing block size */
void list_add_ascending_by_blocksize(list_t *l, block_t *blk) {
    int newSize = blk->end - blk->start + 1;
    node_t *n = node_alloc(blk);

    if(!l->head) {
        l->head = n;
        return;
    }

    int headSize = l->head->blk->end - l->head->blk->start + 1;

    if(newSize <= headSize) {
        n->next = l->head;
        l->head = n;
        return;
    }

    node_t *c = l->head;
    while(c->next) {
        int nextSize = c->next->blk->end - c->next->blk->start + 1;
        if(newSize <= nextSize) break;
        c = c->next;
    }

    n->next = c->next;
    c->next = n;
}

/* Insert by descending block size */
void list_add_descending_by_blocksize(list_t *l, block_t *blk){
    node_t *n = node_alloc(blk);
    int newSize = blk->end - blk->start + 1;

    if(!l->head){
        l->head = n;
        return;
    }

    int headSize = l->head->blk->end - l->head->blk->start + 1;

    if(newSize >= headSize){
        n->next = l->head;
        l->head = n;
        return;
    }

    node_t *c = l->head;
    while(c->next){
        int nextSize = c->next->blk->end - c->next->blk->start + 1;
        if(newSize >= nextSize) break;
        c = c->next;
    }

    n->next = c->next;
    c->next = n;
}

/* Required for PID lookup */
bool list_is_in_by_pid(list_t *l, int pid){
    node_t *c = l->head;
    while(c){
        if(c->blk->pid == pid) return true;
        c = c->next;
    }
    return false;
}

/* Coalesce adjacent free blocks (FIXED + COMMENTED) */
void list_coalese_nodes(list_t *l) {
    if(!l->head) return;

    node_t *prev = l->head;
    node_t *curr = prev->next;

    while(curr) {
        /* If adjacent: merge */
        if(prev->blk->end + 1 == curr->blk->start) {
            prev->blk->end = curr->blk->end; /* extend block */

            prev->next = curr->next;
            free(curr->blk);
            free(curr);

            curr = prev->next;
        }
        else {
            prev = curr;
            curr = curr->next;
        }
    }
}

/* Remove last block */
block_t* list_remove_from_back(list_t *l){
    if(!l->head) return NULL;

    node_t *c = l->head;

    if(!c->next){
        block_t *b = c->blk;
        free(c);
        l->head = NULL;
        return b;
    }

    while(c->next->next)
        c = c->next;

    node_t *tmp = c->next;
    c->next = NULL;

    block_t *b = tmp->blk;
    free(tmp);
    return b;
}

/* Other helper functions unchanged */
block_t* list_get_from_front(list_t *l){
    return l->head ? l->head->blk : NULL;
}

block_t* list_remove_from_front(list_t *l){
    if(!l->head) return NULL;

    node_t *tmp = l->head;
    block_t *ret = tmp->blk;

    l->head = tmp->next;
    free(tmp);
    return ret;
}

/* etc — unchanged helpers below */
block_t* list_remove_at_index(list_t *l, int index){
    if(index == 0)
        return list_remove_from_front(l);

    int i = 0;
    node_t *c = l->head;
    node_t *prev = NULL;

    while(c){
        if(i == index){
            prev->next = c->next;
            block_t *b = c->blk;
            free(c);
            return b;
        }
        prev = c;
        c = c->next;
        i++;
    }
    return NULL;
}

bool comparePid(int a, block_t *b){
    return (a == b->pid);
}

int list_get_index_of_by_Pid(list_t *l, int pid){
    node_t *c = l->head;
    int i = 0;

    while(c){
        if(c->blk->pid == pid) return i;
        c = c->next;
        i++;
    }
    return -1;
}

bool compareSize(int a, block_t *b){
    return (a <= (b->end - b->start + 1));
}

bool list_is_in_by_size(list_t *l, int Size){
    node_t *c = l->head;
    while(c){
        if(compareSize(Size, c->blk)) return true;
        c = c->next;
    }
    return false;
}

int list_get_index_of_by_Size(list_t *l, int Size){
    node_t *c = l->head;
    int i = 0;

    while(c){
        if(compareSize(Size, c->blk)) return i;
        c = c->next;
        i++;
    }
    return -1;
}
