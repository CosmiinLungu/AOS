/*
    sim_pag_fifo.c
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sim_paging.h"

//Function that initialize the tables

void init_tables (ssystem * S)
{
    int i;

    // Pages to zero
    memset (S->pgt, 0, sizeof(spage)*S->numpags);

    //LRU Stack--> Empty
    S->lru = -1;

    // Time --> LRU(t) to zero
    S->clock = 0;

    // Circular list with free frames
    for (i=0; i<S->numframes-1; i++)
    {
        S->frt[i].page = -1;
        S->frt[i].next = i+1;
    

		S->frt[i].page = -1;  // i == numframes-1
		S->frt[i].next = 0; // Close the circular list
		S->listfree = i;     // Point to the last
    }
    // Circular list that contains busy(?) empty frames 
    S->listoccupied = -1;
}
void reference_page (ssystem * S, int page, char op)
{
    if (op=='R')                         // if read,
        S->numrefsread ++;            // count
    else if (op=='W')
    {                                    // if write
        S->pgt[page].modified = 1;   // count and set the page as modified
        S->numrefswrite ++;          
    }
	S->pgt[page].referenced=1;
}
int choose_page_to_be_replaced (ssystem * S)
{
    int frame, victim;
	while(1){//While we are at zero the reference does not choose the victim
		frame = S->frt[S->listoccupied].next;         
		if (S->pgt[S->frt[frame].page].referenced!=1){			
			victim = S->frt[frame].page; 
			break;
		}
		else{
			S->pgt[S->frt[frame].page].referenced=0;
		}
		S->listoccupied= S->frt[S->listoccupied].next;
	}
    if (S->detailed)
        printf ("@ Choosing trough the FIFO2 P%d of M%d for "
                "replacing it\n", victim, frame);

    return victim;
}

// Function that simulate the operating system

void handle_page_fault (ssystem * S, unsigned virtual_addr)
{
    int page, victim, frame, last;
    S -> numpagefaults ++; //Increment the page number counter
    page = virtual_addr / S -> pagsz ; //Calculate the page where the page fault is occuring
    if (S -> detailed )
		printf ("@ 1⁄2Page faults at P %d !\n" , page );
	if (S-> listfree != -1) // if free frames exist
	{
		last = S-> listfree ; //save the last one
		frame = S-> frt [ last ]. next ; //take the next one
		if ( frame == last ) // if they are the same
			S-> listfree = -1; // it was only one free; 
		else
			S-> frt [ last ]. next = S-> frt [ frame ]. next ; // if not, we replace them with the new ones
		occupy_free_frame (S, frame , page );
	}
	else // if no free frames
	{
		victim = choose_page_to_be_replaced (S); //we choose page with the algorithm
		replace_page (S, victim , page ); //and we clear the page
	}
}

//Functions that simulate the MMU hardware
unsigned sim_mmu (ssystem * S, unsigned virtual_addr, char op)
{
    unsigned physical_addr;
    int page, frame, offset;
	
    page= virtual_addr / S -> pagsz ; //Divide the virtual address and we obtain the number of the page  
    offset = virtual_addr % S -> pagsz ; //The reminder of the dividing is the offset
    if ( page <0 || page >=S -> numpags )//check if the page number is less than 0 or is bigger/equal than the page number of the pages
    {
	S -> numillegalrefs ++;
	return ~0U; // return the physical address FF...F
    }
    if (!S -> pgt [ page ]. present ) //if the page is not present in memory
		handle_page_fault (S , virtual_addr ); // try to find/fail it
    // now it is the page
    frame = S -> pgt [ page ]. frame ; 
    physical_addr = frame * S -> pagsz + offset ; // calculate physical address 
    reference_page (S , page , op ); //We set the reference bit to 1
    if (S -> detailed )
		printf ("\t %c %u == P %d(M %d)+ %d\n" , op , virtual_addr , page , frame , offset );
    return physical_addr;
}


void replace_page (ssystem * S, int victim, int new_one)
{
    int frame;

    frame = S->pgt[victim].frame;

    if (S->pgt[victim].modified)
    {
        if (S->detailed)
            printf ("@ Push P%d modified disk to "
                    "replaceit\n", victim);

        S->numpgwriteback ++;
    }

    if (S->detailed)
	{
        printf ("@ Replace the victim P%d for P%d to M%d\n",
                victim, new_one, frame);
	}
	//On replacing one present for the other; the operations modified are realized
	S->pgt[victim].present = 0; //Since the replaced present is no longer in memory, its presence bit is set to 0

    S->pgt[new_one].present = 1; //the replaced present is the one that occupies the free place, its present bit is set 1
    S->pgt[new_one].frame = frame; //the new one is like a temporal, where it is stored
    S->pgt[new_one].modified = 0;//the modifed bit of the new one , we set to 0	

    S->frt[frame].page = new_one; //put the temporal one in the page one
}

void occupy_free_frame (ssystem * S, int frame, int page)
{   
    if (S->detailed)
        printf ("@ Hosting P%d to M%d\n", page, frame);
    
    S->pgt[page].present = 1;
    S->pgt[page].frame = frame;
    S->pgt[page].modified = 0;
    S->frt[frame].page = page;
}

// Functions that shows the results

void print_page_table (ssystem * S)
{
    int p;

    printf ("%10s %10s %10s %10s  %s\n",
            "Page", "Present", "Frame", "Modified", "Referenced");
	
	
    for (p=0; p<S->numpags; p++)
    {
        if (S->pgt[p].present)
            printf ("%8d   %6d     %8d   %6d	%8d\n", p,
                    S->pgt[p].present, S->pgt[p].frame,
                    S->pgt[p].modified,
					S->pgt[p].referenced);
        else
            printf ("%8d   %6d     %8s   %6s\n", p,
                    S->pgt[p].present, "-", "-");
    }
}

void print_frames_table (ssystem * S)
{
    int p, m;

    printf ("%10s %10s %10s %10s   %s\n",
            "frame", "Page", "present", "modified", "Referenced");

    for (m=0; m<S->numframes; m++)
    {
        p = S->frt[m].page;

        if (p==-1)
            printf ("%8d   %8s   %6s     %6s\n", m, "-", "-", "-");
        else if (S->pgt[p].present)
            printf ("%8d   %8d   %6d     %6d  %8d\n", m, p,
                    S->pgt[p].present, S->pgt[p].modified, S->pgt[p].referenced);
        else
            printf ("%8d   %8d   %6d     %6s   ¡ERROR!\n", m, p,
                    S->pgt[p].present, "-");
    }
}

void print_replacement_report (ssystem * S)
{
    int z;
    printf("Fifo2: \n" "Frames: \n ");//Show the result
    for(int k=0; k<S->numframes; k++){
        z=S->pgt[S->frt[k].page].present;
        if(z==1)
       	   printf ("(The complete frames  %8d  which contains the page %8d referenced: %8d\n", k, S->frt[k].page, S->pgt[S->frt[k].page].referenced); 
    }

}


