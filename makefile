
PROGNAME   = sshlogin
VERSION    = 

OBJFILES   = 
INCFILES   = 

CFLAGS_GEN = -Wall -funsigned-char -g -ggdb -I/usr/local/include/ \
			 -I/opt/local/include/ -I/usr/include/libxml2/ 

CFLAGS_DBG = 
CFLAGS_OPT = $(CFLAGS_GEN)

LDFLAGS   += -L/usr/local/lib/ -L/opt/local/lib
LIBS      += -lpthread

all: $(PROGNAME)

$(PROGNAME): $(PROGNAME).c $(OBJFILES) $(INCFILES)
	$(CC) $(LDFLAGS) $(PROGNAME).c -o $(PROGNAME) $(CFLAGS_OPT) $(OBJFILES) $(LIBS)
	@echo
	@echo "print \"./ssh_login -?\" for using"
	@echo
	@echo "Jiang Wenxu <jwx0819@gmail.com>"
	@echo

debug: $(PROGNAME).c $(OBJFILES) $(INCFILES)
	$(CC) $(LDFLAGS) $(PROGNAME).c -o $(PROGNAME) $(CFLAGS_DBG) $(OBJFILES) $(LIBS)

clean:
	rm -f $(PROGNAME) *.exe *.o *~ a.out core core.[1-9][0-9]* *.stackdump \
	      LOG same_test
	rm -rf tmpdir

