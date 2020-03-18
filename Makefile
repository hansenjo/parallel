SRC	= ppodd.cxx DataFile.cxx Decoder.cxx Variable.cxx Detector.cxx \
		DetectorTypeA.cxx DetectorTypeB.cxx Output.cxx Util.cxx Context.cxx
OBJ	= $(SRC:.cxx=.o)
DEP	= $(SRC:.cxx=.d)

DEFINES  =
CXXFLAGSST = -g -O -Wall # -DNDEBUG
#CXXFLAGSST = -O2 -DNDEBUG
CXXFLAGS = $(CXXFLAGSST) -pthread
CXXFLAGS += $(INCLUDES) $(DEFINES)
#LIBS	= -Wl,-Bstatic -lboost_date_time -lboost_program_options -Wl,-Bdynamic
LIBS    = -lboost_iostreams-mt

PROGRAM = ppodd
GENERATOR = generate

.PHONY: all clean realclean

all:		$(PROGRAM) $(GENERATOR)

$(PROGRAM):	$(OBJ) $(DEP)
		$(CXX) $(CXXFLAGS) $(OBJ) $(LIBS) -o $@

$(GENERATOR):	$(GENERATOR).o
		$(CXX) $(CXXFLAGSST) $^ -o $@

$(GENERATOR).o:	$(GENERATOR).cxx Makefile
		$(CXX) -c $(CXXFLAGSST) -o $@ $<

clean:
		rm -f $(PROGRAM) $(GENERATOR) *.o *~

realclean:	clean
		rm -f *.d

.SUFFIXES:
.SUFFIXES: .cxx .o .d

%.o:		%.cxx Makefile
		$(CXX) -c $(CXXFLAGS) -o $@ $<

%.d:		%.cxx Makefile
		@$(SHELL) -ec 'gcc -MM $(INCLUDES) $(DEFINES) -c $< \
			| sed '\''s%^.*\.o%$*\.o%g'\'' \
			| sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
			[ -s $@ ] || rm -f $@'


-include $(DEP)
