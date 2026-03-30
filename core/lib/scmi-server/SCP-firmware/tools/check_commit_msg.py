#!/usr/bin/env python3
#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

import sys
import argparse
import re
import subprocess
import os
import xml.sax.saxutils as xml_utils
from utils import banner, get_previous_commit, Results
from textblob import Word
from enum import Enum
from dataclasses import dataclass


class IssueLevel(Enum):
    ERROR = "error"
    WARNING = "warning"


@dataclass
class CommitIssue:
    level: IssueLevel
    message: str

    def __str__(self):
        symbol = "❌" if self.level == IssueLevel.ERROR else "❗"
        return f"{symbol} [{self.level.value.upper()}] {self.message}"


# Allowed abbreviations (common technical terms)
ALLOWED_ABBREVIATIONS = {"fwk", "mod", "prod", "msg", "ci", "arch", "doc"}

# Preferred commit prefixes
PREFERRED_PREFIXES = {"prod", "mod", "fwk", "ci", "tools", "arch", "doc"}

# Regular expression for commit message structure
COMMIT_REGEX = re.compile(
    r"^(?P<prefix>\w+)(/(?P<name>[\w\-]+))?(/test)?: (?P<subject>.+)$"
)

# Restricted commit subject words (should use imperative mood)
RESTRICTED_SUBJECT_WORDS = {
    "added", "fixed", "updated", "changed", "removed", "renamed", "merged"
}

SUBJECT_LIMIT = 70
BODY_LINE_LIMIT = 79

SIGN_OFF_PATTERN = re.compile(r"^\s*Signed-off-by:.*", re.IGNORECASE)


def get_commit_hashes(start_commit):
    """Retrieve all commit hashes from 'start_commit' to HEAD."""
    git_log_cmd = ["git", "rev-list", f"{start_commit}..HEAD"]
    result = subprocess.run(git_log_cmd, capture_output=True, text=True)

    print(result.stdout)
    print(result.stderr)

    return result.stdout.strip().split("\n") if result.stdout.strip() else []


def get_commit_message(commit_hash):
    """Retrieve the full commit message for a given commit hash."""
    git_show_cmd = ["git", "show", "-s", "--format=%B", commit_hash]
    result = subprocess.run(git_show_cmd, capture_output=True, text=True)
    return result.stdout.strip()


def check_spelling(text):
    """Checks spelling using TextBlob, allowing common abbreviations."""
    words = re.findall(r'\b[a-zA-Z]+\b', text)  # Extract words from text
    misspelled = []

    for word in words:
        word_lower = word.lower()
        spell_check = Word(word_lower).spellcheck()[0][1]
        if word_lower not in ALLOWED_ABBREVIATIONS and spell_check < 0.8:
            misspelled.append(word)

    return misspelled


def validate_commit_message(commit_message):
    """Validates a single commit message and returns a list of issues."""
    issues = []
    lines = commit_message.split("\n")

    if not lines:
        issues.append(CommitIssue(IssueLevel.ERROR,
                                  "Commit message is empty."))
        return issues

    first_line = lines[0]

    # Validate structure
    match = COMMIT_REGEX.match(first_line)
    if not match:
        issues.append(CommitIssue(
            IssueLevel.ERROR, "Incorrect format."
                              "Expected: <prefix>[/name][/test]: <subject>"))
    else:
        prefix, name, subject = match.group("prefix"), match.group("name"), \
            match.group("subject")

        # Validate prefix
        if prefix not in PREFERRED_PREFIXES:
            issues.append(CommitIssue(IssueLevel.WARNING,
                                      f"Not preferred prefix '{prefix}'."
                                      f"Preferred: "
                                      f"{', '.join(PREFERRED_PREFIXES)}"
                                      )
                          )

        # Validate subject length
        if len(subject) > SUBJECT_LIMIT:
            issues.append(
                CommitIssue(IssueLevel.ERROR,
                            f"Subject too long ({len(subject)} chars)."
                            f" Limit: {SUBJECT_LIMIT}."))

        # Validate imperative mood
        first_word = subject.split()[0].lower()
        if first_word in RESTRICTED_SUBJECT_WORDS:
            issues.append(
                CommitIssue(
                    IssueLevel.WARNING,
                    f"Subject should be in imperative mood "
                    f"(e.g., 'Add', not '{first_word.capitalize()}')."))

    # Check for blank line between title and body (if there is a body)
    if len(lines) > 1 and lines[1].strip():
        issues.append(CommitIssue(IssueLevel.ERROR,
                      "Missing blank line between title and body."))

    # Validate body lines length
    for line in lines[2:]:  # Skip the blank line
        if len(line) > BODY_LINE_LIMIT:
            issues.append(
                CommitIssue(
                    IssueLevel.ERROR,
                    f"Body line exceeds {BODY_LINE_LIMIT} chars: "
                    f"{line[:30]}..."))

    # Skip sign-off lines for spell-checking
    body_lines = \
        [line for line in lines[2:] if not SIGN_OFF_PATTERN.match(line)]

    # Check for Empty body
    if len(body_lines) == 0:
        issues.append(CommitIssue(IssueLevel.ERROR, "Empty message body"))

    misspelled_subject_words = check_spelling(first_line)
    misspelled_body_words = check_spelling("\n".join(body_lines))\
        if body_lines else []

    if misspelled_subject_words:
        issues.append(CommitIssue(IssueLevel.WARNING,
                                  "Misspelled words in subject: "
                                  f"{', '.join(misspelled_subject_words)}"))

    if misspelled_body_words:
        issues.append(CommitIssue(IssueLevel.WARNING,
                                  "Misspelled words in body: "
                                  f"{', '.join(misspelled_body_words)}"))
    return issues


def get_short_sha(full_sha):
    return full_sha[:7]


def generate_junit_xml(commit_results):
    """
    Generates a JUnit XML report for GitLab CI showing commit validation
    results, including both errors (as <failure>)
    and warnings (as <system-out>).
    """
    junit_report = '<?xml version="1.0" encoding="UTF-8"?>\n'
    junit_report += '<testsuites>\n'
    junit_report += '  <testsuite name="Commit Message Validation">\n'

    for commit_sha, data in commit_results.items():
        short_sha = get_short_sha(commit_sha)
        issues = data["issues"]

        error_issues = [i for i in issues if i.level == IssueLevel.ERROR]
        warning_issues = [i for i in issues if i.level == IssueLevel.WARNING]

        junit_report += f'    <testcase classname="commit" ' \
                        f'name="{short_sha}">\n'

        if error_issues:
            junit_report += f'      <failure message="Commit ({short_sha})' \
                            f' has errors">\n'
            for issue in error_issues:
                # Escape special XML characters in failure content
                escaped_msg = xml_utils.escape(issue.message)
                junit_report += f'       ❌ {escaped_msg}\n'
            junit_report += "      </failure>\n"

        warning_msg = ""
        warning_issues_str = ""
        if warning_issues:
            warning_msg += f'      <system-out><![CDATA[\n'
            warning_msg += f'❗ Commit ({short_sha}) has warnings:\n'
            for issue in warning_issues:
                warning_issues_str += f"- {issue.message}\n"
            warning_msg += warning_issues_str
            warning_msg += f']]></system-out>\n'

        if warning_issues and not error_issues:
            skipped_msg = f'❗ Commit ({short_sha}) has warnings'
            escaped_attr = xml_utils.escape(skipped_msg, {'"': "&quot;"})
            junit_report += f'      <skipped message="{escaped_attr}\n'
            junit_report += f'{warning_issues_str}"/>\n'
        elif warning_issues:
            junit_report += warning_msg

        if not error_issues and not warning_issues:
            junit_report += f'    <system-out>Commit ({short_sha}) ' \
                            f'Message follows best practices!</system-out>\n'

        junit_report += "    </testcase>\n"

    junit_report += "  </testsuite>\n"
    junit_report += "</testsuites>\n"

    # Save the XML report to a file
    with open("commit_msg_validation.xml", "w", encoding="utf-8") as f:
        f.write(junit_report)


def print_results(commit_results):
    for commit_sha, data in commit_results.items():
        commit_msg = data["commit_msg"]
        issues = data["issues"]

        has_errors = any(i.level == IssueLevel.ERROR for i in issues)
        has_warnings = any(i.level == IssueLevel.WARNING for i in issues)

        if has_errors:
            status = "❌"
        elif has_warnings:
            status = "❗"
        else:
            status = "✅"

        print(f"🔹 Commit ({commit_sha[:7]}) {status}:")
        print("-" * 80)
        print(commit_msg, "\n")

        if issues:
            print("\n".join(f"{issue}" for issue in issues), "\n")
        else:
            print("✅ Message follows best practices!")
        print("=" * 80)


def run(commit_hash):
    """Runs the commit validation for all commits from commit_hash to HEAD."""
    is_ci = os.getenv("CI") is not None  # Detect if running in GitLab CI

    print(banner(f"Checking all commits from {commit_hash[:7]} to HEAD..."))

    commit_hash = commit_hash.strip()
    commit_hashes = get_commit_hashes(commit_hash)

    if not commit_hashes:
        print("No commits found in the specified range.")
        return False

    results = Results()

    ret_val = True
    commit_results = {}
    for commit in commit_hashes:
        commit_msg = get_commit_message(commit)
        issues = validate_commit_message(commit_msg)

        # Store results in a dictionary
        commit_results[commit] = {
            "commit_msg": commit_msg,
            "issues": issues
        }

        ret_val = False if len(issues) else ret_val

    print_results(commit_results)
    if is_ci:
        generate_junit_xml(commit_results)

    return ret_val


def parse_args(argv, prog_name):
    """Parses command-line arguments."""
    parser = argparse.ArgumentParser(
        prog=prog_name,
        description="Perform Git commit message validation based on best "
                    "practices."
    )

    parser.add_argument(
        "-c",
        "--commit",
        dest="commit_hash",
        required=False,
        default=get_previous_commit(),
        type=str,
        action="store",
        help="Specify a starting commit hash."
             "Defaults to the previous commit.",
    )

    return parser.parse_args(argv)


def main(argv=[], prog_name=""):
    """Main function handling argument parsing and validation execution."""
    args = parse_args(argv, prog_name)
    return 0 if run(commit_hash=args.commit_hash) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:], sys.argv[0]))
