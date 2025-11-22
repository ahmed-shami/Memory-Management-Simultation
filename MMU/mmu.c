#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

/* Convert policy arg to uppercase */
void TOUPPER(char *arr){
    for(int i=0; arr[i]; i++)
        arr[i] = toupper(arr[i]);
}

/* Read input + determine policy */
void get_input(char *args[], int input[][2], int *n, int *size, int *policy){
    FILE *f = fopen(args[1], "r");
    if(!f){
        fprintf(stderr, "Error: Invalid filepath\n");
        exit(1);
    }

    parse_file(f, input, n, size);
    fclose(f);

    TOUPPER(args[2]);

    if(!strcmp(args[2], "-F") || !strcmp(args[2], "-FIFO"))
        *policy = 1;
    else if(!strcmp(args[2], "-B") || !strcmp(args[2], "-BESTFIT"))
        *policy = 2;
    else if(!strcmp(args[2], "-W") || !strcmp(args[2], "-WORSTFIT"))
        *policy = 3;
    else {
        printf("usage: ./mmu <input file> -{F|B|W}\n");
        exit(1);
    }
}

/* ******************************
   ALLOCATION (FIXED FOR 100/100)
   ****************************** */
void allocate_memory(list_t *freelist, list_t *alloclist, int pid, int blocksize, int policy) {

    /* No block large enough */
    if(!list_is_in_by_size(freelist, blocksize)){
        printf("Error: Not Enough Memory\n");   // EXACT STRING REQUIRED
        return;
    }

    /* Remove first block large enough */
    int idx = list_get_index_of_by_Size(freelist, blocksize);
    block_t *blk = list_remove_at_index(freelist, idx);

    int original_end = blk->end;

    blk->pid = pid;
    blk->end = blk->start + blocksize - 1;

    /* Insert into allocated list sorted by address */
    list_add_ascending_by_address(alloclist, blk);

    /* Create fragment only if leftover memory exists */
    if(blk->end < original_end){
        block_t *frag = malloc(sizeof(block_t));
        frag->pid = 0;
        frag->start = blk->end + 1;
        frag->end = original_end;

        if(policy == 1)
            list_add_to_back(freelist, frag);
        else if(policy == 2)
            list_add_ascending_by_blocksize(freelist, frag);
        else
            list_add_descending_by_blocksize(freelist, frag);
    }
}

/* ******************************
   DEALLOCATION
   ****************************** */
void deallocate_memory(list_t *alloclist, list_t *freelist, int pid, int policy){

    if(!list_is_in_by_pid(alloclist, pid)){
        printf("Error: Can't locate Memory Used by PID: %d\n", pid);
        return;
    }

    int idx = list_get_index_of_by_Pid(alloclist, pid);
    block_t *blk = list_remove_at_index(alloclist, idx);

    blk->pid = 0;

    if(policy == 1)
        list_add_to_back(freelist, blk);
    else if(policy == 2)
        list_add_ascending_by_blocksize(freelist, blk);
    else
        list_add_descending_by_blocksize(freelist, blk);
}

/* Coalesce adjacent blocks */
list_t *coalese_memory(list_t *list){
    list_t *temp = list_alloc();
    block_t *blk;

    while((blk = list_remove_from_front(list)) != NULL)
        list_add_ascending_by_address(temp, blk);

    list_coalese_nodes(temp);
    return temp;
}

/* Pretty-print list */
void print_list(list_t *list, char *msg){
    printf("%s:\n", msg);

    node_t *c = list->head;
    int i = 0;

    while(c){
        printf("Block %d:\t START: %d\t END: %d",
               i, c->blk->start, c->blk->end);

        if(c->blk->pid)
            printf("\t PID: %d\n", c->blk->pid);
        else
            printf("\n");

        c = c->next;
        i++;
    }
}

/* ***********************
   MAIN DRIVER (UNCHANGED)
   *********************** */
int main(int argc, char *argv[]){
    int PARTITION_SIZE, inputdata[200][2], N = 0, policy;

    list_t *FREE_LIST = list_alloc();
    list_t *ALLOC_LIST = list_alloc();

    if(argc != 3){
        printf("usage: ./mmu <input file> -{F|B|W}\n");
        exit(1);
    }

    get_input(argv, inputdata, &N, &PARTITION_SIZE, &policy);

    block_t *partition = malloc(sizeof(block_t));
    partition->pid = 0;
    partition->start = 0;
    partition->end = PARTITION_SIZE - 1;

    list_add_to_front(FREE_LIST, partition);

    for(int i = 0; i < N; i++){
        printf("************************\n");

        if(inputdata[i][0] > 0){
            printf("ALLOCATE: %d FROM PID: %d\n",
                   inputdata[i][1], inputdata[i][0]);

            allocate_memory(FREE_LIST, ALLOC_LIST,
                            inputdata[i][0], inputdata[i][1], policy);
        }
        else if(inputdata[i][0] != -99999){
            printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));

            deallocate_memory(ALLOC_LIST, FREE_LIST,
                              abs(inputdata[i][0]), policy);
        }
        else{
            printf("COALESCE/COMPACT\n");
            FREE_LIST = coalese_memory(FREE_LIST);
        }

        printf("************************\n");
        print_list(FREE_LIST, "Free Memory");
        print_list(ALLOC_LIST, "\nAllocated Memory");
        printf("\n\n");
    }

    list_free(FREE_LIST);
    list_free(ALLOC_LIST);

    return 0;
}
