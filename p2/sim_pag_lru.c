/*
    sim_pag_lru.c
*/
// tdm =frt,tdp=pgt
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sim_paging.h"

// Function that initialises the tables

void init_tables (ssystem * S)
{
    int i;

    // Reset pages
    memset (S->pgt, 0, sizeof(spage)*S->numpags);

    // Empty LRU stack
    S->lru = -1;

    // Reset LRU(t) time
    S->clock = 0;

    // Circular list of free frames
    for (i=0; i<S->numframes-1; i++)
    {
        S->frt[i].page = -1;
        S->frt[i].next = i+1;
    }

    S->frt[i].page = -1;    // Now i == numframes-1
    S->frt[i].next = 0;     // Close circular list
    S->listfree = i;        // Point to the last one

    // Empty circular list of occupied frames
    S->listoccupied = -1;
}

// Functions that simulate the hardware of the MMU

unsigned sim_mmu (ssystem * S, unsigned virtual_addr, char op)
{
    unsigned physical_addr;
    int page, frame, offset;

    // TODO: Type in the code that simulates the MMU's (hardware)
    //       behaviour in response to a memory access operation
    page = virtual_addr/S->pagsz;
    offset = virtual_addr%S->pagsz;
    if (page<0 || page>=S->numpags)
    {
    S->numillegalrefs ++;
    return ~0U;
    }
    if (!S->pgt[page].present)
    handle_page_fault (S,virtual_addr);
    // Now it is present
    frame=S->pgt[page].frame;
    physical_addr=frame*S->pagsz+offset;
    reference_page (S, page, op);
    if (S->detailed)
    printf ("\t%c %u==P%d(M%d)+%d\n", op, virtual_addr, page, frame, offset);
    return physical_addr;
}

void reference_page (ssystem * S, int page, char op)
{   
    if (op=='R')                         // If it's a read,
        S->numrefsread ++;               // count it
    else if (op=='W')
    {                                    // If it's a write,
        S->pgt[page].modified=1;       // count it and mark the
        S->numrefswrite ++;              // page 'modified'
    }
    S->pgt[page].timestamp=S->clock;
    S->clock++;
    if(S->clock==0)
    {
        printf("warning the clock is overflow");
    }
}


// Functions that simulate the operating system

void handle_page_fault (ssystem * S, unsigned virtual_addr)
{
    int page, victim, frame, last;

    // TODO: Type in the code that simulates the Operating
    //       System's response to a page fault trap
    S->numpagefaults ++;
    page = virtual_addr / S->pagsz;
    if (S->detailed)
        printf ("@ PAGE FAULT in P%d!\n", page);
    if (S->listfree!= -1)  
    {
        last = S->listfree;
        frame = S->frt[last].next;
        if (frame==last)
            S->listfree = -1;
        else
            S->frt[last].next = S->frt[frame].next;
        occupy_free_frame (S, frame, page);
    }
    else
    {
        victim = choose_page_to_be_replaced (S);
        replace_page (S, victim, page);
    }
}


int choose_page_to_be_replaced (ssystem * S)
{
    int frame =-1, victim;
    //unsigned tp_aux=82345678;//Limite(tp_aux)
    //victim=S->frt[0].page;
    //frame=0;
    //frame = myrandom (0, S->numframes);  erease       // <<--- random

   // victim = S->frt[frame].page; erease
    for(int i=0; i<S->numframes;i++){
        if(frame==-1)
            frame=i;
    if(S->pgt[S->frt[i].page].timestamp<S->pgt[S->frt[frame].page].timestamp) //
        frame=i ; 
    }

    victim = S->frt[frame].page;
        /*if(S->pgt[S->frt[frame].page].timestamp<tp_aux){
            tp_aux=S->pgt[S->frt[frame].page].timestamp;// tpd tdm
            victim=S->frt[frame].page;
        }
        frame=S->frt[frame].next;
*/
    if (S->detailed)
        printf ("@ Choosing (at random) P%d of F%d to be "
                "replaced\n", victim, frame);

    return victim;
	/*
int marco, victima;
    unsigned tp_aux=82345678;//Limite
    victima=S->tdm[0].pagina;
    marco=0;
    for(int i=0; i<S->nummarcos;i++){
        if(S->tdp[S->tdm[marco].pagina].timestamp<tp_aux){
            tp_aux=S->tdp[S->tdm[marco].pagina].timestamp;
            victima=S->tdm[marco].pagina;
        }
        marco=S->tdm[marco].sig;
    }


    if (S->detallado)
        printf ("@ Eligiendo (LRU) P%d de M%d para "
                "reemplazarla\n", victima, marco);*/
}

void replace_page (ssystem * S, int victim, int newpage)
{
    int frame;

    frame = S->pgt[victim].frame;

    if (S->pgt[victim].modified)
    {
        if (S->detailed)
            printf ("@ Writing modified P%d back (to disc) to "
                    "replace it\n", victim);

        S->numpgwriteback ++;
    }

    if (S->detailed)
        printf ("@ Replacing victim P%d with P%d in F%d\n",
                victim, newpage, frame);

    S->pgt[victim].present = 0;

    S->pgt[newpage].present = 1;
    S->pgt[newpage].frame = frame;
    S->pgt[newpage].modified = 0;

    S->frt[frame].page = newpage;
}

void occupy_free_frame (ssystem * S, int frame, int page)
{
    if (S->detailed)
        printf ("@ Storing P%d in F%d\n", page, frame);

    // TODO: Write the code that links the page with the frame and
    //       vice-versa, and wites the corresponding values in the
    //       state bits of the page (presence...)
        S->pgt[page].frame=frame; //Asignamos el marco a la pagina
		S->frt[frame].page=page; //Y la pagina al marco
		S->pgt[page].present=1; //Establecemos el bit de presencia a 1
		S->pgt[page].modified=0; //Ponemos el bit de modificacion a 0
}

// Functions that show results

void print_page_table (ssystem * S)
{
    int p;

    printf ("%10s %10s %10s   %s\n",
            "PAGE", "Present", "Frame", "Modified");

    for (p=0; p<S->numpags; p++)
        if (S->pgt[p].present)
            printf ("%8d   %6d     %8d   %6d\n", p,
                    S->pgt[p].present, S->pgt[p].frame,
                    S->pgt[p].modified);
        else
            printf ("%8d   %6d     %8s   %6s\n", p,
                    S->pgt[p].present, "-", "-");
}

void print_frames_table (ssystem * S)
{
    int p, f;

    printf ("%10s %10s %10s   %s\n",
            "FRAME", "Page", "Present", "Modified");

    for (f=0; f<S->numframes; f++)
    {
        p = S->frt[f].page;

        if (p==-1)
            printf ("%8d   %8s   %6s     %6s\n", f, "-", "-", "-");
        else if (S->pgt[p].present)
            printf ("%8d   %8d   %6d     %6d\n", f, p,
                    S->pgt[p].present, S->pgt[p].modified);
        else
            printf ("%8d   %8d   %6d     %6s   ERROR!\n", f, p,
                    S->pgt[p].present, "-");
    }
}

void print_replacement_report (ssystem * S)
{
    unsigned maxtstamp, mintstamp;
    for(int i=0; i<S->numpags; i++)
	{
		if(i==0)
		{
			maxtstamp=S->pgt[i].timestamp;
			mintstamp=S->pgt[i].timestamp;
		}
		else
		{
			if(maxtstamp<S->pgt[i].timestamp) //Si el maximo timestamp actual es menor que con el que le estamos comparando, decimos que el nuevo maximo es este ultimo
			{
				maxtstamp=S->pgt[i].timestamp;
			}
			if(mintstamp>S->pgt[i].timestamp) //Si el minimo timestamp actual es menor que con el que le estamos comparando, decimos que el nuevo minimo es este ultimo
			{
				mintstamp=S->pgt[i].timestamp;
			}
		}
		
	}
    printf ("the clock value is :%u\nthe minimum timestamp is:%u\nthe maximum timestamp is:%u\n",S->clock, mintstamp, maxtstamp);
    //printf ("Random replacement "
      //      "(no specific information)\n");   erease   // <<--- random
}


