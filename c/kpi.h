#pragma once
///
/// Purpose of this header is to implement Key Performance Indicators
///
/// Running it will produce KPI syslogs
/// Compile it with gcc kpi.c -o kpi -lpthread
///
/// @file        kpi.h
///
/// @Revision    $Rev$
/// @Author      $Author$
/// @Date        $Date$
///
#ifndef _KPI_H
#define _KPI_H
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//
// Cut n paste from std.h
//
#define __constructor       __attribute__((constructor))
#define __destructor        __attribute__((destructor))


///
/// Method to create a double time (<secs>.<usecs>) format double
/// @public
///
static inline double doubleTime()
{
    double ret = 0.0;
    struct timeval  quantum = { 0, 0 };
    if(gettimeofday(&quantum, NULL) == 0) {
        ret += (double)quantum.tv_sec;
        ret += (double)quantum.tv_usec / 1000000.0;
    }

    return ret;
}

//
// Avoid compiler warnings and shorten the declarations
//
typedef struct kpi_t* Prekpi;

///
/// Public method which init's the kpi_t wrapper handle
///
/// @memberof kpi_t
/// @public
///
typedef struct kpi_t {
    const char*         name;   ///< Name of the Key Performance Indicator
    const char*         type;   ///< Type of the KPI (NULL=None)
    double             start;   ///< Start time of the KPI
    double             begin;   ///< Beinging of current transaction
    double               end;   ///< End of the last transaction
    int                  num;   ///< Numbner of transactions processed
    double           latency;   ///< Average latency of transaction
    double              busy;   ///< Total time spend by transactions
    bool          (*ctx_func)(Prekpi); ///< Custom Context method
    void*               ctx;    ///< user context data.
    Prekpi             next;   ///< Next KPI in link list
} kpi_t;

///
/// Global list of KPIs, for each Thread
///
__thread kpi_t*   KPIs = NULL;

///
/// Public method which logs and dumps the current kpi to syslog
///
/// @memberof kpi_t
/// @public
///
static inline void kpi_print(kpi_t* kpi, const char* prefix, char* buff, size_t maxlen)
{
    if(kpi == NULL)
    {
        if(buff == NULL)
        {
            for(kpi=KPIs; kpi; kpi=kpi->next)
            {
                kpi_print(kpi, prefix, NULL, 0);
            }
        }
        return;
    }

    if(prefix == NULL) {
        prefix = "INFO";
    }

    if(buff)
    {
        snprintf(buff, maxlen, "%s KPI:%s%s NUM:%d LATENCY:%.9f BUSY:%f DURATION:%f",
            prefix,
            kpi->name, kpi->type,
            kpi->num, kpi->latency,
            kpi->busy,
            kpi->end - kpi->start);
    }
    else
    {
        fprintf(stderr, "%s KPI:%s%s NUM:%d LATENCY:%.9f BUSY:%f DURATION:%f\n",
            prefix,
            kpi->name, kpi->type,
            kpi->num, kpi->latency,
            kpi->busy,
            kpi->end - kpi->start);
    }
}

#define KPIST_PORT 51515

inline static void* kpiThread(void* arg)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        fprintf(stderr, "kpi socket error\n");
        return arg;
    }

    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof (optval));

    struct sockaddr_in  sin = { 0 };
    sin.sin_family = AF_INET;
    sin.sin_port   = KPIST_PORT;
    sin.sin_addr.s_addr = INADDR_ANY;

    if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        fprintf(stderr, "kpi bind error\n");
        return arg;
    }

    // Blocking recv for new stat requests
    //
    char                sid[32] = "none";
    struct sockaddr_in  rad = { 0 };
    int                 ren = sizeof(rad);
    int                dlen = 0;
    while((dlen = recvfrom(fd, sid, sizeof sid, MSG_TRUNC, (struct sockaddr*)&rad, (socklen_t*)&ren)) > 0)
    {
        char resp[8192] = "";

        // Now respond to the request using the SID in the responses
        //
        for(kpi_t* kpi = KPIs; kpi; kpi=kpi->next) 
        {
            resp[0] = kpi->next?'1':'0';
            resp[1] = ':';
            kpi_print(kpi, &resp[2], resp, sizeof(resp));
            sendto(fd, resp, strlen(resp), 0, (struct sockaddr*)&rad, sizeof(rad));
        }
    }

    close(fd);

    return arg;
}

///
/// Public method which starts up the KPI socket
///
/// @memberof kpi_t
/// @public
///
static inline void __constructor kpi_init(void) 
{
    openlog("kpiunit", LOG_CONS | LOG_PID, LOG_USER);

    pthread_t   tid;
    pthread_create(&tid, NULL, kpiThread, NULL);
    pthread_detach(tid);
}

///
/// Public method which restarts an existing kpi
///
/// @memberof kpi_t
/// @public
///
static inline kpi_t* kpi_restart(kpi_t* kpi)
{
    if(kpi == NULL) {
        for(kpi=KPIs; kpi; kpi=kpi->next)
        {
            kpi_restart(kpi);
        }
    }
    else {
        kpi->start = doubleTime();
        kpi->num        = 0;
        kpi->begin      = kpi->start;
        kpi->end        = kpi->start;
        kpi->latency    = 0.0;
        kpi->busy       = 0.0;
    }
    return kpi;
}

///
/// Public method which finds named KPI
///
/// @memberof kpi_t
/// @public
///
static inline kpi_t* kpi_find(
    const char*     name,
    const char*     type,
    kpi_t*        kpi)
{
    kpi_t* match = NULL;

    if(kpi == NULL) {
        for(kpi=KPIs; kpi && match == NULL; kpi=kpi->next) {
            match = kpi_find(name, type, kpi);            
        }
    }
    else {
        if(name == NULL || !strncmp(name, kpi->name, strlen(name))) {
            match = kpi;
        }

        if(match && type == NULL || !strncmp(type, kpi->type, strlen(type))) {
            match = kpi; 
        }
    }
    return match;
}


///
/// Public method which starts a kpi and allocates the object
///
/// @memberof kpi_t
/// @public
///
static inline kpi_t* kpi_start(
    const char*     name,
    const char*     type,
    kpi_t*        kpi)
{
    if(kpi == NULL) {
        kpi = calloc(1, sizeof(kpi_t));
        kpi->name  = name;
        kpi->type  = type;
        kpi->next  = KPIs;
        KPIs       = kpi;
    }
    return kpi_restart(kpi);
}

///
/// Public method which starts a kpi and allocates the object
///
/// @memberof kpi_t
/// @public
///
static inline kpi_t* kpi_begin(kpi_t* kpi)
{
    kpi->begin = doubleTime();
    return kpi;
}

///
/// Public method which ends a kpi and allocates the object
///
/// @memberof kpi_t
/// @public
///
static inline double kpi_end(kpi_t* kpi)
{
    kpi->num += 1;
    kpi->end = doubleTime();
    double duration = kpi->end - kpi->begin;
    kpi->busy += duration;
    kpi->latency = kpi->busy / kpi->num;

    return duration;
}

///
/// Public method which logs and dumps the current kpi to log
///
/// @memberof kpi_t
/// @public
///
static inline void kpi_log(kpi_t* kpi, const char* prefix)
{
    if(kpi == NULL)
    {
        for(kpi=KPIs; kpi; kpi=kpi->next)
        {
            kpi_log(kpi, prefix);
        }
        return;
    }

    if(prefix == NULL) {
        prefix = "INFO";
    }

    syslog(LOG_INFO, "%s KPI:%s%s NUM:%d LATENCY:%.9f BUSY:%f DURATION:%f",
           prefix,
           kpi->name, kpi->type,
           kpi->num, kpi->latency,
           kpi->busy,
           kpi->end - kpi->start);
}

///
/// Public method which frees off a kpi
///
/// @memberof kpi_t
/// @public
///
static inline void kpi_free(kpi_t** kpi)
{
    if(kpi == NULL) {
        kpi_t*   next = NULL;
        for(kpi_t* this=KPIs; this; this=next)
        {
            next = this->next;
            kpi_free(&this);
        }
        KPIs=NULL;
    } 
    else {
        if(*kpi) {
            free(*kpi);
        }
        *kpi = NULL;
    }
}

///
/// Dump all KPI's and free them at exit
///
/// @memberof kpi_t
/// @public
///
static void  __destructor kpi_dump(void)
{
    kpi_log(NULL, "DUMP");
    kpi_free(NULL);
    closelog();
}

#endif // _KPI_H
