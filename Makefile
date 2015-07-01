 # Application Authorization Component Demonstration

NAME=twtr

# NAME OF OUR COMPONENT AND DEMO APP

NAMCMP=$(NAME)comp
NAMAPP=$(NAME)demo

# NB: our *component* is the *client* of mojito, thus the need for
# the client flags and libs.

MOJITO_CLIENT_CFLAGS = -I/usr/include/mojito -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include  
MOJITO_CLIENT_LIBS = -L/lib -lmojito-client -ldbus-glib-1 -ldbus-1

CFLAGSGLIB=`pkg-config --cflags glib-2.0`
LIBSGLIB=`pkg-config --libs glib-2.0`

CFLAGSADP=`pkg-config --cflags adpcore`
LIBSADP=`pkg-config --libs adpcore`

CFLAGSGTK=`pkg-config --cflags gtk+-2.0`
LIBSGTK=`pkg-config --libs gtk+-2.0`

CFLAGSCMP=$(CFLAGSGLIB) $(CFLAGSADP) $(MOJITO_CLIENT_CFLAGS)
LIBSCMP=$(LIBSADP) $(LIBSGLIB) $(MOJITO_CLIENT_LIBS)

CFLAGSAPP=$(CFLAGSGLIB) $(CFLAGSGTK) $(CFLAGSADP)
LIBSAPP=$(LIBSGLIB) $(LIBSGTK)

# Default to making Demo App with Shared Library

all: $(NAMAPP)

static: $(NAMAPP)sta

# Demo App with Shared Library

$(NAMAPP): lib$(NAMCMP).so $(NAMAPP).c
	gcc -o $(NAMAPP) $(NAMAPP).c $(CFLAGSAPP) $(LIBSAPP) -L. -l$(NAMCMP)


# Demo App with Static Library

$(NAMAPP)sta: lib$(NAMCMP)sta.a $(NAMAPP)sta.o
	gcc -g -o $(NAMAPP)sta $(NAMAPP)sta.o  -L. -l$(NAMCMP)sta  $(LIBSCMP) $(LIBSGTK)

$(NAMAPP)sta.o: $(NAMAPP).c
	gcc -g -Wall -o $(NAMAPP)sta.o -c $(NAMAPP).c $(CFLAGSAPP)


# Shared Library

lib$(NAMCMP).so: $(NAMCMP).o
	gcc -g -Wall -shared -Wl,-soname,lib$(NAMCMP).so.0 \
	 -o lib$(NAMCMP).so $(NAMCMP).o $(LIBSCMP)


# Static Library

lib$(NAMCMP)sta.a: $(NAMCMP)sta.o
	ar -cvq lib$(NAMCMP)sta.a $(NAMCMP)sta.o


# Object File used to make shared libraries

$(NAMCMP).o: $(NAMCMP).c
	gcc -g -Wall -fPIC -c $(NAMCMP).c $(CFLAGSCMP)


# Object File used to make static libraries TODO is this getting depended OK?

$(NAMCMP)sta.o: $(NAMCMP).c
	gcc -g -Wall -o $(NAMCMP)sta.o -c $(NAMCMP).c $(CFLAGSCMP)

clean:
	rm -f *.so* *.o *.a $(NAMAPP) $(NAMAPP)sta