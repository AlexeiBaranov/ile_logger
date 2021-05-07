# ile_logger
Universal logger for IBM i

ile_logger is simple universal logger for using with IBM i ILE programs

It allows to log program messages by sending them to syslogd server, stdout/stderr or to job log

Usage:

```
#include "logger.h"
...
void *l;
l = open_logger("test", LOGGER_FAC_LOCAL0, LOGGER_SEV_INFO, LOGGER_UDP, "localhost", "514");
log_it(l, 0, "message: %s", "123");
close_logger(l);
```

Logger.zip contains saved SRVPGM object LOGGER. To restore - put it to IFS, unzip, upload to SAVF, then use RSTOBJ.

