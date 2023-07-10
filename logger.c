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
static iconv_t E2U;

typedef struct logger logger;
struct logger {
   enum logger_severity severity;
   char                *ident;
   char                *client;

   enum logger_facility facility;
   enum logger_protocol protocol;
   char                *address;
   char                *port;
   _RTX_ENTRY           destroy_proc;
   _POINTER             this;

   pthread_key_t        localKey;
}; 

typedef struct loggerThreadLocal loggerThreadLocal;
struct loggerThreadLocal {
   int                  log_fd;
   char                *outbuffer;
   char                *outbuffer_u;
   int                  buflength;
   int                  ulength;
   int                  elength;
   char                *error;
   int                  num_connection_tries;
};

static void *test_thread(void *data) {
   return NULL;
}

void localDestructor(void *value) {
   loggerThreadLocal *ptr = value;
   if(ptr->log_fd>=0) close(ptr->log_fd);
   if (ptr->outbuffer) free(ptr->outbuffer);
   if (ptr->outbuffer_u) free(ptr->outbuffer_u);
   free(ptr);
}

loggerThreadLocal *getLocal(logger *l) {
   loggerThreadLocal *ptr;
   if((ptr= pthread_getspecific(l->localKey)) == NULL) {
      ptr = malloc(sizeof(loggerThreadLocal));
      memset(ptr, 0, sizeof(loggerThreadLocal));
      ptr->log_fd=-1;
      ptr->num_connection_tries=10;
      pthread_setspecific(l->localKey, ptr);
   }
   return ptr;
}

static void init_logger(void)
{
   QtqCode_T from, to;

   memset(&to, 0, sizeof(to));
   memset(&from, 0, sizeof(from));
   from.CCSID = 0; to.CCSID = 1208;
   E2U = QtqIconvOpen(&to, &from);
}  

static int __close(logger *l) {

   loggerThreadLocal *lcl = getLocal(l);

   switch (l->protocol) {
   case LOGGER_UDP:
   case LOGGER_TCP: {
      if (lcl->log_fd >= 0) close(lcl->log_fd);
   } break;
   }
   lcl->log_fd = -1;
   return 0;
}

static int __connect(logger *l) {
   loggerThreadLocal *lcl = getLocal(l);

   if (lcl->log_fd >= 0) __close(l);
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
         lcl->error = gai_strerror(error);
      }

      for (res = res0; res; res = res->ai_next) {
         lcl->error = NULL;
         lcl->log_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
         if (lcl->log_fd < 0) {
            lcl->error = "Unable to open socket";
            continue;
         }
         if (connect(lcl->log_fd, res->ai_addr, res->ai_addrlen) < 0) {
            lcl->error = "Unable to connect socket";
            close(lcl->log_fd);
            lcl->log_fd = -1;
            continue;
         }
         break;
      }
      freeaddrinfo(res0);
      so = 1;
      if (setsockopt(lcl->log_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&so, sizeof(int)) == -1) {
         close(lcl->log_fd);
         lcl->log_fd = -1;
         lcl->error = "Unable to setsockopt SO_REUSEADDR";
      }
      so = 1024 * 1024;
      if (setsockopt(lcl->log_fd, SOL_SOCKET, SO_SNDBUF, (void *)&so, sizeof(int)) == -1) {
         close(lcl->log_fd);
         lcl->log_fd = -1;
         lcl->error = "Unable to setsockopt SO_SNDBUF 1024*1024";
      }
   } break;
   case LOGGER_STDOUT:
   case LOGGER_STDERR:
   case LOGGER_JOBLOG:
   break;
   }
   if(lcl->log_fd>=0) lcl->num_connection_tries=10;
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

   loggerThreadLocal *lcl = getLocal(l);
   
   if (!lcl->buflength) {
      lcl->buflength = 1025;
      lcl->outbuffer = malloc(lcl->buflength);
      if(l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) lcl->outbuffer_u = malloc(lcl->buflength * 4 + 1);
   }

   lcl->ulength = 0;
   gettimeofday(&tv, &tzp);
   nowtime = tv.tv_sec;
   localtime_r(&nowtime, &nowtm);
   if (tzp.tz_minuteswest > 0) tzs = '-';
   else { tzs = '+'; tzp.tz_minuteswest = -tzp.tz_minuteswest; }
   tzh = tzp.tz_minuteswest / 60;
   tzm = tzp.tz_minuteswest % 60;
   strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d-%H:%M:%S", &nowtm);
   sprintf(buf, "%s.%06lu%c%02d:%02d", tmbuf, tv.tv_usec, tzs, tzh, tzm);
   
   meta_length = sprintf(lcl->outbuffer, "<%d> %-10.10sT%-21.21s %s %s 0x%.8X - - ", (l->facility << 3) + priority, buf, buf + 11, l->client, l->ident, tid);
   il = meta_length;
   ib = (unsigned char *)lcl->outbuffer;
   if (l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) {
      ob = (unsigned char *)lcl->outbuffer_u;
      ol = lcl->buflength * 4 + 1;
      iconv(E2U, &ib, &il, &ob, &ol);
      *ob++ = '\xEF'; *ob++ = '\xBB'; *ob++ = '\xBF';
      ol = ob - lcl->outbuffer_u;
      *ib++ = ' '; *ib++ = ' '; *ib++ = ' ';
      il = ib - lcl->outbuffer;
      meta_length += 3;
   }
   else
   {
      ib = (unsigned char *)lcl->outbuffer + meta_length;
   }
   while (1) {
      va_list args_copy;
      va_copy(args_copy, args);
      n = vsnprintf(lcl->outbuffer + meta_length, lcl->buflength - meta_length, message, args);
      if (n == lcl->buflength - meta_length - 1) {
         va_copy(args, args_copy);
         lcl->buflength = (lcl->buflength + 1024) + 1;
         lcl->outbuffer = realloc(lcl->outbuffer, lcl->buflength);
         if (l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) lcl->outbuffer_u = realloc(lcl->outbuffer_u, lcl->buflength*4+1);
      }
      else break;
   }

   if (l->protocol == LOGGER_UDP || l->protocol == LOGGER_TCP) {
      ib = lcl->outbuffer + il;
      ob = lcl->outbuffer_u + ol;
      il = n;
      ol = n*4+1;
      if (n) {
         iconv(E2U, &ib, &il, &ob, &ol);
      }

      lcl->elength = ib - lcl->outbuffer;
      lcl->ulength = ob - lcl->outbuffer_u;
   }
   else
   {
      lcl->elength = meta_length + n;
   }
   return 0;
}

static int __write(logger *l) {

   loggerThreadLocal *lcl = getLocal(l);

   if(lcl->log_fd<0 && lcl->num_connection_tries) {
      lcl->num_connection_tries--;
      __connect(l);
   }

   switch (l->protocol) {
   case LOGGER_UDP: {
      if (lcl->log_fd < 0) return 1;
      if (send(lcl->log_fd, lcl->outbuffer_u, lcl->ulength, 0) == -1) {
         __close(l);
         return 1;
      }
   } break;
   case LOGGER_TCP: {
      if (lcl->log_fd < 0) return 1;
      if (write(lcl->log_fd, lcl->outbuffer_u, lcl->ulength) == -1) {
         __close(l);
         return 1;
      }
   } break;
   case LOGGER_STDOUT: {
      int rc = fprintf(stdout, "%s\n", lcl->outbuffer);
      return rc==-1;
   } break;
   case LOGGER_STDERR: {
      int rc = fprintf(stderr, "%s\n", lcl->outbuffer);
      return rc==-1;
   } break;
   case LOGGER_JOBLOG: {
      return Qp0zLprintf("%s\n", lcl->outbuffer)==-1;
   } break;
   }
   return 0;
}

int log_it(logger *l, int priority, char *message, ...) {
   va_list va_args;

   if (priority > l->severity) return 0;

   va_start(va_args, message);
   __message(l, priority, message, va_args);
   va_end(va_args);
   if (__write(l)) {
      loggerThreadLocal *lcl = getLocal(l);
      Qp0zLprintf("%s\n", lcl->outbuffer); // write to joblog
      return 1;
   }
   
   return 0;
}

static void destroy_logger(void **pl) {
   logger *l = *pl;
   long long e_c = 0;
   loggerThreadLocal *lcl;
   CEEUTX(&(l->destroy_proc), (void *)&e_c);

   __close(l);
   lcl = getLocal(l);
   pthread_setspecific(l->localKey, NULL);
   localDestructor(lcl);


   if (l->address) free(l->address);
   if (l->port) free(l->port);
   if (l->ident) free(l->ident);
   if (l->client) free(l->client);
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

   pthread_key_create(&l->localKey, localDestructor);
   
   l->destroy_proc = destroy_logger;
   l->this = l;
   
   get_job_name(jobq);
   l->client = strdup(jobq);

   __connect(l);

   CEERTX(&l->destroy_proc, &l->this, (void *)&e_c);

   return l;
}

char *logger_error(logger *l) {
   loggerThreadLocal *lcl = getLocal(l);
   return lcl->error;
}

void close_logger(logger *l) {
   destroy_logger(&l);
}
