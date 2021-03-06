#
# Makefile for ITensor library
#
################################

include ../this_dir.mk
include ../options.mk

####################################

SOURCES=index.cc indexset.cc itensor.cc itsparse.cc \
        iqindex.cc iqindexset.cc iqtensor.cc iqtsparse.cc\
        svdworker.cc mps.cc mpo.cc dmrg.cc

HEADERS=global.h allocator.h real.h permutation.h index.h prodstats.h \
        indexset.h itensor.h qn.h iqindex.h iqindexset.h iqtensor.h \
        condenser.h combiner.h iqcombiner.h \
        svdworker.h mps.h mpo.h dmrg.h core.h observer.h DMRGObserver.h \
        BaseDMRGWorker.h DMRGWorker.h Sweeps.h hams.h measure.h model.h\
        hams/hubbardchain.h hams/heisenberg.h hams/ExtendedHubbard.h \
        hams/triheisenberg.h hams/ising.h hams/J1J2Chain.h \
        model/spinhalf.h model/spinone.h model/hubbard.h model/spinless.h\
        eigensolver.h localop.h localmpo.h localmposet.h itsparse.h iqtsparse.h\
        partition.h option.h hambuilder.h localmpo_mps.h tevol.h

####################################

INCLUDEFLAGS=-I$(INCLUDEDIR) -I$(BOOST_DIR) 

CCFLAGS= $(INCLUDEFLAGS) $(OPTIMIZATIONS)
CCGFLAGS= $(INCLUDEFLAGS) -ansi -Wall -pedantic -Wno-long-long -g -O0\
 -DDEBUG -DBOUNDS -DMATRIXBOUNDS -DSTRONG_DEBUG

OBJECTS= $(patsubst %.cc,%.o, $(SOURCES))
GOBJECTS= $(patsubst %.cc,.debug_objs/%.o, $(SOURCES))
INCLUDE_HEADERS= $(patsubst %,$(INCLUDEDIR)/%, $(HEADERS))

.SUFFIXES: .cc $(SUFFIXES)

$(INCLUDEDIR)/hams/%.h: hams/%.h
	mkdir -p $(INCLUDEDIR)/hams
	cp $^ $(INCLUDEDIR)/hams

$(INCLUDEDIR)/model/%.h: model/%.h
	mkdir -p $(INCLUDEDIR)/model
	cp $^ $(INCLUDEDIR)/model

$(INCLUDEDIR)/%.h: %.h
	cp $^ $(INCLUDEDIR)

%.o: %.cc
	$(CCCOM) -c $(CCFLAGS) -o $@ $<

.debug_objs/%.o: %.cc
	$(CCCOM) -c $(CCGFLAGS) -o $@ $<

install: installo installg

libitensor.a: $(OBJECTS)
	ar rv libitensor.a $(OBJECTS)
	ranlib libitensor.a

libitensor-g.a: mkdebugdir $(GOBJECTS)
	ar rv libitensor-g.a $(GOBJECTS)
	ranlib libitensor-g.a

headerscopy: $(INCLUDE_HEADERS)
	cp core.h $(INCLUDEDIR)

libs: libitensor.a 
	cp libitensor.a $(LIBDIR)

installo: headerscopy libitensor.a
	cp libitensor.a $(LIBDIR)
	ranlib $(LIBDIR)/libitensor.a

installg: headerscopy libitensor-g.a
	cp libitensor-g.a $(LIBDIR)
	ranlib $(LIBDIR)/libitensor-g.a

mkdebugdir:
	mkdir -p .debug_objs

clean:	
	rm -fr *.o .debug_objs libitensor.a libitensor-g.a

DEPHEADERS=global.h real.h permutation.h index.h 
index.o: $(DEPHEADERS)
.debug_objs/index.o: $(DEPHEADERS)
DEPHEADERS+= indexset.h
indexset.o: $(DEPHEADERS)
.debug_objs/indexset.o: $(DEPHEADERS)
DEPHEADERS+= allocator.h itensor.h
itensor.o: $(DEPHEADERS)
.debug_objs/itensor.o: $(DEPHEADERS)
DEPHEADERS+= itsparse.h
itsparse.o: $(DEPHEADERS)
.debug_objs/itsparse.o: $(DEPHEADERS)
DEPHEADERS+= qn.h iqindex.h
iqindex.o: $(DEPHEADERS)
.debug_objs/iqindex.o: $(DEPHEADERS)
DEPHEADERS+= iqindexset.h
iqindexset.o: $(DEPHEADERS)
.debug_objs/iqindexset.o: $(DEPHEADERS)
DEPHEADERS+= iqtensor.h
iqtensor.o: $(DEPHEADERS)
.debug_objs/iqtensor.o: $(DEPHEADERS)
DEPHEADERS+= iqtsparse.h
iqtsparse.o: $(DEPHEADERS)
.debug_objs/iqtsparse.o: $(DEPHEADERS)
DEPHEADERS+= combiner.h condenser.h iqcombiner.h localmpo.h svdworker.h
svdworker.o: $(DEPHEADERS)
.debug_objs/svdworker.o: $(DEPHEADERS)
DEPHEADERS+= mps.h
mps.o: $(DEPHEADERS)
.debug_objs/mps.o: $(DEPHEADERS)
DEPHEADERS+= mpo.h
mpo.o: $(DEPHEADERS)
.debug_objs/mpo.o: $(DEPHEADERS)
DEPHEADERS+= DMRGObserver.h dmrg.h
dmrg.o: $(DEPHEADERS)
.debug_objs/dmrg.o: $(DEPHEADERS)
