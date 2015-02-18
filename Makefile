##### Debug #####
DEBUG = -DDEBUG

##### Output #####
TGT = e.exe

##### Sources #####
HDRS = eexe.h

SRCS = eexe.c

RCS = eexe.rc

OBJS = $(SRCS:.c=.obj)

RES = $(RCS:.rc=.res)

LIBS = kernel32.lib user32.lib gdi32.lib advapi32.lib

##### Commands #####
CC = cl -nologo

RC = rc

LD = cl

RM = del

ZIP = zip

##### Flags #####
CFLAGS = -MD -G6 -O2

RCFLAGS =

LDFLAGS = $(CFLAGS)

RFLAGS =

##### Options #####
.SUFFIXES : .exe .obj .c .h .rc .res .lib

##### Precious Rules #####
.c.obj :
	$(CC) $(CFLAGS) -c $<

.rc.res :
	$(RC) $(RCFLAGS) $<

##### Rules #####
all : $(TGT)

zip : $(TGT)
	$(ZIP) -9 $(TGT:.exe=.zip) $(TGT) $(SRCS) $(RCS) $(HDRS) Makefile README.txt

clean :
	-$(RM) $(RFLAGS) $(OBJS) $(RES)

distclean : clean
	-$(RM) $(RFLAGS) $(TGT)

$(TGT) : $(OBJS) $(RES)
	$(LD) $(LDFLAGS) -Fe$(TGT) $(OBJS) $(LIBS) $(RES)

$(OBJS) : $(HDRS)

$(RES) : $(HDRS)
