SRC	= ppodd.cxx DataFile.cxx
OBJ	= $(SRC:.cxx=.o)
DEP	= $(SRC:.cxx=.d)

DEFINES  =
CXXFLAGS = -g -O0 -Wall
#CXXFLAGS = -O2 -DNDEBUG 
CXXFLAGS += $(INCLUDES) $(DEFINES)
#LIBS	= -Wl,-Bstatic -lboost_date_time -lboost_program_options -Wl,-Bdynamic
LIBS    = -pthread

PROGRAM = ppodd
GENERATOR = generate

.PHONY: all clean realclean

all:		$(PROGRAM) $(GENERATOR)

$(PROGRAM):	$(OBJ) $(DEP)
		$(CXX) $(CXXFLAGS) $(OBJ) $(LIBS) -o $@

$(GENERATOR):	$(GENERATOR).o
		$(CXX) $(CXXFLAGS) $^ -o $@

clean:
		rm -f $(PROGRAM) *.o *~

realclean:	clean
		rm -f *.d

%.o:		%.cxx Makefile
		$(CXX) -c $(CXXFLAGS) -o $@ $<

%.d:		%.cxx Makefile
		@$(SHELL) -ec 'gcc -MM $(INCLUDES) $(DEFINES) -c $< \
			| sed '\''s%^.*\.o%$*\.o%g'\'' \
			| sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
			[ -s $@ ] || rm -f $@'

