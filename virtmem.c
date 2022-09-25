#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Some compile-time constants.
 */

#define REPLACE_NONE 0
#define REPLACE_FIFO 1
#define REPLACE_LRU  2
#define REPLACE_SECONDCHANCE 3
#define REPLACE_OPTIMAL 4


#define TRUE 1
#define FALSE 0
#define PROGRESS_BAR_WIDTH 60
#define MAX_LINE_LEN 100


/*
 * Some function prototypes to keep the compiler happy.
 */
int setup(void);
int teardown(void);
int output_report(void);
long resolve_address(long, int);
long replace_fifo(long);
long replace_lru(long);
long replace_second_chance(long);
void error_resolve_address(long, int);


/*
 * Variables used to keep track of the number of memory-system events
 * that are simulated.
 * 
 */

int page_faults = 0;
int mem_refs    = 0;
int swap_outs   = 0;
int swap_ins    = 0;
long timestamp   = 0;
int victim_frame = 0;



/*
 * Page-table information.
struct page_table_entry *page_table = NULL;
struct page_table_entry {
    long page_num;
    int dirty;
    int free;
    int ref_bit;
    int last_access;
};


/*
 * These global variables will be set in the main() function.
 */

int size_of_frame = 0;  /* power of 2 */
int size_of_memory = 0; /* number of frames */
int page_replacement_scheme = REPLACE_NONE;


/*
 * Function to convert a logical address into its corresponding 
 * physical address. The value returned by this function is the
 * physical address (or -1 if no physical address can exist for
 * the logical address given the current page-allocation state.
 */
long resolve_address(long logical, int memwrite) {
    int i;
    long page, frame;
    long offset;
    long mask = 0;
    long effective;

    /* Get the page and offset */
    page = (logical >> size_of_frame);

    for (i = 0; i < size_of_frame; i++) {
        mask = mask << 1;
        mask |= 1;
    }
    offset = logical & mask;

    /* Find page in the inverted page table. */
    frame = -1;
    for (i = 0; i < size_of_memory; i++ ) {
        if (!page_table[i].free && page_table[i].page_num == page) {
            frame = i;
            break;
        }
    }


    /* If frame is not -1, then we can successfully resolve the
     * address and return the result. 
     * 
     * so here the page is already in a frame (page hit)
     * for second chance we set the ref bit to 1
     * for lru, then we also update the last_access
     * to be the current timestamp
     */
    if (frame != -1) {
        page_table[frame].ref_bit = 1;
        page_table[frame].last_access = timestamp;
        if (memwrite == TRUE) {
            page_table[frame].dirty = TRUE;
        }
        effective = (frame << size_of_frame) | offset;
        return effective;
    }


    /* If we reach this point, there was a page fault. Find
     * a free frame. */
    page_faults++;
    swap_ins++;

    for (i = 0; i < size_of_memory; i++) {
        if (page_table[i].free) {
            frame = i;
            break;
        }
    }

    /* If we found a free frame, then patch up the
     * page table entry and compute the effective
     * address. Otherwise return -1.
     * 
     * update its timestamp for fifo and lru
     * set its reference bit to 1 for second chance
     */
    if (frame != -1) {
        page_table[frame].page_num = page;
        page_table[frame].free = FALSE;
        page_table[frame].last_access = timestamp;
        page_table[frame].ref_bit = 1;
        if (memwrite == TRUE) {
            page_table[frame].dirty = TRUE;
        }
        effective = (frame << size_of_frame) | offset;
        return effective;
    }

    /*
    free frame not found, use page replacement scheme to swap a page out
    */
    else {
        if (page_replacement_scheme == REPLACE_FIFO) {
            frame = replace_fifo(page);
            victim_frame = (victim_frame + 1) % size_of_memory;
            if (page_table[frame].dirty == TRUE) {
                swap_outs++;
                page_table[frame].dirty = FALSE;
            }
            if (memwrite == TRUE) {
                page_table[frame].dirty = TRUE;
            }
            effective = (frame << size_of_frame) | offset;
            return effective;
        }
        else if (page_replacement_scheme == REPLACE_LRU) {
            frame = replace_lru(page);
            if (page_table[frame].dirty == TRUE) {
                swap_outs++;
                page_table[frame].dirty = FALSE;
            }
            if (memwrite == TRUE) {
                page_table[frame].dirty = TRUE;
            }
            effective = (frame << size_of_frame) | offset;
            return effective;
        }
        else if (page_replacement_scheme == REPLACE_SECONDCHANCE) {
            frame = replace_second_chance(page);
            victim_frame = (victim_frame + 1) % size_of_memory;
            if (page_table[frame].dirty == TRUE) {
                swap_outs++;
                page_table[frame].dirty = FALSE;
            }
            if (memwrite == TRUE) {
                page_table[frame].dirty = TRUE;
            }
            effective = (frame << size_of_frame) | offset;
            return effective;
        }
        
    }

    return -1;
}

long replace_second_chance(long page) {
    //always start at victim frame and increment it by one at the end
    //use victim frame to iterate through the page table and
    //remove the first frame with ref bit set to 0.

   
    while (1) {
        //current bit is set to 0, so it is our victim frame
        if (page_table[victim_frame].ref_bit == 0) {
            page_table[victim_frame].page_num = page;
            page_table[victim_frame].ref_bit = 1;
            break;
        }
        //current bit is set to 1, so set it to zero and check next one
        else if (page_table[victim_frame].ref_bit == 1) {
            page_table[victim_frame].ref_bit = 0;
            victim_frame = (victim_frame + 1) % size_of_memory;
        }
    }
    return victim_frame;
    
    
}

//simply remove the frame the victim frame points to
long replace_fifo(long page) {
    page_table[victim_frame].page_num = page;
    return victim_frame;
}

//iterate through all the pages in the page table,
//and select the one with the lowest last access time

long replace_lru(long page) {
    int current_min = page_table[0].last_access;
    int victim = 0;
    for (int i = 0; i < size_of_memory; i++) {
        if (page_table[i].last_access < current_min) {
            current_min = page_table[i].last_access;
            victim = i;
        }
    }
    page_table[victim].page_num = page;
    page_table[victim].last_access = timestamp;
    return victim;
}


/*
 * Super-simple progress bar.
 */
void display_progress(int percent)
{
    int to_date = PROGRESS_BAR_WIDTH * percent / 100;
    static int last_to_date = 0;
    int i;

    if (last_to_date < to_date) {
        last_to_date = to_date;
    } else {
        return;
    }

    printf("Progress [");
    for (i=0; i<to_date; i++) {
        printf(".");
    }
    for (; i<PROGRESS_BAR_WIDTH; i++) {
        printf(" ");
    }
    printf("] %3d%%", percent);
    printf("\r");
    fflush(stdout);
}


int setup()
{
    int i;

    page_table = (struct page_table_entry *)malloc(
        sizeof(struct page_table_entry) * size_of_memory
    );

    if (page_table == NULL) {
        fprintf(stderr,
            "Simulator error: cannot allocate memory for page table.\n");
        exit(1);
    }

    for (i=0; i<size_of_memory; i++) {
        page_table[i].free = TRUE;
    }

    return -1;
}


int teardown()
{

    return -1;
}


void error_resolve_address(long a, int l)
{
    fprintf(stderr, "\n");
    fprintf(stderr, 
        "Simulator error: cannot resolve address 0x%lx at line %d\n",
        a, l
    );
    exit(1);
}


int output_report()
{
    printf("\n");
    printf("Memory references: %d\n", mem_refs);
    printf("Page faults: %d\n", page_faults);
    printf("Swap ins: %d\n", swap_ins);
    printf("Swap outs: %d\n", swap_outs);

    return -1;
}


int main(int argc, char **argv)
{
    /* For working with command-line arguments. */
    int i;
    char *s;

    /* For working with input file. */
    FILE *infile = NULL;
    char *infile_name = NULL;
    struct stat infile_stat;
    int  line_num = 0;
    int infile_size = 0;

    /* For processing each individual line in the input file. */
    char buffer[MAX_LINE_LEN];
    long addr;
    char addr_type;
    int  is_write;

    /* For making visible the work being done by the simulator. */
    int show_progress = FALSE;

    /* Process the command-line parameters. Note that the
     * REPLACE_OPTIMAL scheme is not required for A#3.
     */
    for (i=1; i < argc; i++) {
        if (strncmp(argv[i], "--replace=", 9) == 0) {
            s = strstr(argv[i], "=") + 1;
            if (strcmp(s, "fifo") == 0) {
                page_replacement_scheme = REPLACE_FIFO;
            } else if (strcmp(s, "lru") == 0) {
                page_replacement_scheme = REPLACE_LRU;
            } else if (strcmp(s, "secondchance") == 0) {
                page_replacement_scheme = REPLACE_SECONDCHANCE;
            } else if (strcmp(s, "optimal") == 0) {
                page_replacement_scheme = REPLACE_OPTIMAL;
            } else {
                page_replacement_scheme = REPLACE_NONE;
            }
        } else if (strncmp(argv[i], "--file=", 7) == 0) {
            infile_name = strstr(argv[i], "=") + 1;
        } else if (strncmp(argv[i], "--framesize=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_frame = atoi(s);
        } else if (strncmp(argv[i], "--numframes=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_memory = atoi(s);
        } else if (strcmp(argv[i], "--progress") == 0) {
            show_progress = TRUE;
        }
    }

    if (infile_name == NULL) {
        infile = stdin;
    } else if (stat(infile_name, &infile_stat) == 0) {
        infile_size = (int)(infile_stat.st_size);
        /* If this fails, infile will be null */
        infile = fopen(infile_name, "r");  
    }


    if (page_replacement_scheme == REPLACE_NONE ||
        size_of_frame <= 0 ||
        size_of_memory <= 0 ||
        infile == NULL)
    {
        fprintf(stderr, 
            "usage: %s --framesize=<m> --numframes=<n>", argv[0]);
        fprintf(stderr, 
            " --replace={fifo|lru|optimal} [--file=<filename>]\n");
        exit(1);
    }


    setup();

    while (fgets(buffer, MAX_LINE_LEN-1, infile)) {
        line_num++;
        if (strstr(buffer, ":")) {
            sscanf(buffer, "%c: %lx", &addr_type, &addr);
            if (addr_type == 'W') {
                is_write = TRUE;
            } else {
                is_write = FALSE;
            }

            if (resolve_address(addr, is_write) == -1) {
                error_resolve_address(addr, line_num);
            }
            mem_refs++;
        } 

        if (show_progress) {
            display_progress(ftell(infile) * 100 / infile_size);
        }
        timestamp++;
    }
    

    teardown();
    output_report();

    fclose(infile);

    exit(0);
}
