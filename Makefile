.PHONY: all package

OBJ=logger.o

DEBUG=*ALL
TGTRLS=V7R1M0

ILDFLAGSE =-v -qTGTRLS=$(TGTRLS) -qDETAIL=*BASIC -qSTGMDL=*INHERIT
ICCFLAGSE =-v -c -qFLAG=20 -qDTAMDL=*P128 -qTGTRLS=$(TGTRLS) -qOUTPUT=*PRINT -qDBGVIEW=$(DEBUG) -qINCDIRFIRST -zIFSIO -I./ -qENUM=*INT -qPACKSTRUCT=1 -qTERASPACE='*YES' -qSTGMDL=*INHERIT

all: logger.so logger_test

clean:
	rm -f $(OBJ)

logger.so: $(OBJ) Makefile
	-system "CRTSRCPF SYSLOG/QSRVSRC RCDLEN(150) MBR(*NONE)"
	system "CPYFRMSTMF FROMSTMF(logger.bnd) TOMBR('/qsys.lib/SYSLOG.lib/QSRVSRC.FILE/LOGGER.MBR') MBROPT(*REPLACE)"
	ld -v -x -o logger.so $(OBJ) $(ILDFLAGSE) -qSRCMBR=LOGGER -qSRCFILE=SYSLOG/QSRVSRC -d'IBMi universal logger'


%.o : %.c Makefile
	icc $(ICCFLAGSE) $< -o $@

logger_test: logger_test.o logger.so
	icc -v -o logger_test logger_test.o -qTGTRLS=$(TGTRLS) \
            -d'LOGGER test' -rSYSLOG/LOGGER


package: logger.so
	-system "DLTLIB LOGGERPRD"
	system "CRTLIB LOGGERPRD"
	system "CRTDUPOBJ OBJ(LOGGER) NEWOBJ(LOGGER) FROMLIB(SYSLOG) TOLIB(LOGGERPRD) OBJTYPE(*SRVPGM)"
	system "CHGSRVPGM SRVPGM(LOGGERPRD/LOGGER) RMVOBS(*CRTDTA *DBGDTA)"
	-system "DLTF SYSLOG/LOGGER"
	system "CRTSAVF SYSLOG/LOGGER"
	system "SAVOBJ OBJ(LOGGER) LIB(LOGGERPRD) DEV(*SAVF) SAVF(SYSLOG/LOGGER) TGTRLS($(TGTRLS))"
	system "CPYTOSTMF FROMMBR('/QSYS.LIB/SYSLOG.LIB/LOGGER.FILE') TOSTMF('/home/btab/svn/syslog/logger.savf') STMFOPT(*REPLACE) CVTDTA(*NONE)"
	jar -cMvf Logger.zip LOGGER.SAVF
	