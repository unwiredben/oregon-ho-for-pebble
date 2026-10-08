# Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT
"""Test settings messages and capture long-name panels on isolated emulators.

Run with Pebble Tool's Python interpreter after building. Does not alter the
user's emulator flash/state or stop unrelated emulator processes.
"""
import json
import argparse
import os
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from uuid import UUID

from libpebble2.communication import PebbleConnection
from libpebble2.protocol.apps import AppRunState, AppRunStateStart, AppRunStateStop
from libpebble2.services.appmessage import AppMessageService, CString, Uint8
from libpebble2.protocol.logs import AppLogMessage, AppLogShippingControl
from pebble_tool.commands.install import ToolAppInstaller
from pebble_tool.commands.screenshot import ScreenshotCommand
from pebble_tool.sdk import add_tools_to_path, sdk_manager

from capture_store_assets import capture
from verify_animation import TestEmulator


class SettingsEmulator(TestEmulator):
    """Isolate phone localStorage too; keep task-owned logs for diagnostics."""
    def __init__(self, platform, directory, logs):
        self.logs = logs / platform
        self.logs.mkdir(parents=True, exist_ok=True)
        super().__init__(platform, directory)

    def _get_output(self):
        return open(self.logs / 'emulator.log', 'a')

    def _spawn_pypkjs(self):
        (self.directory / 'phone').mkdir()
        layout = Path(sdk_manager.path_for_sdk(self.version)) / 'pebble' / self.platform / 'qemu' / 'layouts.json'
        command = [sys.executable, '-m', 'pypkjs', '--qemu', 'localhost:' + str(self.qemu_port),
                   '--port', str(self.pypkjs_port), '--persist', str(self.directory / 'phone'), '--debug']
        if layout.exists():
            command.extend(['--layout', str(layout)])
        with self._get_output() as log:
            process = subprocess.Popen(command, stdout=log, stderr=log, start_new_session=True)
        self.pypkjs_pid = process.pid
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            assert process.poll() is None, 'Phone simulator exited; inspect ' + str(self.logs)
            try:
                with socket.create_connection(('127.0.0.1', self.pypkjs_port), timeout=.2):
                    return
            except OSError:
                time.sleep(.1)
        raise AssertionError('Phone simulator did not start; inspect ' + str(self.logs))


def panel_bounds(image):
    width, height = image.size
    ground = height * 66 // 100
    points = [(x, y) for y in range(ground, height) for x in range(width)
              if image.getpixel((x, y)) == (255, 255, 255, 255)]
    assert points, 'No visible status panel'
    left, right = min(x for x, _ in points), max(x for x, _ in points)
    top, bottom = min(y for _, y in points), max(y for _, y in points)
    panel_width = 150 if width >= 200 else 104
    assert right - left + 1 == panel_width, 'Panel clipped horizontally'
    assert top >= ground and bottom < height - 1, 'Panel exceeds ground area'
    # The complete bottom border must survive the circular framebuffer mask.
    for x in range(left, right + 1):
        assert image.getpixel((x, bottom)) == (255, 255, 255, 255), 'Bottom border clipped'
    return [left, top, right, bottom]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', action='append', help='Test only selected target(s).')
    parser.add_argument('--bundle', type=Path, help='Install a specific PBW for comparison.')
    parser.add_argument('--launch-only', action='store_true', help='Only capture launch and first story.')
    args = parser.parse_args()
    add_tools_to_path()
    os.environ.setdefault('PEBBLE_QEMU_PATH', str(
        Path(sdk_manager.root_path_for_sdk('4.33.1')) / 'toolchain' / 'bin' / 'qemu-pebble'))
    project = Path(__file__).resolve().parents[1]
    package = json.loads((project / 'package.json').read_text())
    app_uuid = UUID(package['pebble']['uuid'])
    output = project / 'previews' / 'settings'
    watches, commands, services, transports, report = {}, {}, {}, [], {}
    temporary = tempfile.TemporaryDirectory(prefix='oregon-ho-settings-')
    try:
        for platform in args.platform or package['pebble']['targetPlatforms']:
            transport = SettingsEmulator(platform, Path(temporary.name), project / '.host' / 'settings-logs')
            transports.append(transport)
            watch = PebbleConnection(transport)
            watch.connect()
            watch.run_async()
            watches[platform] = watch
            watch.fetch_watch_info()
            watch.register_endpoint(AppLogMessage, lambda packet: print(
                'Watch log: ' + packet.filename + ': ' + packet.message, flush=True))
            watch.send_packet(AppLogShippingControl(enable=True))
            ToolAppInstaller(watch, str(args.bundle or project / 'build' / 'oregon-ho.pbw')).install()
            command = ScreenshotCommand()
            command.pebble = watch
            commands[platform] = command
            services[platform] = AppMessageService(watch)
            (output / platform).mkdir(parents=True, exist_ok=True)
            report[platform] = {}
            watch.send_packet(AppRunState(data=AppRunStateStop(uuid=app_uuid)))
            print(platform + ': installed', flush=True)

        def restart(initial=False):
            for watch in watches.values():
                watch.send_packet(AppRunState(data=AppRunStateStop(uuid=app_uuid)))
            time.sleep(.5)
            for watch in watches.values():
                watch.send_packet(AppRunState(data=AppRunStateStart(uuid=app_uuid)))
            # The first transient story starts after 24 seconds.
            if initial:
                time.sleep(2)
                snapshots('launch')
                time.sleep(23)
            else:
                time.sleep(25)

        def send(platform, names, no_guns):
            service = services[platform]
            ack = threading.Event()
            rejected = threading.Event()
            handle = service.register_handler('ack', lambda *args: ack.set())
            nack_handle = service.register_handler('nack', lambda *args: rejected.set())
            try:
                service.send_message(app_uuid, {**{i + 1: CString(name) for i, name in enumerate(names)},
                                                6: Uint8(no_guns)})
                if not ack.wait(10):
                    raise AssertionError(platform + ': settings ' +
                        ('rejected by watch' if rejected.is_set() else 'message not acknowledged'))
            finally:
                service.unregister_handler(handle)
                service.unregister_handler(nack_handle)

        def snapshots(label):
            for platform, command in commands.items():
                image = capture(command)
                image.save(output / platform / (label + '.png'))
                report[platform][label] = {'panel_bounds': panel_bounds(image)}
            print(label + ': ' + str(len(commands)) + ' panel(s) within screen bounds', flush=True)

        restart(initial=True)
        snapshots('before-settings')
        if args.launch_only:
            return
        for platform in commands:
            send(platform, ['Robin', '', '', '', ''], True)
        time.sleep(.5)
        snapshots('custom-name')
        cases = [
            ('wide-name', ['W' * 24, '', '', '', '']),
            ('long-name', ['Alexanderson Montgomery', '', '', '', '']),
            ('unicode-name', ['Élodie', '', '', '', '']),
            ('five-names', ['Robin', 'Alex', 'Sam', 'Mary', 'Ben']),
            ('saved-name', ['Persist', '', '', '', '']),
        ]
        for label, names in cases:
            for platform in commands:
                send(platform, names, True)
            time.sleep(.5)
            snapshots(label)
        restart()
        snapshots('after-restart')
        for platform in commands:
            send(platform, ['', '', '', '', ''], False)
        time.sleep(.5)
        snapshots('defaults-restored')
        (output / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
        print('Settings delivery, restart and rendering captures completed on ' +
              ', '.join(commands) + '.', flush=True)
    finally:
        for service in services.values():
            service.shutdown()
        for watch in watches.values():
            watch.transport.ws.close()
        for transport in transports:
            for pid in (transport.pypkjs_pid, transport.qemu_pid):
                if pid:
                    try:
                        os.kill(pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
        temporary.cleanup()


if __name__ == '__main__':
    main()
