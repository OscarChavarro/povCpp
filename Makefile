#===========================================================================
CCO=g++ -ansi -Wall -pedantic -O3 -I./src
#CCO=g++ -ansi -Wall -pedantic -g -pg -I./src
LIBTOOL=g++ -O3
#LIBTOOL=g++ -g -pg

INCLUDES=./src/common/config.h ./src/common/frame.h ./src/common/povproto.h ./src/media/texture.h ./src/common/vector.h

OBJS=./tmp/colour.o ./tmp/matrices.o ./tmp/ray.o ./tmp/prioq.o ./tmp/texture.o \
./tmp/txtbump.o ./tmp/txtcolor.o ./tmp/txtmap.o ./tmp/txttest.o ./tmp/vect.o \
./tmp/bezier.o ./tmp/blob.o ./tmp/boxes.o ./tmp/csg.o ./tmp/hfield.o \
./tmp/objects.o ./tmp/planes.o ./tmp/light.o ./tmp/poly.o ./tmp/quadrics.o \
./tmp/spheres.o ./tmp/triangle.o ./tmp/viewpnt.o ./tmp/dump.o ./tmp/gif.o \
./tmp/gifdecod.o ./tmp/iff.o ./tmp/parse.o ./tmp/raw.o ./tmp/targa.o \
./tmp/tokenize.o ./tmp/lighting.o ./tmp/render.o ./tmp/unix.o ./tmp/povray.o

#===========================================================================
all: ./bin/povray

#===========================================================================
./bin/povray: $(OBJS)
	$(LIBTOOL) -o ./bin/povray $(OBJS) -lm
	strip ./bin/povray

#===========================================================================

./tmp/colour.o: ./src/common/colour.cpp $(INCLUDES)
	$(CCO) -o ./tmp/colour.o -c ./src/common/colour.cpp

./tmp/matrices.o: ./src/common/matrices.cpp $(INCLUDES)
	$(CCO) -o ./tmp/matrices.o -c ./src/common/matrices.cpp

./tmp/ray.o: ./src/common/ray.cpp $(INCLUDES)
	$(CCO) -o ./tmp/ray.o -c ./src/common/ray.cpp

#---------------------------------------------------------------------------
./tmp/texture.o: ./src/media/texture.cpp $(INCLUDES)
	$(CCO) -o ./tmp/texture.o -c ./src/media/texture.cpp

./tmp/txtbump.o: ./src/media/txtbump.cpp $(INCLUDES)
	$(CCO) -o ./tmp/txtbump.o -c ./src/media/txtbump.cpp

./tmp/txtcolor.o: ./src/media/txtcolor.cpp $(INCLUDES)
	$(CCO) -o ./tmp/txtcolor.o -c ./src/media/txtcolor.cpp

./tmp/txtmap.o: ./src/media/txtmap.cpp $(INCLUDES)
	$(CCO) -o ./tmp/txtmap.o -c ./src/media/txtmap.cpp

./tmp/txttest.o: ./src/media/txttest.cpp $(INCLUDES)
	$(CCO) -o ./tmp/txttest.o -c ./src/media/txttest.cpp

./tmp/vect.o: ./src/media/vect.cpp $(INCLUDES)
	$(CCO) -o ./tmp/vect.o -c ./src/media/vect.cpp

#---------------------------------------------------------------------------

./tmp/prioq.o: ./src/geom/prioq.cpp $(INCLUDES)
	$(CCO) -o ./tmp/prioq.o -c ./src/geom/prioq.cpp

./tmp/bezier.o: ./src/geom/bezier.cpp $(INCLUDES)
	$(CCO) -o ./tmp/bezier.o -c ./src/geom/bezier.cpp

./tmp/blob.o: ./src/geom/blob.cpp $(INCLUDES)
	$(CCO) -o ./tmp/blob.o -c ./src/geom/blob.cpp

./tmp/boxes.o: ./src/geom/boxes.cpp $(INCLUDES)
	$(CCO) -o ./tmp/boxes.o -c ./src/geom/boxes.cpp

./tmp/csg.o: ./src/geom/csg.cpp $(INCLUDES)
	$(CCO) -o ./tmp/csg.o -c ./src/geom/csg.cpp

./tmp/hfield.o: ./src/geom/hfield.cpp $(INCLUDES)
	$(CCO) -o ./tmp/hfield.o -c ./src/geom/hfield.cpp

./tmp/objects.o: ./src/geom/objects.cpp $(INCLUDES)
	$(CCO) -o ./tmp/objects.o -c ./src/geom/objects.cpp

./tmp/planes.o: ./src/geom/planes.cpp $(INCLUDES)
	$(CCO) -o ./tmp/planes.o -c ./src/geom/planes.cpp

./tmp/light.o: ./src/geom/light.cpp $(INCLUDES)
	$(CCO) -o ./tmp/light.o -c ./src/geom/light.cpp

./tmp/poly.o: ./src/geom/poly.cpp $(INCLUDES)
	$(CCO) -o ./tmp/poly.o -c ./src/geom/poly.cpp

./tmp/quadrics.o: ./src/geom/quadrics.cpp $(INCLUDES)
	$(CCO) -o ./tmp/quadrics.o -c ./src/geom/quadrics.cpp

./tmp/spheres.o: ./src/geom/spheres.cpp $(INCLUDES)
	$(CCO) -o ./tmp/spheres.o -c ./src/geom/spheres.cpp

./tmp/triangle.o: ./src/geom/triangle.cpp $(INCLUDES)
	$(CCO) -o ./tmp/triangle.o -c ./src/geom/triangle.cpp

./tmp/viewpnt.o: ./src/geom/viewpnt.cpp $(INCLUDES)
	$(CCO) -o ./tmp/viewpnt.o -c ./src/geom/viewpnt.cpp

#---------------------------------------------------------------------------

./tmp/dump.o: ./src/io/dump.cpp $(INCLUDES)
	$(CCO) -o ./tmp/dump.o -c ./src/io/dump.cpp

./tmp/gif.o: ./src/io/gif.cpp $(INCLUDES)
	$(CCO) -o ./tmp/gif.o -c ./src/io/gif.cpp

./tmp/gifdecod.o: ./src/io/gifdecod.cpp $(INCLUDES)
	$(CCO) -o ./tmp/gifdecod.o -c ./src/io/gifdecod.cpp

./tmp/iff.o: ./src/io/iff.cpp $(INCLUDES)
	$(CCO) -o ./tmp/iff.o -c ./src/io/iff.cpp

./tmp/parse.o: ./src/io/parse.cpp $(INCLUDES)
	$(CCO) -o ./tmp/parse.o -c ./src/io/parse.cpp

./tmp/raw.o: ./src/io/raw.cpp $(INCLUDES)
	$(CCO) -o ./tmp/raw.o -c ./src/io/raw.cpp

./tmp/targa.o: ./src/io/targa.cpp $(INCLUDES)
	$(CCO) -o ./tmp/targa.o -c ./src/io/targa.cpp

./tmp/tokenize.o: ./src/io/tokenize.cpp $(INCLUDES)
	$(CCO) -o ./tmp/tokenize.o -c ./src/io/tokenize.cpp

#---------------------------------------------------------------------------

./tmp/lighting.o: ./src/render/lighting.cpp $(INCLUDES)
	$(CCO) -o ./tmp/lighting.o -c ./src/render/lighting.cpp

./tmp/render.o: ./src/render/render.cpp $(INCLUDES)
	$(CCO) -o ./tmp/render.o -c ./src/render/render.cpp

#---------------------------------------------------------------------------

./tmp/povray.o: ./src/app/povray.cpp $(INCLUDES)
	$(CCO) -o ./tmp/povray.o -c ./src/app/povray.cpp

./tmp/unix.o: ./src/app/unix.cpp $(INCLUDES)
	$(CCO) -o ./tmp/unix.o -c ./src/app/unix.cpp

#===========================================================================

clean:
	@rm -rf `find . -name "*~"` `find .  -name "*.tga"` ./tmp/* ./bin/povray

#===========================================================================
#= EOF                                                                     =
#===========================================================================
