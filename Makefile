all: SpiceServerGLib-0.1.typelib

SPICE_SERVER_INCLUDE_DIR=$(shell pkg-config --cflags spice-server | sed -e 's/-I//g' -e 's/ /\n/g' | grep 'spice-server$$')

ssg-enum-types.h:
	glib-mkenums --symbol-prefix ssg --template ssg-enum-types.h.template $(SPICE_SERVER_INCLUDE_DIR)/spice-server.h ssg-fixup-enum-types.h \
			> ssg-enum-types.h

ssg-enum-types.c: ssg-enum-types.h
	glib-mkenums --symbol-prefix ssg --template ssg-enum-types.c.template $(SPICE_SERVER_INCLUDE_DIR)/spice-server.h ssg-fixup-enum-types.h \
			| sed -e 's/"SPICE_COMPAT_\(VERSION_[^"]*\)", "[^"]*"/"SPICE_COMPAT_\1", "\1"/g' \
			      -e 's/"spice_image_compression_t"/"ssg_spice_image_compression_t"/g' \
			      -e 's/"spice_wan_compression_t"/"ssg_spice_wan_compression_t"/g' \
			> ssg-enum-types.c

ssg-enum-types.lo: ssg-enum-types.c ssg-enum-types.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-enum-types.c -o ssg-enum-types.lo

ssg-server.lo: ssg-server.c ssg-enum-types.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-server.c -o ssg-server.lo

ssg-baseinstance.lo: ssg-baseinstance.c ssg-baseinstance.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-baseinstance.c -o ssg-baseinstance.lo

ssg-qxl-instance.lo: ssg-qxl-instance.c ssg-qxl-instance.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-qxl-instance.c -o ssg-qxl-instance.lo

ssg-keyboard-instance.lo: ssg-keyboard-instance.c ssg-keyboard-instance.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-keyboard-instance.c -o ssg-keyboard-instance.lo

ssg-tablet-instance.lo: ssg-tablet-instance.c ssg-tablet-instance.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-tablet-instance.c -o ssg-tablet-instance.lo

ssg-mouse-instance.lo: ssg-mouse-instance.c ssg-mouse-instance.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-mouse-instance.c -o ssg-mouse-instance.lo

ssg-qxl-command.lo: ssg-qxl-command.c ssg-qxl-command.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-qxl-command.c -o ssg-qxl-command.lo

ssg-qxl-draw-copy-command.lo: ssg-qxl-draw-copy-command.c ssg-qxl-draw-copy-command.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-qxl-draw-copy-command.c -o ssg-qxl-draw-copy-command.lo

ssg-qxl-cursor-set-command.lo: ssg-qxl-cursor-set-command.c ssg-qxl-cursor-set-command.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-qxl-cursor-set-command.c -o ssg-qxl-cursor-set-command.lo

ssg-qxl-cursor-move-command.lo: ssg-qxl-cursor-move-command.c ssg-qxl-cursor-move-command.h
	libtool compile gcc `pkg-config --cflags gobject-2.0 spice-server cairo` -g -c ssg-qxl-cursor-move-command.c -o ssg-qxl-cursor-move-command.lo

	

libspiceserverglib.la: ssg-server.lo ssg-baseinstance.lo ssg-qxl-instance.lo ssg-keyboard-instance.lo \
						  ssg-tablet-instance.lo ssg-mouse-instance.lo ssg-enum-types.lo ssg-qxl-command.lo \
						  ssg-qxl-draw-copy-command.lo ssg-qxl-cursor-set-command.lo ssg-qxl-cursor-move-command.lo
	libtool link gcc `pkg-config --libs gobject-2.0 spice-server cairo` -rpath /usr/local/lib ssg-server.lo \
			ssg-baseinstance.lo ssg-qxl-instance.lo ssg-keyboard-instance.lo ssg-tablet-instance.lo \
			ssg-mouse-instance.lo ssg-enum-types.lo ssg-qxl-command.lo ssg-qxl-draw-copy-command.lo \
			ssg-qxl-cursor-set-command.lo ssg-qxl-cursor-move-command.lo -o libspiceserverglib.la

SpiceServerGLib-0.1.typelib: libspiceserverglib.la
	g-ir-scanner \
			ssg-server.[ch] \
			ssg-baseinstance.[ch] \
			ssg-qxl-instance.[ch] \
			ssg-keyboard-instance.[ch] \
			ssg-tablet-instance.[ch] \
			ssg-mouse-instance.[ch] \
			ssg-qxl-command.[ch] \
			ssg-qxl-draw-copy-command.[ch] \
			ssg-enum-types.[ch] \
			ssg-qxl-cursor-set-command.[ch] \
			ssg-qxl-cursor-move-command.[ch] \
			--accept-unprefixed \
			--library=spiceserverglib \
			--cflags-begin \
			`pkg-config --cflags gobject-2.0 gio-2.0 spice-server cairo` \
			--cflags-end \
			--include=GObject-2.0 \
			--include=Gio-2.0 \
			--include=cairo-1.0 \
			--nsversion=0.1 \
			--output=SpiceServerGLib-0.1.gir \
			--symbol-prefix=ssg \
			--identifier-prefix=Ssg \
			--namespace=SpiceServerGLib \
			--warn-all
	sed -i \
			-e 's/enumeration name="spice_compat_version_t"/enumeration name="CompatVersion"/g' \
			-e 's/type name="spice_compat_version_t"/type name="CompatVersion"/g' \
			-e 's/enumeration name="ssg_spice_image_compression_t"/enumeration name="ImageCompression"/g' \
			-e 's/type name="ssg_spice_image_compression_t"/type name="ImageCompression"/g' \
			-e 's/enumeration name="ssg_spice_wan_compression_t"/enumeration name="WANCompression"/g' \
			-e 's/type name="ssg_spice_wan_compression_t"/type name="WANCompression"/g' \
			-e 's/bitfield name="spice_addr_flags_t"/bitfield name="AddrFlags"/g' \
			-e 's/type name="spice_addr_flags_t"/type name="AddrFlags"/g' \
			-e 's/enumeration name="spice_stream_video_t"/enumeration name="StreamingVideo"/g' \
			-e 's/type name="spice_stream_video_t"/type name="StreamingVideo"/g' \
			-e 's/enumeration name="spice_channel_security_t"/enumeration name="ChannelSecurity"/g' \
			-e 's/type name="spice_channel_security_t"/type name="ChannelSecurity"/g' \
			SpiceServerGLib-0.1.gir
	g-ir-compiler SpiceServerGLib-0.1.gir --output=SpiceServerGLib-0.1.typelib
		


clean:
	rm -f *.o *.gir *.typelib *.lo *.la ssg-enum-types.h ssg-enum-types.c
        