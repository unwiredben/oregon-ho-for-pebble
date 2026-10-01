# Copyright (c) 2026 Ben Combee. SPDX-License-Identifier: MIT
"""Exercise launch, deferred idle, minute refresh, and tap on all four emulators.

Run with Pebble Tool's Python after building the current PBW. Installs into
isolated emulator flash images; writes screenshots and a validation report
in previews/animation-idle/.
"""
import json
import os
import signal
import tempfile
import time
from pathlib import Path
from uuid import UUID

from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.qemu.protocol import QemuTap
from libpebble2.protocol.apps import AppRunState, AppRunStateStart, AppRunStateStop
from pebble_tool.commands.emucontrol import send_data_to_qemu
from pebble_tool.commands.install import ToolAppInstaller
from pebble_tool.commands.screenshot import ScreenshotCommand
from pebble_tool.sdk.emulator import ManagedEmulatorTransport
from pebble_tool.sdk import add_tools_to_path, sdk_manager

from capture_store_assets import capture


class TestEmulator(ManagedEmulatorTransport):
    """Use independent ports and flash so existing emulators are preserved."""
    def __init__(self, platform, directory):
        self.directory = directory / platform
        self.directory.mkdir()
        super().__init__(platform, '4.33.1')

    def _find_ports(self):
        self.qemu_pid = self.pypkjs_pid = None
        self.qemu_port = self._choose_port()
        self.qemu_serial_port = self._choose_port()
        self.qemu_gdb_port = self._choose_port()
        self.qemu_monitor_port = self._choose_port()
        self.pypkjs_port = self._choose_port()

    def _get_spi_path(self):
        path = str(self.directory / 'flash.bin')
        self._copy_spi_image(path)
        return path

    def _save_state(self):
        # Do not replace the SDK's records for the user's emulators.
        pass


def scene(image):
    width, height = image.size
    ground = height * 66 // 100
    scale = 2 if width >= 200 else 1
    return image.crop((0, ground - 37 * scale, width, ground)).tobytes()


def panel(image):
    width, height = image.size
    return image.crop((0, height * 66 // 100 + 12, width, height)).tobytes()


def clock(image):
    width, height = image.size
    return image.crop((0, 0, width, height // 3)).tobytes()


def main():
    add_tools_to_path()
    os.environ.setdefault('PEBBLE_QEMU_PATH', str(
        Path(sdk_manager.root_path_for_sdk('4.33.1')) / 'toolchain' / 'bin' / 'qemu-pebble'))
    project = Path(__file__).resolve().parents[1]
    package = json.loads((project / 'package.json').read_text())
    app_uuid = UUID(package['pebble']['uuid'])
    output = project / 'previews' / 'animation-idle'
    watches, commands, report, transports = {}, {}, {}, []
    temporary = tempfile.TemporaryDirectory(prefix='oregon-ho-animation-')
    try:
        for platform in package['pebble']['targetPlatforms']:
            transport = TestEmulator(platform, Path(temporary.name))
            transports.append(transport)
            watch = PebbleConnection(transport)
            watch.connect()
            watch.run_async()
            watches[platform] = watch
            watch.fetch_watch_info()
            ToolAppInstaller(watch, str(project / 'build' / 'oregon-ho.pbw')).install()
            command = ScreenshotCommand()
            command.pebble = watch
            commands[platform] = command
            (output / platform).mkdir(parents=True, exist_ok=True)
            watch.send_packet(AppRunState(data=AppRunStateStop(uuid=app_uuid)))
        time.sleep(.3)
        for watch in watches.values():
            watch.send_packet(AppRunState(data=AppRunStateStart(uuid=app_uuid)))
        started = time.monotonic()

        def wait_until(seconds):
            while time.monotonic() - started < seconds:
                time.sleep(min(.5, seconds - (time.monotonic() - started)))

        def snapshots(label):
            images = {}
            for platform, command in commands.items():
                images[platform] = capture(command)
                images[platform].save(output / platform / (label + '.png'))
            print(f'{label}: {time.monotonic() - started:.1f}s', flush=True)
            return images

        wait_until(2)
        launch = snapshots('launch')
        time.sleep(.65)
        moving = snapshots('moving')
        for platform in commands:
            assert scene(launch[platform]) != scene(moving[platform]), platform + ': launch is static'
        # The first landmark is visible from 12 to 84 seconds, beyond the budget.
        wait_until(65)
        landmark = snapshots('landmark-after-minute')
        time.sleep(.65)
        landmark_moving = snapshots('landmark-still-moving')
        for platform in commands:
            assert scene(landmark[platform]) != scene(landmark_moving[platform]), platform + ': froze landmark'
        # Even a story starting near 84 seconds expires before 100 seconds.
        wait_until(100)
        idle = snapshots('idle')
        time.sleep(1)
        still = snapshots('still')
        for platform in commands:
            assert scene(idle[platform]) == scene(still[platform]), platform + ': did not stop'
        minute_seen = set()
        for _ in range(14):
            time.sleep(5)
            fresh = snapshots('minute-refresh')
            for platform in commands:
                assert scene(idle[platform]) == scene(fresh[platform]), platform + ': minute restarted scene'
                if clock(idle[platform]) != clock(fresh[platform]) and panel(idle[platform]) != panel(fresh[platform]):
                    minute_seen.add(platform)
            if len(minute_seen) == len(commands):
                break
        assert minute_seen == set(commands), 'Clock/story minute refresh missing: ' + str(set(commands) - minute_seen)
        for watch in watches.values():
            send_data_to_qemu(watch.transport, QemuTap(axis=QemuTap.Axis.X, direction=1))
        time.sleep(.7)
        tapped = snapshots('tap-restarted')
        for platform in commands:
            assert scene(idle[platform]) != scene(tapped[platform]), platform + ': tap did not restart'
            report[platform] = {'launch_animates': True, 'landmark_finishes_after_minute': True,
                                'idle_scene_frozen': True, 'idle_clock_and_story_refresh': True,
                                'tap_restarts_animation': True}
        (output / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
        print('All four emulators passed launch, landmark completion, idle minute refresh and tap restart.', flush=True)
    finally:
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
