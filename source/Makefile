# -------------------------------------------------------------------------
#
#  Copyright (C) 2003-2018 Fons Adriaensen <fons@linuxaudio.org>
#    
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# -------------------------------------------------------------------------


SUFFIX := $(shell uname -m | sed -e 's/^unknown/$//' -e 's/^i.86/$//' -e 's/^x86_64/$/64/')
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib$(SUFFIX)

VERSION = 0.9.9
CPPFLAGS += -MMD -MP -DVERSION=\"$(VERSION)\" -DLIBDIR=\"$(LIBDIR)\"
CXXFLAGS += -O2 -Wall
CXXFLAGS += -march=native


all:	aeolus aeolus_x11.so aeolus_txt.so


AEOLUS_O =	main.o audio.o model.o slave.o imidi.o addsynth.o scales.o \
		reverb.o asection.o division.o rankwave.o rngen.o exp2ap.o lfqueue.o
aeolus:	LDLIBS += -lzita-alsa-pcmi -lclthreads -ljack -lasound -lpthread -ldl -lrt
aeolus: LDFLAGS += -L$(LIBDIR)
aeolus:	$(AEOLUS_O)
	$(CXX) $(LDFLAGS) -o $@ $(AEOLUS_O) $(LDLIBS)
addsynth.o:	CPPFLAGS += -fPIC -D_REENTRANT
$(AEOLUS_O):
-include $(AEOLUS_O:%.o=%.d)


XIFACE_O =	styles.o mainwin.o midiwin.o audiowin.o instrwin.o editwin.o \
	midimatrix.o multislider.o functionwin.o xiface.o addsynth.o
aeolus_x11.so:	CPPFLAGS += -D_REENTRANT
aeolus_x11.so:	CPPFLAGS += $(shell pkg-config --cflags freetype2)
aeolus_x11.so:	CXXFLAGS += -shared -fPIC
aeolus_x11.so:	LDFLAGS += -shared
aeolus_x11.so:	LDLIBS += -lclthreads -lclxclient -lpthread -lXft -lX11
aeolus_x11.so: $(XIFACE_O) $(LIBCLX)
	$(CXX) $(LDFLAGS) -o $@ $(XIFACE_O) $(LDLIBS)

$(XIFACE_O):
-include $(XIFACE_O:%.o=%.d)


TIFACE_O =	tiface.o
aeolus_txt.so:	CPPFLAGS += -D_REENTRANT
aeolus_txt.so:	CXXFLAGS += -shared -fPIC
aeolus_txt.so:	LDFLAGS += -shared
aeolus_txt.so:	LDLIBS += -lclthreads -lpthread -lreadline
aeolus_txt.so: $(TIFACE_O)
	$(CXX) $(LDFLAGS) -o $@ $(TIFACE_O) $(LDLIBS)

$(TIFACE_O):
-include $(TIFACE_O:%.o=%.d)


install:	aeolus aeolus_x11.so aeolus_txt.so 
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 aeolus $(DESTDIR)$(BINDIR)
	install -m 755 aeolus_x11.so $(DESTDIR)$(LIBDIR)
	install -m 755 aeolus_txt.so $(DESTDIR)$(LIBDIR)
	ldconfig $(PREFIX)/$(LIBDIR)


clean:
	/bin/rm -f *~ *.o *.d *.a *.so aeolus

