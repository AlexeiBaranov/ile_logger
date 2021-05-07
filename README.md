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

The logger service program will automatically add additional fields required by the syslog protocol. Logged message will shown as follows:

syslog:
```
May  7 16:40:34 127.0.0.1 local0:panic|emerg  2021-05-07T16:40:34.017624+03:00 AS400:494517/USRA/QP0ZSPWP test 0x000000B1 - - message: 123
```
joblog:
```
<128> 2021-05-07T16:25:29.336976+03:00 AS400:494498/USRA/QP0ZSPWP test 0x000000A9 - - message: 123                               
```
Logger.zip contains saved SRVPGM object LOGGER. To restore - put it to IFS, unzip, upload to SAVF, then use RSTOBJ.

