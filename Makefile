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
	-system "CRTSRCPF LOGGER/QSRVSRC RCDLEN(150) MBR(*NONE)"
	system "CPYFRMSTMF FROMSTMF(logger.bnd) TOMBR('/qsys.lib/LOGGER.lib/QSRVSRC.FILE/LOGGER.MBR') MBROPT(*REPLACE)"
	ld -v -x -o logger.so $(OBJ) $(ILDFLAGSE) -qSRCMBR=LOGGER -qSRCFILE=LOGGER/QSRVSRC


%.o : %.c Makefile
	icc $(ICCFLAGSE) $< -o $@

logger_test: logger_test.o logger.so
	icc -v -o logger_test logger_test.o -qTGTRLS=$(TGTRLS) -rLOGGER/LOGGER

	