#!/usr/bin/env python3
#
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */
#
# This script looks for problems in chromium_src.
#
# 1. Look for files in chromium_src that don't have a corresponding
#    file under src tree (or grit).
#
# 2. Look for #defines that redefine symbols that aren't present in
#    the overriden file.
#
# 3. Look for #include statements of the overriden file that have
#    incorrect number of ../ in the path.
#
# !!! This script does return false positives, but better check a
# !!! few of those than not check at all.

import argparse
import re
import os
import sys

"""
Look for potential problems in chromium_src overrides.
"""

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
CHROMIUM_SRC = os.path.join(SOURCE_ROOT, 'chromium_src')
SRC_DIR = os.path.abspath(os.path.join(SOURCE_ROOT, '..', '..', 'src'))

EXCLUDES = [
  './CPPLINT.cfg',
  '_(unit|browser)test(_mac)?.cc',
  'third_party/blink/renderer/modules/battery/navigator_batterytest.cc',
  'third_party/blink/renderer/modules/bluetooth/navigator_bluetoothtest.cc',
  'third_party/blink/renderer/modules/quota/navigator_storagetest.cc',
  'third_party/blink/renderer/modules/storage/brave_dom_window_storage.h',
  './chrome/installer/linux/common/brave-browser/chromium-browser.appdata.xml',
  './chrome/installer/linux/common/brave-browser/chromium-browser.info',
]


def do_check_includes(override_filepath):
    """
    Checks if |override_filepath| uses the correct number of ".." in the
    include statement for the original file.
    """
    with open(override_filepath, 'r') as override_file:
        override_filename = os.path.basename(override_filepath)
        override_dirpath = os.path.dirname(override_filepath)

        # Calculate the expected number of parent dirs.
        # -1 for the dir where the file is.
        # +3 to get to chromium_src->brave->src.
        expected_count = len(override_dirpath.split(os.path.sep)) - 1 + 3

        # Check relative includes go up the expected amount of steps.
        for line in override_file:
            # We're only interested in include paths for parent directories.
            regexp = r'^#include "(\.\./.*{})"'.format(override_filename)
            line_match = re.search(regexp, line)
            if not line_match:
                continue

            include_path = line_match.group(1)
            actual_count = include_path.split(os.path.sep).count('..')

            # Check actual vs expected.
            if actual_count != expected_count:
                print("ERROR: while processing {}".format(override_filepath))

                # Sanity check
                saved_cwd = os.getcwd()
                os.chdir(override_dirpath)
                if not os.path.isfile(include_path):
                    print("       File {} doesn't exist".format(include_path))
                os.chdir(saved_cwd)

                print("       Expected {} \"..\"".format(expected_count))
                print("       Found {} \"..\"".format(actual_count))
                print("-------------------------")


def do_check_defines(override_filepath, original_filepath):
    """
    Finds `#define TARGET REPLACEMENT` statements in |override_filepath| and
    attempts to find the <TARGET> in the |original_filepath|.
    """
    with open(override_filepath, 'r') as override_file:
        for line in override_file:
            line_match = re.search(r'^#define\s*(\S*)\s*(\S*)', line)
            if not line_match:
                continue
            target = line_match.group(1)
            replacement = line_match.group(2)
            if not replacement:
                continue

            # Adjust target name for BUILDFLAG_INTERNAL_*() cases.
            if target.startswith('BUILDFLAG_INTERNAL_'):
                buildflag_match = re.search(r'BUILDFLAG_INTERNAL_(\S*)\(\)',
                                            target)
                target = buildflag_match.group(1)

            # Report ERROR if target can't be found in the original file.
            with open(original_filepath, 'r') as original_file:
                if target not in original_file.read():
                    print("ERROR: Unable to find symbol {} in {}"
                          .format(target, original_filepath))
                    print("-------------------------")


def do_check_overrides(overrides_list, search_dir, check_includes=False):
    """
    Checks that each path in the passed in list |overrides_list| exists in
    the passed in directory (|search_dir|), optionally checking includes too.
    """
    print("--------------------------------------------------")
    print("Checking overrides in {} ...".format(search_dir))
    print("--------------------------------------------------")
    for override_filepath in overrides_list:
        original_filepath = os.path.join(search_dir, override_filepath)
        if not os.path.isfile(original_filepath):
            print("WARNING: No source for override {}"
                  .format(override_filepath))
            print("-------------------------")
            continue

        do_check_defines(override_filepath, original_filepath)
        if check_includes:
            do_check_includes(override_filepath)


def get_generated_builddir(build, target_os=None, target_arch=None):
    """
    Returns the path to the generated build directory for a |build|, accounting
    for the optional |target_os| and |target_arch| parameters, if present.
    """
    build_dirname = '_'.join(filter(None,
                                    [target_os, build, target_arch]))
    return os.path.join(SRC_DIR, 'out', build_dirname, 'gen')


def filter_filepaths(dir, include_regexp=None, exclude_regexp=None):
    """
    Return a list of paths pointing to the files under the directory |dir| after
    filtering the results according to the |include_regexp| and |exclude_regexp|
    regexp rules, where |include_regexp| takes precedence over |exclude_regexp|.
    """
    result = []

    if not include_regexp and not exclude_regexp:
        return result

    for dir_path, dirnames, filenames in os.walk(dir):
        for filename in filenames:
            full_path = os.path.join(dir_path, filename)
            if include_regexp and not re.search(include_regexp, full_path):
                continue
            if exclude_regexp and re.search(exclude_regexp, full_path):
                continue
            result.append(full_path.replace('{}/'.format(dir), ''))

    return result


def main(args):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build',
                        help='Build type (i.e. Component|Static|Debug|Release)',
                        nargs='?',
                        default='Component')
    parser.add_argument('--os',
                        required=False,
                        help='Target OS (e.g. android|win|linux|mac|ios)')
    parser.add_argument('--arch',
                        required=False,
                        help='Target architecture (e.g. x86|x64|arm|arm64)')
    options = parser.parse_args(args)

    gen_buildir = get_generated_builddir(options.build,
                                         options.os,
                                         options.arch)

    # Check that the required directories exist.
    for dir in [SOURCE_ROOT, gen_buildir]:
        if not os.path.isdir(dir):
            print("ERROR: {} is not a valid directory.".format(dir))
            return 1

    # Change into the chromium_src directory for convenience.
    os.chdir(CHROMIUM_SRC)

    # Check non-GRIT overrides.
    src_overrides = filter_filepaths(
        CHROMIUM_SRC,
        exclude_regexp='|'.join(EXCLUDES + ['python_modules|.*grit.*']))
    do_check_overrides(src_overrides, SRC_DIR, True)

    # Check GRIT overrides.
    grit_overrides = filter_filepaths(CHROMIUM_SRC, include_regexp='.*grit.*')
    do_check_overrides(grit_overrides, gen_buildir, False)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
