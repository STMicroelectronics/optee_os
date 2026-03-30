#!/usr/bin/env python3
#
# Arm SCP/MCP Software
# Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import os
import signal
import subprocess
import sys

# Code quality and build checks
import check_build
import check_copyright
import check_doc
import check_spacing
import check_tabs
import check_pycodestyle
import check_style
import check_utest
import check_api

from product import Product, Build, Parameter
from typing import List, Tuple
from utils import Results, banner

#
# Default products build configuration file
#
PRODUCTS_BUILD_FILE_DEFAULT = \
    './tools/config/check_build/default_products_build.yml'

#
# Default build output directory
#
BUILD_OUTPUT_DIR_DEFAULT = '/tmp/scp/build'

STAGE_CHOICES = ["all", "code-quality", "unit-testing", "build"]

STAGE_CHECKS = {
    "code-quality": [
        check_copyright,
        check_spacing,
        check_tabs,
        check_style,
        check_pycodestyle,
        check_doc,
        check_api,
    ],
    "unit-testing": [
        (check_utest, (False, 'fwk')),
        (check_utest, (False, 'mod')),
        (check_utest, (False, 'prod')),
    ],
}


def check_argument(results: Results, check, start_ref=None) -> None:
    if isinstance(check, tuple):
        func, args = check
        # Check if this function accepts start_ref
        if func in [check_style, check_copyright]:
            if start_ref is not None:
                result = func.run(*args, commit_hash=start_ref)
            else:
                result = func.run(*args)
        else:
            result = func.run(*args)
        test_name = f"{func.__name__.split('_')[-1]} {args[1]}"
    else:
        if check in [check_style, check_copyright]:
            if start_ref is not None:
                result = check.run(commit_hash=start_ref)
            else:
                result = check.run()
        else:
            result = check.run()
        test_name = check.__name__.split('_')[-1]
    results.append((f"Check {test_name}", 0 if result else 1))


def run_stage(name: str, checks: list, start_ref: str = None) -> Results:
    banner(name)
    results = Results()
    for check in checks:
        check_argument(results, check, start_ref)
    return results


def print_results(results: List[Tuple[str, int]]) -> Tuple[int, int]:
    banner('Tests summary')
    total_success = 0
    for result in results:
        if result[1] == 0:
            total_success += 1
            verbose_result = "Success"
        else:
            verbose_result = "Failed"
        print("{}: {}".format(result[0], verbose_result))

    assert total_success <= len(results)
    return (total_success, len(results))


def analyze_results(success: int, total: int) -> int:
    print("{} / {} passed ({}% pass rate)".format(success, total,
                                                  int(success * 100 / total)))
    return 1 if success < total else 0


def main(stage: str, config_file: str, ignore_errors: bool, log_level: str,
         output_path: str, start_ref: str = None):
    results = Results()

    if stage == "all":
        for stage_name, checks in STAGE_CHECKS.items():
            results.extend(run_stage(stage_name, checks, start_ref))
    elif stage in STAGE_CHECKS:
        results.extend(run_stage(stage, STAGE_CHECKS[stage], start_ref))

    if not ignore_errors and results.errors:
        print('Errors detected! Excecution stopped')
        return analyze_results(*print_results(results))

    if stage in ("all", "build"):
        banner('Test building products')
        results.extend(check_build.build_products(config_file=config_file,
                                                  ignore_errors=ignore_errors,
                                                  log_level=log_level,
                                                  products=[],
                                                  output_path=output_path))

    return analyze_results(*print_results(results))


def parse_args():
    parser = argparse.ArgumentParser(
        description='Perform basic checks to SCP-Firmware and build for all \
                     supported platforms, modes and compilers.')

    parser.add_argument(
        "-s", "--stage",
        choices=STAGE_CHOICES,
        default="all",
        help="Stage to run (default: all)"
    )

    parser.add_argument('-c', '--config-file', dest='config_file',
                        required=False, default=PRODUCTS_BUILD_FILE_DEFAULT,
                        type=str, action='store', help=f'Products build \
                        configuration file, if it is not provided, the \
                        default location is {PRODUCTS_BUILD_FILE_DEFAULT}')

    parser.add_argument('-i', '--ignore-errors', dest='ignore_errors',
                        required=False, default=False, action='store_true',
                        help='Ignore errors and continue testing.')

    parser.add_argument('-ll', '--log-level', dest='log_level',
                        required=False, default="", type=str,
                        action='store', help='Build every product with the \
                        specified log level.')

    parser.add_argument('-bod', '--build-output-dir', dest='output_path',
                        required=False, default=BUILD_OUTPUT_DIR_DEFAULT,
                        type=str, action='store', help='Parent directory of \
                        the "build-output" directory, the one were the build \
                        logs will be stored in.\nIf bod is not given, the \
                        default location is /tmp/scp/ build-output')

    parser.add_argument('--start-ref', dest='start_ref',
                        required=False, default=None,
                        type=str,
                        help='Git reference (commit hash) \
                        to start checking from.')

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    sys.exit(main(args.stage, args.config_file, args.ignore_errors,
                  args.log_level, args.output_path, args.start_ref))
