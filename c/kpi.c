//
// Very very simple test of the kpi implmentation 
//
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include "kpi.h"

int main(int argc, char* argv[]) 
{
    kpi_t*    total = kpi_start("TEST-", "TOTAL", NULL);
    kpi_t*    trans = kpi_start("TEST-", "TRANS", NULL);

    kpi_begin(total);
    for(int i=0; i < 1000; ++i)
    {
        kpi_begin(trans);
        usleep(1000);
        kpi_end(trans);
    }
    kpi_end(total);
    
    return 0;
}

// End of kpi.c
