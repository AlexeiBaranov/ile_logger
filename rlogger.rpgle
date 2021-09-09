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
 *  CALL YOURLIB/LOGGER                                                       
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
 *                KERN     = 0                              
 *                USER     = 1                              
 *                MAIL     = 2                              
 *                DAEMON   = 3                              
 *                AUTH     = 4                              
 *                SYSLOG   = 5                              
 *                LPR      = 6                              
 *                NEWS     = 7                              
 *                UUCP     = 8                              
 *                CRON     = 9                              
 *                AUTHPRIV = 10                             
 *                FTP      = 11                             
 *                LOCAL0   = 16                             
 *                LOCAL1   = 17                             
 *                LOCAL2   = 18                             
 *                LOCAL3   = 19                             
 *                LOCAL4   = 20                             
 *                LOCAL5   = 21                             
 *                LOCAL6   = 22                             
 *                LOCAL7   = 23                             
D    Severity                   10I 0 Value                 
 *                EMERG    = 0                              
 *                ALERT    = 1                              
 *                CRIT     = 2                              
 *                ERR      = 3                              
 *                WARNING  = 4                              
 *                NOTICE   = 5                              
 *                INFO     = 6                              
 *                DEBUG    = 7                              
D    Protocol                   10I 0 Value                 
 *                UDP      = 0                              
 *                TCP      = 1                              
 *                STDOUT   = 2                              
 *                STDERR   = 3                              
 *                JOBLOG   = 4                              
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
C                                                   LOCAL0:              
C                                                   DEBUG:               
C                                                   UDP:          
C                                                   'localhost':
C                                                   '514')      
                                                                
C                   Eval      rc = log_it(Handler:              
C                                         EMERG:                    
C                                         'Message: %s':        
C                                         'Hi from RPGLE')      
                                                                
C                   Callp     close_logger(Handler)             
                                                                
C                   Return                                      
