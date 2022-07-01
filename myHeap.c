///////////////////////////////////////////////////////////////////////////////
//
//
//James Gui
//Gui (login)
//jgui2@wisc.edu (email)
//no outside help
//myHeap.c (program)
///////////////////////////////////////////////////////////////////////////////

// DEB'S PARTIAL SOLUTION FOR SPRING 2021 DO NOT SHARE
 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           
    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *  can be 3, 2, 1, or 0, remember to modulo by 8 to determine, i think
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */

    int size_status;


} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;   
blockHeader *endMarkGlobal = NULL;   

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;




    /*
    * checks if the new block is a better match for best fit than the current selected block
    * returns an int indicating if the block is a better match or not
    * return of 0 means it is not a good match
    * return of 1 means it is a perfect match 
    * return of 2 means it is a better match than the current selected block
    * params
    * sizeToCheckTo size of the block needed, size to be checked
    * blockToCheck the new block being checked for
    * mostFavorableBlock the currently best block to be checked for
    */
 int checkIfCloseEnough(int sizeToCheckTo, blockHeader* blockToCheck, blockHeader* mostFavorableBlock)
 {
     int blockToCheckSizeModified =  blockToCheck -> size_status - (blockToCheck -> size_status % 8) ; //this gives the size of the block without overhang
     int mostFavorableBlockSizeModified = 0; 

    //below if statement checks if there is a currently favored block, if not then it determines if the block to check is a suitable block
     if(mostFavorableBlock == NULL)
        {
            if(blockToCheckSizeModified < sizeToCheckTo)
            {
                
                return 0;
            }

            else if(blockToCheckSizeModified == sizeToCheckTo)
            {
                
                return 1;
            }

            else if( blockToCheckSizeModified > sizeToCheckTo) 
            {
                return 2;
            }
        }

    
    mostFavorableBlockSizeModified  = mostFavorableBlock -> size_status - ( mostFavorableBlock -> size_status % 8) ; //size of block without overhang

    //determines if the new block is better than the origninal block
     if( blockToCheckSizeModified < sizeToCheckTo)
     {
         
         return 0;
     }

     else if( blockToCheckSizeModified == sizeToCheckTo)
     {
         return 1;
     }

    else if( blockToCheckSizeModified <= mostFavorableBlockSizeModified )
     {
        return 2;
     }

     return 0;
 }

/* 
* This function is called to split a block when needed, returns if the block size is too small to split, otherwise it splits the block and
* creates a new header and footer marking the new block
* params
* blockToSplit is the block to be checked for splitting and split if possible
* sizeToSplit, size of the orignal block that is needed to be split
* previousblockallocated, checks if the previous block is allocated
*/
 void chooseAndSplitBlock(blockHeader* blockToSplit, int sizeToSplit, int previousBlockAlloced)
 {
    int modifiedSizeStatus = ( blockToSplit -> size_status  ) - ( blockToSplit -> size_status % 8); // block size without overhang
    
    int remanderSize = ( modifiedSizeStatus ) - sizeToSplit; //remander of the block if size of original block is taken out

    //if the new block isn't big enough, end the function
    if(remanderSize < 8 )
    {
        blockToSplit -> size_status = blockToSplit -> size_status + 1;

        blockHeader* nextBlockHeader = (blockHeader*) (( void* ) blockToSplit + modifiedSizeStatus );
        
        if(nextBlockHeader -> size_status  == 1)
        {
            
            
            return;
        }
        else{
            nextBlockHeader -> size_status = nextBlockHeader-> size_status + 2;
            
            
            return;
        }
    }
  
    //create a new block header and footer as the new block is big enough to split
    blockHeader* newBlock = (blockHeader*)  ( (void*) blockToSplit +  sizeToSplit ) ; 
    blockHeader* newFooterHeader = (blockHeader*) ( (void*) newBlock + remanderSize - 4);
    
    //sets up the size status's for all pertanant blocks
   if(previousBlockAlloced == 0)
   {
        blockToSplit -> size_status = sizeToSplit + 1; 
        
   }
   else 
   {
       blockToSplit -> size_status = sizeToSplit + 3;
       
   }

    newBlock -> size_status = ( modifiedSizeStatus - sizeToSplit) + 2;
    
    
    newFooterHeader -> size_status = remanderSize; 

    
    return;

    //add header and footer, check previous footer
 }
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 */
void* myAlloc(int size) {   

    if(size <= 0)
    {
       
        return NULL;
    }

    
    int sizeToAdd = size + 4 ; //size of block + 4 nessecary for the header

    int tempInt;

    int previousBlockStatus;

    if(sizeToAdd % 8 != 0)
    {
        sizeToAdd = sizeToAdd + (8 - (sizeToAdd % 8)); //makes sure size is a multiple of 8, required for block sizes
    }
    
    blockHeader*  currentBlockStatus  = (blockHeader*) ( (void*) heapStart ); 
    blockHeader*  mostFavorableBlock  = NULL;

    int currentBlockSize = currentBlockStatus -> size_status - ( ( currentBlockStatus -> size_status ) % 8 ); //block size without overhang
    

    int blockStatusOverflow = currentBlockStatus -> size_status % 8;


    //this while loop compares block sizes, and proceeds accordingly, if the block is not suitable it continues to the next block
    //if the block is free it checks block sizes, if the block is allocated it continues to the next block, stops at the end of the heap
    //if the block is exactly perfect it returns the block + 4 as the pointer value, since the pointer must not inclue the header the +4 is needed
    //if the block is not perfect but better than the original block then it continues.
    while( (currentBlockStatus -> size_status != 1) )
    { 
        switch (blockStatusOverflow)
        {
            //if the block is exactly perfect it returns the block + 4 as the pointer value, since the pointer must not inclue the header the +4 is needed
            //if the block is not perfect but better than the original block then it continues.
            case 0: //means is free prev is free, have previous set as not filled
                    
                
                tempInt = checkIfCloseEnough( sizeToAdd, currentBlockStatus, mostFavorableBlock );
                
                //have previous set as yes filled

                switch (tempInt)
                    {
                    case 0:
                        break;

                    case 1:
                        mostFavorableBlock = (blockHeader*) ( (void* )currentBlockStatus );
                        chooseAndSplitBlock(mostFavorableBlock, sizeToAdd, 0);
                        void* returnItem = (void*)  mostFavorableBlock + 4;
                        
                        return  returnItem;
                        break;

                    case 2:
                        mostFavorableBlock = (blockHeader*) ( (void* )currentBlockStatus );
                        previousBlockStatus = 0;
                        break;

                    default:
                        
                        break;
                    }

                break;

            //block is allocated, continue
            case 1:
                
                
                previousBlockStatus = 1;
                
                break;

            //if the block is exactly perfect it returns the block + 4 as the pointer value, since the pointer must not inclue the header the +4 is needed
            //if the block is not perfect but better than the original block then it continues.
            case 2: //also means current block is free prev is allocated
                
                tempInt = checkIfCloseEnough( sizeToAdd, currentBlockStatus, mostFavorableBlock );
                
                //have previous set as yes filled

                switch (tempInt)
                    {
                    case 0:
                        
                        break;

                    case 1:
                        mostFavorableBlock = (blockHeader*) ( (void* )currentBlockStatus );
                        
                        chooseAndSplitBlock(mostFavorableBlock, sizeToAdd, previousBlockStatus);
                        
                        void* returnItem = (void*)  mostFavorableBlock + 4;
                        

                        return  returnItem;
                        break;

                    case 2:
                        mostFavorableBlock = (blockHeader*) ( (void* ) currentBlockStatus );
                        
                        break;

                    default:
                        
                        break;
                    }
                break;

            //block is allocated, continue
            case 3:
            
            previousBlockStatus = 1;
                    
            break;

    
        default:
            
            break;

        }
        
        //goes to the next block
        currentBlockSize =  currentBlockStatus -> size_status - ( ( currentBlockStatus -> size_status ) % 8);

        currentBlockStatus =  (blockHeader*) ( (void*) currentBlockStatus + currentBlockSize ); 
  
        blockStatusOverflow = (currentBlockStatus -> size_status) % 8;
            
        if(currentBlockStatus -> size_status == 1 )
        {
            if(mostFavorableBlock == NULL)
            {
                return NULL;
            }
            chooseAndSplitBlock(mostFavorableBlock, sizeToAdd, previousBlockStatus);
            void* returnItem = ( (void*)   mostFavorableBlock + 4 ) ;
            
            return returnItem;
        }
                 
    }

    //shouldn't ever get here but its always good to have backup. 
    if(mostFavorableBlock == NULL)
    {
        return NULL;
    }
    else{
        chooseAndSplitBlock(mostFavorableBlock, sizeToAdd, previousBlockStatus);
                
            void* returnItem = (void*) mostFavorableBlock + 4;
            return returnItem;
    }
} 
 


/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                    
int myFree(void *ptr) {   

    //checks if the ptr is a usable value
    if(ptr == NULL)
    {
        return -1;
    }


    if(( (int) ptr ) % 8 != 0)
    {
        printf("not a multiple of 8");
        return -1;
    }

    //frees the pointer, including the header 
    if( (int) ptr <  (int) heapStart || (int) ptr > ( (int) heapStart + allocsize ))
    {
        return -1;
    }

    else{
        
        blockHeader* blockPtr = (blockHeader*)   ( (void*) ptr - 4 ) ; 

        blockHeader* nextBlockHeader = NULL;

        //if the block pointer is not a multiple of 8, it is not usable
        if( ( blockPtr -> size_status % 8) == 0 || ( blockPtr -> size_status % 8) == 2)
        {
            printf("cannot free, current size status is %i, meaning its already freed \n", blockPtr->size_status );
            return -1;
        }

        int modifiedSizeStatus = ( blockPtr -> size_status  ) - ( blockPtr -> size_status % 8);
        

        nextBlockHeader = (blockHeader*) ( (void*)  blockPtr + modifiedSizeStatus );
        nextBlockHeader -> size_status = nextBlockHeader -> size_status - 2;
        blockPtr -> size_status = (blockPtr -> size_status - 1);

        
        return 0;
    }

    return -1;
} 



/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
    
    blockHeader* currentBlock = (blockHeader*) ( ( void*) heapStart ); 
    int modifiedSizeStatus = currentBlock -> size_status - (currentBlock -> size_status % 8);
    blockHeader* nextBlock = (blockHeader*) ((void*) currentBlock + modifiedSizeStatus);
    int hasCoalesed = 0;
    
    
    //the while loop to coalesce all adjacent free blocks
    while( ( currentBlock -> size_status != 1 ) )
    { 
        switch(currentBlock -> size_status % 8)
        {
            case 0:
                if(nextBlock -> size_status == 1)
                {
                    return 0;
                }
                printf("this should never have happened \n");
            break;

            case 1:
                if(nextBlock -> size_status == 1)
                {
                    return 0;
                }
            break;
            
            case 2:
                if(nextBlock -> size_status == 1)
                {
                    return 0;
                }
                if( (nextBlock -> size_status % 8 == 0 ) ||  (nextBlock -> size_status % 8 == 2))
                {
                    //I know 2 shouldn't be right but its fine for right now
                    currentBlock -> size_status = currentBlock -> size_status + (nextBlock -> size_status - (nextBlock -> size_status % 8));
                    nextBlock->size_status = 0;
                    hasCoalesed = 1;
                }
            break;
            
            case 3:
                if(nextBlock -> size_status == 1)
                {
                    return 0;
                }
            break;

            default:
                if(nextBlock -> size_status == 1)
                {
                    return 0;
                }
                printf("should never have come here line 467, \n");
            break;
        }

        if(hasCoalesed == 0)
        {
            currentBlock = (blockHeader* ) ( ( void* ) nextBlock);
            modifiedSizeStatus = nextBlock -> size_status - ( nextBlock -> size_status % 8);
            nextBlock = (blockHeader*) ( (void*) nextBlock + modifiedSizeStatus );
        }
        else{
            modifiedSizeStatus = currentBlock -> size_status - ( currentBlock -> size_status % 8);
            nextBlock = (blockHeader*) ( (void*) currentBlock + modifiedSizeStatus );
            hasCoalesed = 0;
        }
        
    }

    printf("reached end of coalese, this is the end result %d \n", currentBlock ->size_status);
    printf("\n");
    printf("\n");
    printf("\n");
    
    return 0;
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;
    endMarkGlobal = (blockHeader*) (void*) endMark;


    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 

                                    

