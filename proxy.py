#!/usr/bin/env python3

import argparse
import cairo
import ctypes
import gi
import re
import sys
import time
from builtins import str

assert cairo.version_info > (1,11,0)

gi.require_version('SpiceClientGLib', '2.0')
gi.require_version('SpiceServerGLib', '0.1')

from gi.repository import GLib
from gi.repository import GObject
from gi.repository import SpiceClientGLib
from gi.repository import SpiceServerGLib

from queue import Queue, Empty

MOUSE_LEFT = 1
MOUSE_MIDDLE = 2
MOUSE_RIGHT = 3
MOUSE_UP = 4
MOUSE_DOWN = 5

class SpiceProxy(object):

    def __init__(self, host, port, listen_port, overlay):
        self._mainloop = GLib.MainLoop()
        self._qxlinstance = None
        self._main_channel = None
        self._display_channel = None
        self._cursor_channel = None
        self._inputs_channel = None
        self._command_queue = Queue()
        self._cursor_set_command = None
        self._tablet_buttons_state = 0
        self._show_overlay = overlay

        # Client

        self._spice_session = SpiceClientGLib.Session(host=host, port=port)
        GObject.GObject.connect(self._spice_session, "channel-new", self._channel_new_cb)
        self._spice_session.connect()

        # Server

        self._spiceserver = SpiceServerGLib.Server(noauth=True, port=listen_port,
                                                        image_compression=SpiceServerGLib.ImageCompression.OFF,
                                                        jpeg_compression=SpiceServerGLib.WANCompression.NEVER,
                                                        zlib_glz_compression=SpiceServerGLib.WANCompression.NEVER)
        self._spiceserver.init()

        self._keyboardinstance = SpiceServerGLib.KeyboardInstance()
        self._keyboardinstance.connect("push-key", self._keyboard_push_key_cb)
        self._spiceserver.add_interface(self._keyboardinstance)

        self._tabletinstance = SpiceServerGLib.TabletInstance()
        self._tabletinstance.connect("position", self._tablet_position_cb)
        self._tabletinstance.connect("wheel", self._tablet_wheel_cb)
        self._tabletinstance.connect("buttons", self._tablet_buttons_cb)
        self._spiceserver.add_interface(self._tabletinstance)

    def run(self):
        self._mainloop.run()


    # Client Session callbacks

    def _channel_new_cb(self, session, channel):
        if type(channel) == SpiceClientGLib.MainChannel:
            self._main_channel = channel
        elif type(channel) == SpiceClientGLib.DisplayChannel:
            self._display_channel = channel
            self._display_channel.connect_after("display-primary-create", self._display_primary_create_cb)
            self._display_channel.connect_after("display-invalidate", self._display_invalidate_cb)
            channel.connect()
        elif type(channel) == SpiceClientGLib.CursorChannel:
            self._cursor_channel = channel
            self._cursor_channel.connect_after("notify::cursor", self._cursor_notify_cb)
            self._cursor_channel.connect_after("cursor-set", self._cursor_set_cb)
            channel.connect()
        elif type(channel) == SpiceClientGLib.InputsChannel:
            self._inputs_channel = channel
            channel.connect()

    def _display_primary_create_cb(self, channel, format, width, height, stride, shmid, imgdata):
        SPICE_SURFACE_FMT_32_xRGB = 32
        if not format == SPICE_SURFACE_FMT_32_xRGB:
            raise RuntimeError("Unsupported display format")

        ptype = ctypes.POINTER(ctypes.c_ubyte * stride * height)
        buffer = ptype.from_buffer(ctypes.cast(imgdata,ptype))[0]
        self._primary_surface = cairo.ImageSurface.create_for_data(buffer, cairo.FORMAT_ARGB32, width, height, stride)
        self._primary_width = width
        self._primary_height = height

        self._qxlinstance = SpiceServerGLib.QXLInstance(width=width, height=height, max_surfaces=3)
        self._qxlinstance.connect("attache-worker", self._qxl_attache_worker_cb)
        self._qxlinstance.connect("req-cmd-notification", self._qxl_req_cmd_notification_cb)
        self._qxlinstance.connect("get-command", self._qxl_get_command_cb)
        self._qxlinstance.connect("req-cursor-notification", self._qxl_req_cursor_notification_cb)
        self._qxlinstance.connect("get-cursor-command", self._qxl_get_cursor_command_cb)
        self._spiceserver.add_interface(self._qxlinstance)

        if self._show_overlay:
            surface1 = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
            self._command_queue.put(SpiceServerGLib.QXLSurfaceCreateCommand.new(1, surface1))

            surface2 = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
            self._command_queue.put(SpiceServerGLib.QXLSurfaceCreateCommand.new(2, surface2))

            draw_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
            context = cairo.Context(draw_surface)
            context.set_source_rgba (0, 0, 0, 0.20)
            context.select_font_face("Purisa", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            context.set_font_size(100)
            for y in range(0,1000,100):
                context.move_to(-y, y)
                context.show_text("The quick brown fox jumped over the lazy dog")
            draw_surface.flush()
            image = SpiceServerGLib.QXLBitmapImage.new(draw_surface);
            dc_command = SpiceServerGLib.QXLDrawCopyCommand.new(image, 0, 0, width, height, 2, 0, 0, width, height)
            self._command_queue.put(dc_command)

    def _display_invalidate_cb(self, channel, x, y, width, height):
        update_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        context = cairo.Context(update_surface)
        context.set_source_surface(self._primary_surface, -x, -y)
        context.set_operator(cairo.OPERATOR_SOURCE)
        context.rectangle(0, 0, width, height)
        context.fill()
        update_surface.flush()
        image = SpiceServerGLib.QXLBitmapImage.new(update_surface);

        commands = []
        if not self._show_overlay:
            commands.append(SpiceServerGLib.QXLDrawCopyCommand.new(image,
                                                                   0, 0, width, height,
                                                                   0,
                                                                   x, y, width, height));
        else:
            # Update surface 1 with invalidated region
            commands.append(SpiceServerGLib.QXLDrawCopyCommand.new(image,
                                                                   0, 0, width, height,
                                                                   1,
                                                                   x, y, width, height));

            # Blend overlay in surface 2 onto invalidated region
            image2 = SpiceServerGLib.QXLSurfaceImage.new(2);
            commands.append(SpiceServerGLib.QXLDrawAlphaBlendCommand.new(image2,
                                                                         x, y, width, height,
                                                                         1,
                                                                         x, y, width, height));

            # Copy blended invalidated region onto primary surface
            image3 = SpiceServerGLib.QXLSurfaceImage.new(1);
            commands.append(SpiceServerGLib.QXLDrawCopyCommand.new(image3,
                                                                   x, y, width, height,
                                                                   0,
                                                                   x, y, width, height));

        was_empty = self._command_queue.empty()
        for command in commands:
            self._command_queue.put(command)
        if was_empty and self._qxlinstance is not None:
            self._qxlinstance.wakeup()



    def _cursor_notify_cb(self, channel, cursor):
        # TODO opaque cursor structure - can't use
        pass

    def _cursor_set_cb(self, channel, width, height, hot_x, hot_y, rgba):
        ptype = ctypes.POINTER(ctypes.c_ubyte * width * height * 4)
        buffer = ptype.from_buffer(ctypes.cast(rgba,ptype))[0]
        surface = cairo.ImageSurface.create_for_data(buffer, cairo.FORMAT_ARGB32, width, height)
        # TODO mouse mode
        self._cursor_set_command = SpiceServerGLib.QXLCursorSetCommand.new(surface,hot_x, hot_y, 0, 0)
        if self._qxlinstance is not None:
            self._qxlinstance.wakeup()


    # Server QXLInstance callbacks

    def _qxl_attache_worker_cb (self, instance):
        self._spiceserver.vm_start()

    def _qxl_req_cmd_notification_cb (self, instance):
        return self._command_queue.empty()

    def _qxl_get_command_cb (self, instance):
        try:
            return self._command_queue.get(False)
        except Empty:
            return None

    def _qxl_req_cursor_notification_cb (self, instance):
        return True

    def _qxl_get_cursor_command_cb (self, instance):
        if self._cursor_set_command is not None:
            try:
                return self._cursor_set_command
            finally:
                self._cursor_set_command = None
        return None


    # Server KeyboardInstance callbacks

    def _keyboard_push_key_cb(self, instance, key):
        if key & 0x80:
            SpiceClientGLib.inputs_key_release(self._inputs_channel, key & ~0x80)
        else:
            SpiceClientGLib.inputs_key_press(self._inputs_channel, key)


    # Server TabletInstance callbacks

    def _tablet_position_cb(self, instance, x, y, buttons_state):
        SpiceClientGLib.inputs_position(self._inputs_channel, x, y, 0, buttons_state)

    def _tablet_wheel_cb(self, instance, wheel_motion, buttons_state):
        self._tablet_buttons_cb(instance, buttons_state)
        if wheel_motion < 0:
            SpiceClientGLib.inputs_button_press(self._inputs_channel, MOUSE_UP, self._tablet_buttons_state)
            SpiceClientGLib.inputs_button_release(self._inputs_channel, MOUSE_UP, self._tablet_buttons_state)
        if wheel_motion > 0:
            SpiceClientGLib.inputs_button_press(self._inputs_channel, MOUSE_DOWN, self._tablet_buttons_state)
            SpiceClientGLib.inputs_button_release(self._inputs_channel, MOUSE_DOWN, self._tablet_buttons_state)

    def _tablet_buttons_cb(self, instance, buttons_state):
        # TODO Need to swap middle and right buttons - why?
        buttons_state = (buttons_state & ~6) | (buttons_state & 2) << 1 | (buttons_state & 4) >> 1
        for button in (MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT):
            mask = 1 << (button - 1)
            if (buttons_state & mask) and not (self._tablet_buttons_state & mask):
                SpiceClientGLib.inputs_button_press(self._inputs_channel, button, mask)
                self._tablet_buttons_state = self._tablet_buttons_state | mask
            if (self._tablet_buttons_state & mask) and not (buttons_state & mask):
                SpiceClientGLib.inputs_button_release(self._inputs_channel, button, mask)
                self._tablet_buttons_state = self._tablet_buttons_state & ~mask


if __name__ == '__main__':
    try:
        def host_port_type(str, pattern=re.compile(r"[^:]*:[0-9]{1,5}")):
            if not pattern.match(str):
                raise argparse.ArgumentTypeError
            return str
        parser = argparse.ArgumentParser(description="Proxy a SPICE connection")
        parser.add_argument("host_port", type=host_port_type, metavar="host:port", help="Spice host to proxy")
        parser.add_argument("-p", type=int, default=5910, dest="listen_port", metavar="listen_port", help="Port to serve from")
        parser.add_argument("--overlay", action="store_true", help="Blend a transparent overlay onto the display")
        args = parser.parse_args()
        host, port = args.host_port.split(":")
        proxy = SpiceProxy(host=host, port=port, listen_port=args.listen_port, overlay=args.overlay)
        proxy.run()
    except KeyboardInterrupt:
        sys.exit(1)

