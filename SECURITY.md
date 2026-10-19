# Security Policy

## Supported Versions

The Arduino library, firmware releases published on GitHub, and firmware deployed through OTA may follow different release schedules. Security reports are accepted for every version, including unsupported versions, but fixes are normally released only for versions listed as supported below.

<!--
Maintainers: replace every placeholder in this table with approved values before merging.
Keep the library, manually flashed firmware, OTA firmware, and legacy hardware separate.
-->

| Component or release channel | Security-supported version(s) |
| --- | --- |
| Arduino library | `OWNER_ACTION_REQUIRED: current supported library version or range` |
| Firmware published on GitHub Releases for manual flashing | `OWNER_ACTION_REQUIRED: current supported firmware release or range` |
| Firmware deployed through OTA | `OWNER_ACTION_REQUIRED: current supported OTA version or range` |
| Legacy PCB revisions that require library version 2.4.15 | `OWNER_ACTION_REQUIRED: supported or end of life` |

Older versions do not normally receive backported fixes. Users should run the latest security-supported version compatible with their hardware and release channel. GitHub releases may undergo staged device testing before becoming available through automatic firmware updates.

## Reporting a Vulnerability

Please report suspected security vulnerabilities privately. **Do not open a public GitHub issue or forum topic before the vulnerability has been investigated and the coordinated disclosure process below has been followed.**

<!--
Maintainers: enable GitHub Private Vulnerability Reporting and replace the email handshake below with its direct report URL, or publish a dedicated security address and a public encryption key.
-->

Until a dedicated secure intake channel is published, email [support@airgradient.com](mailto:support@airgradient.com) with the subject:

`Request for secure vulnerability reporting channel: Open-Air-Foundation/arduino`

The initial email should contain only your contact details, the affected component and version, and a brief non-sensitive summary. **Do not include exploit details, proof-of-concept code, credentials, logs, network traces, photographs, or personal data in the initial email.** We will reply with instructions for securely sharing sensitive material.

Once a secure channel has been established, include as much of the following information as possible:

- the affected monitor model, hardware revision, firmware version, and commit or release;
- the affected component or file;
- a clear description of the vulnerability and its security impact;
- reproducible steps or a minimal proof of concept;
- relevant logs, crash output, network traces, or photographs, with credentials and personal data removed;
- any known mitigations or suggested fixes;
- how you would like to be credited, or whether you prefer to remain anonymous.

We aim to acknowledge reports within 7 calendar days and provide an initial status update within 14 calendar days. Remediation timelines depend on severity, affected hardware, and the testing required for a safe firmware rollout. If you do not receive an acknowledgement within 7 days, please follow up on the same email thread without adding sensitive details.

## Scope

This policy covers security issues in code, configuration, and pinned dependencies maintained in this repository, including:

- firmware and library code;
- firmware update and rollback mechanisms;
- local HTTP, MQTT, AirGradient cloud client, Wi-Fi, Bluetooth, and cellular communication paths;
- device provisioning and configuration;
- bundled or pinned dependencies when they create an exploitable issue in a supported AirGradient release.

AirGradient services or infrastructure whose implementation is not maintained in this repository are outside this repository's scope. If you are unsure where an issue belongs, use the initial contact process above so it can be routed without publicly disclosing details.

## Coordinated Disclosure

<!--
Maintainers: confirm that the 90-calendar-day default below matches the project's response capacity and disclosure policy before merging.
-->

The default disclosure deadline is 90 calendar days from the initial contact or secure report, whichever occurs first. We will keep reporters informed of material progress and coordinate a disclosure date. The deadline may be extended or shortened by mutual written agreement.

If a fix or mitigation is not available by the agreed deadline, the reporter may disclose the vulnerability while avoiding unnecessary exposure of user data or exploit details. If exploitation is active or details are already public, we may coordinate an accelerated disclosure and mitigation timeline.

When a report is accepted, we may publish a GitHub Security Advisory and release notes after a fix or mitigation is available. With the reporter's permission, we will include acknowledgement in the advisory or release notes.

## Safe Harbor

<!--
Maintainers and legal counsel: review and approve this safe-harbor commitment before merging.
-->

When security research is conducted in good faith and in accordance with this policy, we consider that research authorized and will not initiate or recommend legal action against the researcher. If a third party initiates legal action related to research performed under this policy, we will make it known that the research was conducted in compliance with this policy.

If you are uncertain whether your planned research is consistent with this policy, contact us through the initial reporting process before proceeding.

## Research Guidelines

When investigating a potential vulnerability:

- test only devices and accounts that you own or are explicitly authorized to use;
- avoid accessing, modifying, or retaining other people's data;
- avoid disrupting production services or other users' devices;
- stop testing once the vulnerability and impact have been demonstrated;
- do not use social engineering, physical coercion, or denial-of-service techniques.

Good-faith reports that follow these guidelines are appreciated and will be handled constructively.
