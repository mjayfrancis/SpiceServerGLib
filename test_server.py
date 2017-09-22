#!/usr/bin/env python3

import asyncio
import cairo
import ctypes
import gi
import queue
import sys
import threading
import time
import types
import unittest

assert cairo.version_info > (1,11,0)

gi.require_version('SpiceClientGLib', '2.0')
gi.require_version('SpiceServerGLib', '0.1')

from gi.repository import GLib
from gi.repository import GObject
from gi.repository import SpiceClientGLib
from gi.repository import SpiceServerGLib

from collections import defaultdict
from functools import wraps
from unittest import TestSuite

# Combined asyncio / GLib main loop thread
class MainLoopThread(threading.Thread):

    def __init__(self):
        super(MainLoopThread, self).__init__()
        self.asyncio_event_loop = asyncio.get_event_loop()
        self.glib_context = GLib.MainContext.default()
        self.running = True
        self.start()

    def run(self):
        async def asyncio_glib_iteration():
            last_progress_time = time.time()
            while self.running:
                if (self.glib_context.iteration(False)):
                    last_progress_time = time.time()
                # Don't burn CPU if we get stuck for some reason
                delay = (0,0.1)[time.time() - last_progress_time > 0.5]
                await asyncio.sleep(delay)
        asyncio.set_event_loop(self.asyncio_event_loop)
        self.asyncio_event_loop.run_until_complete(asyncio_glib_iteration())

    def do_in_main_loop(self, func):
        future = asyncio.run_coroutine_threadsafe(func, self.asyncio_event_loop)
        future.result()

    def stop(self):
        self.running = False
        self.join()


class ThreadsafeAsyncEvent(asyncio.Event):

    def set(self):
        self._loop.call_soon_threadsafe(super().set)

    def clear(self):
        self._loop.call_soon_threadsafe(super().clear)


class ServerHelper(SpiceServerGLib.Server):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.init()
        self._command_queue = queue.Queue()
        self._handler_ids = []
        self._qxlinstance = None

    def _attache_worker_cb (self, instance):
        self.vm_start()

    def _req_cmd_notification_cb (self, instance):
        return self._command_queue.empty()

    def _get_command_cb (self, instance):
        try:
            command = self._command_queue.get(False)
            return command
        except queue.Empty:
            return None

    def add_qxl_instance (self, width, height):
        self._qxlinstance = SpiceServerGLib.QXLInstance(width=width, height=height)
        self._qxlinstance.connect("attache-worker", self._attache_worker_cb)
        self._qxlinstance.connect("req-cmd-notification", self._req_cmd_notification_cb)
        self._handler_ids.append(self._qxlinstance.connect("get-command", self._get_command_cb))
        self.add_interface(self._qxlinstance)

    def queue_qxl_command (self, command):
        self._command_queue.put(command)
        self._qxlinstance.wakeup()

    def disconnect_all(self):
        if (self._qxlinstance is not None):
            map(self._qxlinstance.disconnect,self._handler_ids)


class ClientHelper(SpiceClientGLib.Session):

    def __init__(self, mainloopthread, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._mainloopthread = mainloopthread
        GObject.GObject.connect(self, "channel-new", self._channel_new_cb)
        self._channel_callbacks = {
            SpiceClientGLib.MainChannel: self._main_channel_new,
            SpiceClientGLib.DisplayChannel: self._display_channel_new,
            SpiceClientGLib.InputsChannel: self._inputs_channel_new,
            SpiceClientGLib.PortChannel: self._port_channel_new
        }
        self._channels = {
            SpiceClientGLib.MainChannel: [],
            SpiceClientGLib.DisplayChannel: [],
            SpiceClientGLib.InputsChannel: [],
            SpiceClientGLib.PortChannel: []
        }
        future_dict_factory = lambda: defaultdict(asyncio.Future)
        self._channel_opened_futures = defaultdict(future_dict_factory)
        self._channel_closed_futures = defaultdict(future_dict_factory)
        self._invalidate_queue = asyncio.Queue()
        self._display_mark_event = asyncio.Event()

    def _channel_new_cb(self, session, channel):
        if type(channel) in self._channels:
            self._channels[type(channel)].append(channel)
        if type(channel) in self._channel_callbacks:
            self._channel_callbacks[type(channel)](channel)
        channel.connect_after("channel-event", self._all_channels_event_cb)

    def _all_channels_event_cb(self, channel, event):
        ind = self._channels[type(channel)].index(channel)
        if event == SpiceClientGLib.ChannelEvent.OPENED:
            future = self._channel_opened_futures[type(channel)][ind]
            future.set_result(channel)
        if event == SpiceClientGLib.ChannelEvent.CLOSED:
            future = self._channel_closed_futures[type(channel)][ind]
            future.set_result(channel)

    def _main_channel_new(self, channel):
        pass

    def _display_primary_create_cb(self, channel, format, width, height, stride, shmid, imgdata):
        pass

    def _display_primary_destroy_cb(self, channel):
        pass

    def _display_invalidate_cb(self, channel, x, y, width, height):
        self._invalidate_queue.put_nowait([x,y,width,height]),

    def _display_mark_cb(self,channel,mark):
        self._display_mark_event.set()

    def _display_channel_new(self, channel):
        channel.connect_after("display-primary-create", self._display_primary_create_cb)
        channel.connect_after("display-primary-destroy", self._display_primary_destroy_cb)
        channel.connect_after("display-invalidate", self._display_invalidate_cb)
        channel.connect_after("display-mark", self._display_mark_cb)
        channel.connect()

    def _inputs_channel_new(self,channel):
        channel.connect()

    def _port_channel_new(self,channel):
        channel.connect()

    def connect(self):
        result = super().connect()
        if result == False:
            raise RuntimeError("Client failed to connect")

    def wait_channel_opened(self,channel_type,number=0):
        return self._channel_opened_futures[channel_type][number]

    def wait_channel_closed(self,channel_type,number=0):
        return self._channel_closed_futures[channel_type][number]

    def wait_invalidate(self):
        return self._invalidate_queue.get()

    def wait_display_mark(self):
        return self._display_mark_event.wait()


class KeyboardHelper(SpiceServerGLib.KeyboardInstance):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        GObject.GObject.connect(self, "push-key", self._push_key_cb)
        self._key_queue = asyncio.Queue()

    def _push_key_cb(self, instance, key):
        asyncio.run_coroutine_threadsafe(
            self._key_queue.put(key),
            asyncio.get_event_loop()
        )

    def wait_key(self):
        return self._key_queue.get()


class TabletHelper(SpiceServerGLib.TabletInstance):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        GObject.GObject.connect(self, "set-logical-size", self._tablet_set_logical_size_cb)
        GObject.GObject.connect(self, "position", self._tablet_position_cb)
        GObject.GObject.connect(self, "wheel", self._tablet_wheel_cb)
        GObject.GObject.connect(self, "buttons", self._tablet_buttons_cb)
        self._tablet_queue = asyncio.Queue()

    def _tablet_set_logical_size_cb(self, instance, width, height):
        # TODO
        pass

    def _tablet_position_cb(self, instance, x, y, buttons_state):
        asyncio.run_coroutine_threadsafe(
            self._tablet_queue.put(('position', {'x': x, 'y': y, 'buttons_state': buttons_state})),
            asyncio.get_event_loop()
        )

    def _tablet_wheel_cb(self, instance, wheel_motion, buttons_state):
        asyncio.run_coroutine_threadsafe(
        self._tablet_queue.put_nowait(('wheel', {'wheel_motion': wheel_motion, 'buttons_state': buttons_state})),
            asyncio.get_event_loop()
        )

    def _tablet_buttons_cb(self, instance, buttons_state):
        asyncio.run_coroutine_threadsafe(
            self._tablet_queue.put_nowait(('buttons', {'buttons_state': buttons_state})),
            asyncio.get_event_loop()
        )

    def wait_action(self):
        return self._tablet_queue.get()


class ServerTestCaseBase(unittest.TestCase):

    _default_server_args = {
        "noauth":True,
        "image_compression":SpiceServerGLib.ImageCompression.OFF,
        "port":5913
    }
    _default_client_args = {
        "host":"localhost",
        "port":5913
    }

    @classmethod
    def setUpClass(_class):
        _class.mainloopthread = MainLoopThread()

    @classmethod
    def tearDownClass(_class):
        _class.mainloopthread.stop()

    def __init__(self, *args, **kwargs):
        def makewrapper(inner):
            @wraps(inner)
            def wrapper(self,*args,**kwargs):
                server_args = self.__class__._default_server_args.copy()
                server_args.update(getattr(inner,"_spice_server_args",{}))
                client_args = self.__class__._default_client_args.copy()
                client_args.update(getattr(inner,"_spice_client_args",{}))
                try:
                    self.__class__.mainloopthread.do_in_main_loop(self._joint_init(server_args,client_args))
                    self.__class__.mainloopthread.do_in_main_loop(inner(self,*args,**kwargs))
                    self.__class__.mainloopthread.do_in_main_loop(self._client_stop())
                except:
                    raise
                finally:
                    self.__class__.mainloopthread.do_in_main_loop(self._server_stop())
            return wrapper

        if not hasattr (self.__class__, "__tests_decorated"):
            for m in filter(
                lambda f: f.startswith("test") and callable(getattr(self.__class__, f)),
                dir(self.__class__)
            ):
                setattr(self.__class__, m, makewrapper(getattr(self.__class__, m)))
            setattr(self.__class__, "__tests_decorated", True)

        super().__init__(*args, **kwargs)

    async def _joint_init(self,server_args,client_args):
        self.client = None
        self.server = None
        self.server = ServerHelper(**server_args)
        self.server.init()
        self.client = ClientHelper(self.mainloopthread,**client_args)

    async def _client_stop(self):
        await self.client.wait_channel_opened(SpiceClientGLib.InputsChannel)
        self.client.disconnect()
        await self.client.wait_channel_closed(SpiceClientGLib.MainChannel)

    async def _server_stop(self):
        if self.server is not None:
            self.server.disconnect_all()
            self.server.destroy()


def spiceServerArgs(**kwargs):
    def decorator(func):
        setattr(func, "_spice_server_args", kwargs)
        return func
    return decorator


def spiceClientArgs(**kwargs):
    def decorator(func):
        setattr(func, "_spice_client_args", kwargs)
        return func
    return decorator


class MyTest(ServerTestCaseBase):

    @spiceServerArgs(name="myserver")
    async def testName(self):
        # Given
        event = asyncio.Event()
        GObject.GObject.connect(self.client, "notify::name", lambda a,b: event.set())
        self.client.connect()
        await event.wait()

        # Then
        self.assertEqual("myserver", self.client.props.name)

    async def testInvalidate(self):
        # Given
        self.server.add_qxl_instance(200, 200)
        self.client.connect()
        inval = await self.client.wait_invalidate() # invalidate event for whole display

        # When
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 52, 53)
        context = cairo.Context(surface)
        context.set_source_rgb(1,0,0)
        context.rectangle(0,0,52,53)
        context.fill()
        surface.flush()
        image = SpiceServerGLib.QXLBitmapImage.new(surface)
        command = SpiceServerGLib.QXLDrawCopyCommand.new(image, 0, 0, 52, 53, 0, 50, 51, 52, 53);
        self.server.queue_qxl_command(command)

        # Then
        inval = await self.client.wait_invalidate()
        self.assertEqual([50,51,52,53], inval)
        # TODO Capture and compare surface content

    async def testKeyboard(self):
        # Given
        keyboard_instance = KeyboardHelper()
        self.server.add_interface(keyboard_instance)
        self.client.connect()
        inputs_channel = await self.client.wait_channel_opened(SpiceClientGLib.InputsChannel)

        # When
        inputs_channel.key_press(0x20)
        key = await keyboard_instance.wait_key()

        # Then
        self.assertEqual(key, 0x20)

    async def testTablet(self):
        # Given
        self.server.add_qxl_instance(200, 200)
        tablet_instance = TabletHelper()
        self.server.add_interface(tablet_instance)
        self.client.connect()
        await self.client.wait_channel_opened(SpiceClientGLib.DisplayChannel)
        inputs_channel = await self.client.wait_channel_opened(SpiceClientGLib.InputsChannel)

        # When
        inputs_channel.position(10,20,0,1)
        action = await tablet_instance.wait_action()

        # Then
        self.assertEqual(action, ('position', {'x': 10, 'y': 20, 'buttons_state': 1}))

    async def testCharDeviceName(self):
        # Given
        event = asyncio.Event()
        char_device_instance = SpiceServerGLib.CharDeviceInstance(subtype="port",portname="test.port")
        self.server.add_interface(char_device_instance)
        self.client.connect()
        port_channel = await self.client.wait_channel_opened(SpiceClientGLib.PortChannel)
        GObject.GObject.connect(port_channel, "notify::port-name", lambda a,b: event.set())
        await event.wait()

        # Then
        self.assertEqual(port_channel.props.port_name, "test.port")

    async def testCharDeviceRead(self):
        # Given
        char_device_instance = SpiceServerGLib.CharDeviceInstance(subtype="port",portname="test.port")
        self.server.add_interface(char_device_instance)

        read_data = GLib.Bytes.new(b"abcde")
        def char_device_read_cb(instance, len):
            nonlocal read_data
            to_return = read_data
            read_data = None
            return to_return
        char_device_instance.connect ("read", char_device_read_cb)

        self.client.connect()
        port_channel = await self.client.wait_channel_opened(SpiceClientGLib.PortChannel)
        opened_event = asyncio.Event()
        GObject.GObject.connect(port_channel, "notify::port-opened", lambda a,b: opened_event.set())
        await opened_event.wait()

        read_event = asyncio.Event()
        received_data = None
        def channel_port_data_cb(channel, data, size):
            nonlocal received_data
            ptype = ctypes.POINTER(ctypes.c_ubyte * size)
            buffer = ptype.from_buffer(ctypes.cast(data,ptype))[0]
            received_data = bytearray(buffer)
            read_event.set()
        GObject.GObject.connect(port_channel,"port-data",channel_port_data_cb)

        char_device_instance.wakeup()
        await read_event.wait()

        # Then
        self.assertEqual(b"abcde", received_data)

    async def testCharDeviceWrite(self):
        # Given
        char_device_instance = SpiceServerGLib.CharDeviceInstance(subtype="port",portname="test.port")
        self.server.add_interface(char_device_instance)

        write_event = asyncio.Event()
        written_data = None
        def char_device_write_cb(instance, data):
            nonlocal written_data
            written_data = data
            write_event.set()
            return written_data.get_size()
        char_device_instance.connect ("write", char_device_write_cb)

        self.client.connect()
        port_channel = await self.client.wait_channel_opened(SpiceClientGLib.PortChannel)
        opened_event = asyncio.Event()
        GObject.GObject.connect(port_channel, "notify::port-opened", lambda a,b: opened_event.set())
        char_device_instance.port_event(SpiceServerGLib.PortEvent.OPENED)
        await opened_event.wait()

        # When
        port_channel.write_async(b"efghi", None, lambda s,r,u: port_channel.write_finish(r), None)
        await write_event.wait()

        # Then
        self.assertEqual (b"efghi", written_data.get_data())



# XXX Temporary: patch SpiceClientGLib.PortChannel if method-name fix has not been applied
if not hasattr(SpiceClientGLib.PortChannel, "event"):
    SpiceClientGLib.PortChannel.event = lambda self,a: SpiceClientGLib.port_event(self,a)
    SpiceClientGLib.PortChannel.write_async = lambda self,a,b,c,d: SpiceClientGLib.port_write_async(self,a,b,c,d)
    SpiceClientGLib.PortChannel.write_finish = lambda self,a: SpiceClientGLib.port_write_finish(self,a)
if not hasattr(SpiceClientGLib.InputsChannel, "position"):
    SpiceClientGLib.InputsChannel.position = lambda self,a,b,c,d: SpiceClientGLib.inputs_position(self,a,b,c,d)
    SpiceClientGLib.InputsChannel.key_press = lambda self,a: SpiceClientGLib.inputs_key_press(self,a)

if __name__ == "__main__":
        unittest.main()
