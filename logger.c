#define __cplusplus__strings__ 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <LEENV.h>
#include <QUSRJOBI.h>
#include <QWCRNETA.h>
#include <qp0ztrc.h>

#include "logger.h"

#include <qtqiconv.h>
#include <iconv.h>   


pthread_once_t  onceControl = PTHREAD_ONCE_INIT;
static int is_multithreaded = 0;
static iconv_t E2U;

typedef struct logger logger;
struct logger {
   pthread_mutex_t      lock;
   int                  log_fd;
   char                *outbuffer;
   char                *outbuffer_u;
   int                  buflength;
   int                  ulength;
   int                  elength;
   char                *ident;
   char                *client;

   enum logger_facility facility;
   enum logger_severity severity;
   enum logger_protocol protocol;
   char                *address;
   char                *port;
   _RTX_ENTRY           destroy_proc;
   _POINTER             this;
   char                *error;
}; 

static void *test_thread(void *data) {
   return NULL;
}

static void init_logger(void)
{
   QtqCode_T from, to;

   pthread_t tid;
   if (pthread_create(&tid, NULL, (void *(*)(void *))test_thread, NULL)) {
      is_multithreaded = 0;
   }
   else {
      is_multithreaded = 1;
      pthread_join(tid, NULL);
   }
   memset(&to, 0, sizeof(to));
   memset(&from, 0, sizeof(from));
   from.CCSID = 0; to.CCSID = 1208;
   E2U = QtqIconvOpen(&to, &from);
}  

static int __close(logger *l) {
   switch (l->protocol) {
   case LOGGER_UDP:
   case LOGGER_TCP: {
      if (l->log_fd >= 0) close(l->log_fd);
   } break;
   }
   l->log_fd = -1;
   return 0;
}

static int __connect(logger *l) {

   if (l->log_fd >= 0) __close(l);
   switch (l->protocol) {
   case LOGGER_UDP: 
   case LOGGER_TCP: {
      struct addrinfo hints, *res, *res0;
      int error;
      const char *cause = NULL;
      int so;
		
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = PF_UNSPEC;
      hints.ai_socktype = (l->protocol == LOGGER_TCP ? SOCK_STREAM : SOCK_DGRAM);
      error = getaddrinfo(l->address, l->port, &hints, &res0);
      if (error) {
         l->error = gai_strerror(error);
      }

      for (res = res0; res; res = res->ai_next) {
         l->error = NULL;
         l->log_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
         if (l->log_fd < 0) {
            l->error = "Unable to open socket";
            continue;
         }
         if (connect(l->log_fd, res->ai_addr, res->ai_addrlen) < 0) {
            l->error = "Unable to connect socket";
            close(l->log_fd);
            l->log_fd = -1;
            continue;
         }
         break;
      }
      freeaddrinfo(res0);
      so = 1;
      if (setsockopt(l->log_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&so, sizeof(int)) == -1) {
         close(l->log_fd);
         l->log_fd = -1;
         l->error = "Unable to setsockopt SO_REUSEADDR";
      }
      so = 1024 * 1024;
      if (setsockopt(l->log_fd, SOL_SOCKET, SO_SNDBUF, (void *)&so, sizeof(int)) == -1) {
         close(l->log_fd);
         l->log_fd = -1;
         l->error = "Unable to setsockopt SO_SNDBUF 1024*1024";
      }
   } break;
   case LOGGER_STDOUT:
   case LOGGER_STDERR:
   case LOGGER_JOBLOG:
   break;
   }
   return 0;
}

int __message(logger *l, int priority, char *message, va_list args) {

   int n, meta_length;

   struct timeval tv;
   struct timezone tzp;
   int tzh, tzm;
   char tzs;
   time_t nowtime;
   struct tm nowtm;
   char tmbuf[64], buf[64];
   size_t il, ol;
   unsigned char *ib, *ob;
   
   int tid = pthread_getthreadid_np().intId.lo;
   
   if (!l->buflength) {
      l->buflength = 1025;
      l->outbuffer = malloc(l->buflength);
      if(l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) l->outbuffer_u = malloc(l->buflength * 4 + 1);
   }

   l->ulength = 0;
   gettimeofday(&tv, &tzp);
   nowtime = tv.tv_sec;
   localtime_r(&nowtime, &nowtm);
   if (tzp.tz_minuteswest > 0) tzs = '-';
   else { tzs = '+'; tzp.tz_minuteswest = -tzp.tz_minuteswest; }
   tzh = tzp.tz_minuteswest / 60;
   tzm = tzp.tz_minuteswest % 60;
   strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d-%H:%M:%S", &nowtm);
   sprintf(buf, "%s.%06lu%c%02d:%02d", tmbuf, tv.tv_usec, tzs, tzh, tzm);
   
   meta_length = sprintf(l->outbuffer, "<%d> %-10.10sT%-21.21s %s %s 0x%.8X - - ", l->facility << 3 + priority, buf, buf + 11, l->client, l->ident, tid);
   il = meta_length;
   ib = (unsigned char *)l->outbuffer;
   if (l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) {
      ob = (unsigned char *)l->outbuffer_u;
      ol = l->buflength * 4 + 1;
      iconv(E2U, &ib, &il, &ob, &ol);
      *ob++ = '\xEF'; *ob++ = '\xBB'; *ob++ = '\xBF';
      ol = ob - l->outbuffer_u;
      *ib++ = ' '; *ib++ = ' '; *ib++ = ' ';
      il = ib - l->outbuffer;
      meta_length += 3;
   }
   else
   {
      ib = (unsigned char *)l->outbuffer + meta_length;
   }
   while (1) {
      n = vsnprintf(l->outbuffer + meta_length, l->buflength - meta_length, message, args);
      if (n == l->buflength - meta_length) {
         l->buflength = (l->buflength + 1024) + 1;
         l->outbuffer = realloc(l->outbuffer, l->buflength);
         if (l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) l->outbuffer_u = realloc(l->outbuffer_u, l->buflength*4+1);
      }
      else break;
   }

   if (l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) {
      ib = l->outbuffer + il;
      ob = l->outbuffer_u + ol;
      il = n;
      if (n) {
         iconv(E2U, &ib, &il, &ob, &ol);
      }

      l->elength = ib - l->outbuffer;
      l->ulength = ob - l->outbuffer_u;
   }
   else
   {
      l->elength = meta_length + n;
   }
   return 0;
}

static int __write(logger *l) {
   if (l->log_fd < 0) return 1;
   switch (l->protocol) {
   case LOGGER_UDP: {
      if (send(l->log_fd, l->outbuffer_u, l->ulength, 0) == -1) {
         __close(l);
         return 1;
      }
   } break;
   case LOGGER_TCP: {
      if (write(l->log_fd, l->outbuffer_u, l->ulength) == -1) {
         __close(l);
         return 1;
      }
   } break;
   case LOGGER_STDOUT: {
      fprintf(stdout, "%s\n", l->outbuffer);
   } break;
   case LOGGER_STDERR: {
      fprintf(stderr, "%s\n", l->outbuffer);
   } break;
   case LOGGER_JOBLOG: {
      Qp0zLprintf("%s\n", l->outbuffer);
   } break;
   }
   return 0;
}

int log_it(logger *l, int priority, char *message, ...) {
   va_list va_args;

   if (priority > l->severity) return 0;

   if (is_multithreaded) pthread_mutex_lock(&l->lock);
   va_start(va_args, message);
   __message(l, priority, message, va_args);
   va_end(va_args);
   if (__write(l)) {
      Qp0zLprintf("%s\n", l->outbuffer); // write to joblog
      return 1;
   }
   if (is_multithreaded) pthread_mutex_unlock(&l->lock);
   return 0;
}

static void destroy_logger(void **pl) {
   logger *l = *pl;
   long long e_c = 0;

   CEEUTX(&(l->destroy_proc), (void *)&e_c);

   if (is_multithreaded) pthread_mutex_trylock(&l->lock);

   __close(l);
   if (l->address) free(l->address);
   if (l->port) free(l->port);
   if (l->ident) free(l->ident);
   if (l->client) free(l->client);
   if (l->outbuffer) free(l->outbuffer);
   if (l->outbuffer_u) free(l->outbuffer_u);
   if (is_multithreaded) {
      pthread_mutex_unlock(&l->lock);
      pthread_mutex_destroy(&l->lock);
   }
   free(l);
}

#define TRIM(s) {char *c = s+sizeof(s)-1; while((*c==' ' || *c==0) && c>=s) *c--=0;}
#define get_job_name(jobq)                                         \
{                                                                  \
   struct Qwc_JOBI0100 jobinfo;                                    \
   long long  ec=0;                                                \
   char jobn[7];                                                   \
   char jobu[11];                                                  \
   char jobi[11];                                                  \
   char sysf[4+4+10+1+1+4+8+1];                                    \
   char sysn[9];                                                   \
   QUSRJOBI(&jobinfo,                                              \
             sizeof(jobinfo),                                      \
             "JOBI0100",                                           \
             "*                         ",                         \
             "                ",                                   \
             &ec);                                                 \
   memcpy(jobi, jobinfo.Job_Name,  10);  jobi[10]=0; TRIM(jobi);   \
   memcpy(jobn, jobinfo.Job_Number, 6);  jobn[ 6]=0; TRIM(jobn);   \
   memcpy(jobu, jobinfo.User_Name, 10);  jobu[10]=0; TRIM(jobu);   \
   QWCRNETA(sysf, sizeof(sysf), 1, "SYSNAME   ", &ec);             \
   memcpy(sysn, sysf+4+4+10+1+1+4, 8); sysn[8]=0; TRIM(sysn);      \
   sprintf((jobq), "%s:%s/%s/%s", sysn, jobn, jobu, jobi);         \
}

logger *open_logger(
   char *ident,
   enum logger_facility facility,
   enum logger_severity severity,
   enum logger_protocol protocol,
   char *address,
   char *port) {
   
   char jobq[100] = { 0 };
   logger *l;
   long long e_c=0;

   pthread_once(&onceControl, init_logger);
   l = malloc(sizeof(logger));
   
   memset(l, 0, sizeof(logger));
   if (address) l->address = strdup(address);
   if (port) l->port = strdup(port);
   if (ident) l->ident = strdup(ident);

   l->facility = facility;
   l->severity = severity;
   l->protocol = protocol;
   
   if (is_multithreaded) {
      pthread_mutex_init(&l->lock, NULL);
   }

   l->destroy_proc = destroy_logger;
   l->this = l;
   
   get_job_name(jobq);
   l->client = strdup(jobq);

   __connect(l);

   CEERTX(&l->destroy_proc, &l->this, (void *)&e_c);

   return l;
}

char *logger_error(logger *l) {
   return l->error;
}

void close_logger(logger *l) {
   destroy_logger(&l);
}
