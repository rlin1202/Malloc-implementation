#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* The standard allocator interface from stdlib.h.  These are the
 * functions you must implement, more information on each function is
 * found below. They are declared here in case you want to use one
 * function in the implementation of another. */
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

/* When requesting memory from the OS using sbrk(), request it in
 * increments of CHUNK_SIZE. */
#define CHUNK_SIZE (1<<12)

struct mem_Block {
    size_t size;
    struct mem_Block *next;
} mem_Block;

//write a validator to check the blocks
//size should be the size given to the user not the size user requests for
static struct mem_Block *free_List[13] = {NULL};
/*
 * This function, defined in bulk.c, allocates a contiguous memory
 * region of at least size bytes.  It MAY NOT BE USED as the allocator
 * for pool-allocated regions.  Memory allocated using bulk_alloc()
 * must be freed by bulk_free().
 *
 * This function will return NULL on failure.
 */
extern void *bulk_alloc(size_t size);

/*
 * This function is also defined in bulk.c, and it frees an allocation
 * created with bulk_alloc().  Note that the pointer passed to this
 * function MUST have been returned by bulk_alloc(), and the size MUST
 * be the same as the size passed to bulk_alloc() when that memory was
 * allocated.  Any other usage is likely to fail, and may crash your
 * program.
 *
 * Passing incorrect arguments to this function will result in an
 * error message notifying you of this mistake.
 */
extern void bulk_free(void *ptr, size_t size);

/*
 * This function computes the log base 2 of the allocation block size
 * for a given allocation.  To find the allocation block size from the
 * result of this function, use 1 << block_index(x).
 *
 * This function ALREADY ACCOUNTS FOR both padding and the size of the
 * header.
 *
 * Note that its results are NOT meaningful for any
 * size > 4088!
 *
 * You do NOT need to understand how this function works.  If you are
 * curious, see the gcc info page and search for __builtin_clz; it
 * basically counts the number of leading binary zeroes in the value
 * passed as its argument.
 */
static inline __attribute__((unused)) int block_index(size_t x) {
    if (x <= 8) {
        return 5;
    } else {
        return 32 - __builtin_clz((unsigned int)x + 7);
    }
}

/*
 * You must implement malloc().  Your implementation of malloc() must be
 * the multi-pool allocator described in the project handout.
 */

void *mem_chunk(void *chunk,int chunk_index) {
    //fprintf(stderr,"memchunk is called index\n");
    //make sure malloc only returns the pointer
    //realloc make sure to keep the size;
    //free make sure to handle bulk allocations with free
    struct mem_Block *head = (struct mem_Block*) chunk;
    int blocks = CHUNK_SIZE/(1<<chunk_index);
    
    for (int i = 0;i < blocks;i++) {
        struct mem_Block *block = chunk;
        block->size = 1<<chunk_index;  
        if (i == blocks - 1) {
            block->next = NULL;
            return head;
        } else {
            block->next = chunk+=(1<<chunk_index); 
        }
    }
    // fprintf(stderr,"index is %d\n",chunk_index);
    return head;
}

void *malloc(size_t size) {
    //fprintf(stderr,"malloc is called\n");
    if(size <= 0){
        return NULL;
    }
    if(size > 4088){
        void *bulk_Alloc = bulk_alloc(size + 8);
        *(size_t*)bulk_Alloc = size;
        bulk_Alloc+=sizeof(size_t);
        return bulk_Alloc;
    }
    
    int index = block_index(size);
     
    if (free_List[index] == NULL) {
        free_List[index] = sbrk(CHUNK_SIZE);
        struct mem_Block *head = mem_chunk(free_List[index],index);  
        free_List[index] = head->next;
        return &head->next;
    }else{
        struct mem_Block *head = free_List[index];
        //fprintf(stderr,"index is %d\n",index); 
        //fprintf(stderr,"size is %d\n",(int)head->size); 
        free_List[index] = head->next;
        return &head->next;
    }
}

/*
 * You must also implement calloc().  It should create allocations
 * compatible with those created by malloc().  In particular, any
 * allocations of a total size <= 4088 bytes must be pool allocated,
 * while larger allocations must use the bulk allocator.
 *
 * calloc() (see man 3 calloc) returns a cleared allocation large enough
 * to hold nmemb elements of size size.  It is cleared by setting every
 * byte of the allocation to 0.  You should use the function memset()
 * for this (see man 3 memset).
 */
void *calloc(size_t nmemb, size_t size) {
    //fprintf(stderr,"calloc is called\n");
    size_t total_size = nmemb * size;

    if(total_size == 0){
        return NULL;
    }

    struct mem_Block *ptr = malloc(total_size);
    memset(ptr, 0,total_size - sizeof(size_t));
    return ptr;
}

/*
 * You must also implement realloc().  It should create allocations
 * compatible with those created by malloc(), honoring the pool
 * alocation and bulk allocation rules.  It must move data from the
 * previously-allocated block to the newly-allocated block if it cannot
 * resize the given block directly.  See man 3 realloc for more
 * information on what this means.
 *
 * It is not possible to implement realloc() using bulk_alloc() without
 * additional metadata, so the given code is NOT a working
 * implementation!
 */
void *realloc(void *ptr, size_t size) {
    //fprintf(stderr,"realloc is called\n");
    if(ptr == NULL){
        return malloc(size);
    }
    if(size == 0 && ptr != NULL){
        free(ptr);
        return NULL;
    }
    
    ptr-=sizeof(size_t);
    size_t ptr_size = *(size_t*)ptr;
    ptr+=sizeof(size_t);
    
    //hi Ethan & Carl &  TAs
    //call malloc - to create new block?
    //give new size to ptr?
    //(new notes - keep size malloc returns the ptr for new chunk just replace that ptr with prev ptr that user provides)
    //can we assume freelist is already allocated?
    //how to access size of ptr
    
    if(ptr_size >= 1<<block_index(size)){
        return ptr;
    } else {
        void *new_block = malloc(size);
        memcpy(new_block,ptr,ptr_size - sizeof(size_t));
        free(ptr);
        return new_block;
    }
    //fprintf(stderr,"leaving realloc size is %d\n",(int)size);
}

/*
 * You should implement a free() that can successfully free a region of
 * memory allocated by any of the above allocation routines, whether it
 * is a pool- or bulk-allocated region.
 *
 * The given implementation does nothing.
 */
void free(void *ptr) {
    return;
    //fprintf(stderr,"Free is called\n");
    if(ptr != NULL){
        ptr-=sizeof(size_t);
        size_t ptr_size = *(size_t*)ptr;
        
        if(ptr_size > CHUNK_SIZE){
            //fprintf(stderr,"Bulk_Free is called\n");
            bulk_free(ptr,ptr_size);
        }else{
            int index = block_index(ptr_size - 8);
            //fprintf(stderr,"index is %d\n",index);
            struct mem_Block *pointer = ptr;
            pointer->next = free_List[index];
            free_List[index] = pointer;
            //fprintf(stderr,"ptr size is %d\n",(int)pointer->size);
        }
    }
}
