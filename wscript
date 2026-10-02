# Copyright (c) 2026 Ben Combee
# SPDX-License-Identifier: MIT
# See LICENSE in the repository root for the full license text.

#
# This file is the default set of rules to compile a Pebble application.
#
# Feel free to customize this to your needs.
#
import json
import os.path
import shutil

from waflib import Logs

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    """
    This method is used to configure your build. ctx.load(`pebble_sdk`) automatically configures
    a build for each valid platform in `targetPlatforms`. Platform-specific configuration: add your
    change after calling ctx.load('pebble_sdk') and make sure to set the correct environment first.
    Universal configuration: add your change prior to calling ctx.load('pebble_sdk').
    """
    ctx.load('pebble_sdk')


def copy_versioned_bundle(ctx):
    with open(ctx.path.find_node('package.json').abspath()) as manifest:
        package = json.load(manifest)
    source = ctx.bldnode.make_node(ctx.env.BUNDLE_NAME).abspath()
    filename = '{}-{}.pbw'.format(package['name'], package['version'])
    destination = ctx.path.make_node(filename).abspath()
    shutil.copyfile(source, destination)
    Logs.info('Copied build bundle to {}'.format(filename))


def build(ctx):
    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'), target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf,
                          bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json',
                                         'src/common/**/*.js']),
                   js_entry_file='src/pkjs/index.js')
    ctx.add_post_fun(copy_versioned_bundle)
