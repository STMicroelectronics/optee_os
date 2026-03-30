#!/usr/bin/env python3
#
# Arm SCP/MCP Software
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import os
import requests
from datetime import datetime
from config.slack_notifications.generate_slack_message import (
    SlackMessageParams,
    generate_slack_message
)


def post_to_slack(webhook_url, message):
    headers = {'Content-Type': 'application/json'}
    response = requests.post(webhook_url, json=message, headers=headers)
    if response.status_code != 200:
        raise ValueError(f"Request to Slack returned an \
        error {response.status_code}, \
        the response is:\n{response.text}")


def get_pipeline_duration(start_time_env_var):
    start = os.getenv(start_time_env_var)
    if start:
        start_datetime = datetime.strptime(start, "%Y-%m-%dT%H:%M:%SZ")
        duration_s = (datetime.utcnow() - start_datetime).seconds
        return format_duration(duration_s)
    return "N/A"


def format_duration(seconds):
    hours, remainder = divmod(seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    # Format as a human-readable string for the slack message
    if hours > 0:
        return f"{hours}h {minutes}m {seconds}s"
    elif minutes > 0:
        return f"{minutes}m {seconds}s"
    else:
        return f"{seconds}s"


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--status", required=True, help="Pipeline status: success or failure"
    )
    args = parser.parse_args()

    duration = get_pipeline_duration("CI_PIPELINE_CREATED_AT")

    # Populate the environment variables for generating the message
    params = SlackMessageParams(
        pipeline_type=os.getenv("PIPELINE_TYPE"),
        job_status=args.status,
        project_path=os.getenv("CI_PROJECT_PATH"),
        merge_request_iid=os.getenv("FETCH_PUBLIC_MR_NUMBER", ""),
        merge_request_title=os.getenv("EXTERNAL_MR_TITLE", ""),
        merge_request_event_user=os.getenv("CI_MERGE_REQUEST_EVENT_USER", ""),
        pipeline_id=os.getenv("CI_PIPELINE_ID"),
        pipeline_iid=os.getenv("CI_PIPELINE_IID"),
        pipeline_duration=duration,
        gitlab_user_name=os.getenv("GITLAB_USER_NAME"),
        public_repo=os.getenv("PUBLIC_REPO_URL"),
        pipeline_repo=os.getenv("CI_PIPELINE_URL"),
    )
    slack_webhook_url = os.getenv("SLACK_HOOK_URL")

    slack_message = generate_slack_message(params)
    post_to_slack(slack_webhook_url, slack_message)
