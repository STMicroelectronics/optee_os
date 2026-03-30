#!/usr/bin/env python3
#
# Arm SCP/MCP Software
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

from dataclasses import dataclass
from enum import Enum


@dataclass
class SlackMessageParams:
    pipeline_type: str
    job_status: str
    project_path: str
    merge_request_iid: str
    merge_request_title: str
    merge_request_event_user: str
    pipeline_id: str
    pipeline_iid: str
    pipeline_duration: str
    gitlab_user_name: str
    public_repo: str
    pipeline_repo: str


class JobStatus(Enum):
    SUCCESS = ("#36A64F", "Passed")  # Green
    FAILURE = ("#E01E5A", "Failed")  # Red

    @property
    def color(self):
        return self.value[0]

    @property
    def status(self):
        return self.value[1]


def generate_slack_message(params: SlackMessageParams):
    job_status_enum = JobStatus[params.job_status.upper()]

    color = job_status_enum.color
    status = job_status_enum.status

    if params.pipeline_type == "deployment-pipeline":
        url = params.public_repo.removesuffix(".git")
        clean_url = url.replace("git.", "", 1)
        title = "*External MR Development Pipeline*"
        mr_text = (
            f"<{clean_url}/-/merge_requests/{params.merge_request_iid}|"
            f"MR Title: {params.merge_request_title}>"
        )
        button_text = "View Pipeline"
        button_url = params.pipeline_repo
    elif params.pipeline_type == "daily-pipeline":
        title = "*Daily Pipeline*"
        mr_text = f"Pipeline: {params.pipeline_id}"
        button_text = "View Results"
        button_url = params.pipeline_repo
    else:
        print("*** No pipeline populated ***")
        print(f"Pipeline_type: '{params.pipeline_type}'")

    message = {
        "attachments": [
            {
                "color": color,
                "blocks": [
                    {
                        "type": "section",
                        "text":
                            {
                                "type": "mrkdwn",
                                "text": title
                            },
                    },
                    {
                        "type": "section",
                        "text":
                            {
                                "type": "mrkdwn",
                                "text": mr_text
                            },
                    },
                    {
                        "type": "section",
                        "fields": [
                            {
                                "type": "mrkdwn",
                                "text": f"*Status:*\n{status}"
                            },
                            {
                                "type": "mrkdwn",
                                "text": (
                                    f"*Duration:*\n"
                                    f"{params.pipeline_duration}"
                                )
                            },
                        ],
                    },
                    {
                        "type": "actions",
                        "elements": [
                            {
                                "type": "button",
                                "text":
                                    {
                                        "type": "plain_text",
                                        "text": button_text,
                                        "emoji": True
                                    },
                                "url": button_url,
                            }
                        ],
                    },
                ],
            }
        ]
    }
    # Only add who triggered the job if it's a deployement pipeline
    if params.pipeline_type == "deployment-pipeline":
        message["attachments"][0]["blocks"].append(
            {
                "type": "context",
                "elements": [
                    {
                        "type": "plain_text",
                        "text": (
                            f"Triggered by "
                            f"{params.gitlab_user_name}"
                        ),
                        "emoji": True
                    },
                ],
            }
        )
    return message
