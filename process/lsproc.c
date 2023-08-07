// ANU COMP3300/COMP6330 -- Lab 02
// Alwen Tiu, 2023

#include <stdio.h>
#include <stdlib.h>
#include <proc/readproc.h>

int main(int argc, char *argv[])
{
    // Read /usr/include/proc/readproc.h to find details of the structure proc_t

    proc_t* info; 
    // The flag PROC_EDITCMDLCVT is needed to convert the cmdline field from 
    // a vector of strings to an array of strings.
    PROCTAB *pt = openproc(PROC_FILLSTATUS|PROC_FILLUSR|PROC_FILLCOM|PROC_EDITCMDLCVT); 

    while((info=readproc(pt, NULL)) != NULL) {
        printf("%c, %d, %s, %s\n", info->state, info->tid, info->ruser, *(info->cmdline)); 
    }
    return 0; 
}