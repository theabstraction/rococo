## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = decoder_example encoder_example chaining_example\
		vorbisfile_example seeking_example

EXTRA_DIST = frameview.pl

AM_CPPFLAGS = -I$(top_srcdir)/include @OGG_CFLAGS@

# uncomment to build static executables from the example code
#LDFLAGS = -all-static 

decoder_example_SOURCES = decoder_example.c
decoder_example_LDADD = $(top_builddir)/lib/libvorbis.la @VORBIS_LIBS@ @OGG_LIBS@

encoder_example_SOURCES = encoder_example.c
encoder_example_LDADD = $(top_builddir)/lib/libvorbisenc.la $(top_builddir)/lib/libvorbis.la @VORBIS_LIBS@ @OGG_LIBS@

chaining_example_SOURCES = chaining_example.c
chaining_example_LDADD = $(top_builddir)/lib/libvorbisfile.la $(top_builddir)/lib/libvorbis.la @VORBIS_LIBS@ @OGG_LIBS@

vorbisfile_example_SOURCES = vorbisfile_example.c
vorbisfile_example_LDADD = $(top_builddir)/lib/libvorbisfile.la $(top_builddir)/lib/libvorbis.la @VORBIS_LIBS@ @OGG_LIBS@

seeking_example_SOURCES = seeking_example.c
seeking_example_LDADD = $(top_builddir)/lib/libvorbisfile.la $(top_builddir)/lib/libvorbis.la @VORBIS_LIBS@ @OGG_LIBS@

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
