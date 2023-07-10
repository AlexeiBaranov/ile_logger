      *
      *  ile_logger example (c) Alexei Baranov alexei.baranov@i2rest.com
      *
      *  To compile:
      *  CRTRPGMOD MODULE(YOURLIB/RLOGGER) SRCFILE(YOURLIB/QRPGLESRC) DBGVIEW(*ALL)
      *  CRTPGM PGM(YOURLIB/RLOGGER) BNDSRVPGM((LOGGER/LOGGER))
      *
      *  Ensure syslogd service started on your IBM i
      *  (see https://www.i2rest.com/index.php/I2Rest_with_syslog)
      *
      *  Then:
      *  CALL YOURLIB/RLOGGER
      *
      *  To change logging destination, use other parameters in create_logger
      *
      *//////////////////////////////////////////////////////////
      * PROTORYPES
      *//////////////////////////////////////////////////////////

      * open_logger - Create logger handler
      ***********************************************************
     D open_logger     PR              *   extproc('open_logger')
      *                Returns pointer to logger handler
      *
     D    LogIdent                     *   Value Options(*String)
      *                Identify the device or application that
      *                originated the message
      *
     D    Facility                   10I 0 Value
      *
     D    FAC_KERN     C                   0
     D    FAC_USER     C                   1
     D    FAC_MAIL     C                   2
     D    FAC_DAEMON   C                   3
     D    FAC_AUTH     C                   4
     D    FAC_SYSLOG   C                   5
     D    FAC_LPR      C                   6
     D    FAC_NEWS     C                   7
     D    FAC_UUCP     C                   8
     D    FAC_CRON     C                   9
     D    FAC_AUTHPRV  C                   10
     D    FAC_FTP      C                   11
     D    FAC_LOCAL0   C                   16
     D    FAC_LOCAL1   C                   17
     D    FAC_LOCAL2   C                   18
     D    FAC_LOCAL3   C                   19
     D    FAC_LOCAL4   C                   20
     D    FAC_LOCAL5   C                   21
     D    FAC_LOCAL6   C                   22
     D    FAC_LOCAL7   C                   23
      *
     D    Severity                   10I 0 Value
      *
     D    SEV_EMERG    C                   0
     D    SEV_ALERT    C                   1
     D    SEV_CRIT     C                   2
     D    SEV_ERR      C                   3
     D    SEV_WARNING  C                   4
     D    SEV_NOTICE   C                   5
     D    SEV_INFO     C                   6
     D    SEV_DEBUG    C                   7
      *
     D    Protocol                   10I 0 Value
      *
     D    PR_UDP       C                   0
     D    PR_TCP       C                   1
     D    PR_STDOUT    C                   2
     D    PR_STDERR    C                   3
     D    PR_JOBLOG    C                   4
      *
     D    Host                         *   Value Options(*String)
      *                Required for UDP & TCP
      *                Example: 'localhost'
      *                         '127.0.0.1'
      *
     D    Port                         *   Value Options(*String)
      *                Required for UDP & TCP
      *                Example: '514'

      *
      * close_logger - Close logger handler
      ***********************************************************
     D close_logger    PR                  extproc('close_logger')
      *
     D    Handler                      *   Value
      *

      *
      * logger_error - Returns logger error (if any)
      ***********************************************************
     D logger_error    PR              *   extproc('logger_error')
      *
     D    Handler                      *   Value
      *

      *
      * log_it       - Produce logger message
      ***********************************************************
     D log_it          PR            10I 0 extproc('log_it')
      *
     D    Handler                      *   Value
      *
     D    Priority                   10I 0 Value
      *
     D    Format                       *   Value OPTIONS(*STRING)
      *                C printf format, for example 'Message: %s'

     D                                 *   Value OPTIONS(*STRING:*NOPASS)
     D                                 *   Value OPTIONS(*STRING:*NOPASS)
     D                                 *   Value OPTIONS(*STRING:*NOPASS)
     D                                 *   Value OPTIONS(*STRING:*NOPASS)
     D                                 *   Value OPTIONS(*STRING:*NOPASS)
     D                                 *   Value OPTIONS(*STRING:*NOPASS)
      *             ...Optional parameters that corresponds to Format
      *                Add as many as required, use other types if needed

     D Handler         s               *
     D rc              s             10I 0

      *
      * Mainline
      ********************************************************************
     C                   Eval      Handler = open_logger('RPGLE':
     C                                                   FAC_LOCAL0:
     C                                                   SEV_DEBUG:
     C                                                   PR_UDP:
     C                                                   'localhost':
     C                                                   '514')

     C                   Eval      rc = log_it(Handler:
     C                                         SEV_EMERG:
     C                                         'Message: %s':
     C                                         'Hi from RPGLE')

     C                   Callp     close_logger(Handler)

     C                   Return
