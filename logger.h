
#ifndef __LOGGER_H__
#define __LOGGER_H__

enum logger_protocol {
	LOGGER_UDP = 0,
	LOGGER_TCP = 1,
	LOGGER_STDOUT = 2,
	LOGGER_STDERR = 3,
	LOGGER_JOBLOG = 4
};

enum logger_severity {
	LOGGER_SEV_EMERG = 0,
	LOGGER_SEV_ALERT = 1,
	LOGGER_SEV_CRIT = 2,
	LOGGER_SEV_ERR = 3,
	LOGGER_SEV_WARNING = 4,
	LOGGER_SEV_NOTICE = 5,
	LOGGER_SEV_INFO = 6,
	LOGGER_SEV_DEBUG = 7
};

enum logger_facility {
	LOGGER_FAC_KERN = 0,
	LOGGER_FAC_USER = 1,
	LOGGER_FAC_MAIL = 2,
	LOGGER_FAC_DAEMON = 3,
	LOGGER_FAC_AUTH = 4,
	LOGGER_FAC_SYSLOG = 5,
	LOGGER_FAC_LPR = 6,
	LOGGER_FAC_NEWS = 7,
	LOGGER_FAC_UUCP = 8,
	LOGGER_FAC_CRON = 9,
	LOGGER_FAC_AUTHPRIV = 10,
	LOGGER_FAC_FTP = 11,
	LOGGER_FAC_LOCAL0 = 16,
	LOGGER_FAC_LOCAL1 = 17,
	LOGGER_FAC_LOCAL2 = 18,
	LOGGER_FAC_LOCAL3 = 19,
	LOGGER_FAC_LOCAL4 = 20,
	LOGGER_FAC_LOCAL5 = 21,
	LOGGER_FAC_LOCAL6 = 22,
	LOGGER_FAC_LOCAL7 = 23
};

void *open_logger(
	char *ident,
	enum logger_facility facility,
	enum logger_severity severity,
	enum logger_protocol protocol,
	char *address,
	char *port);

int log_it(void *logger, int priority, char *message, ...);

void close_logger(void *logger);

char *logger_error(void *logger);

#endif
