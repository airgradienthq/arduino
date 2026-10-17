# VHUB Verification Templates

VHUB is a self-hosted workspace for QA to manually validate AirGradient products.
Import a released JSON procedure, start a validation, and record one status and
result notes per applicable test case and variant.

This directory contains product-specific manual release-verification templates
consumed by VHUB. This guide defines the shared authoring format and writing
rules for maintainers and agents across the firmware monorepo.

## Scope

- Include observable outcomes that still need human verification, such as
  display quality, touch response, sound, or faults automation cannot reproduce.
- Inspect host tests, device integration tests, and scripts before adding or
  retaining cases. Do not repeat host-covered parsing, encoding, calculations,
  or deterministic logic checks manually.
- Keep checks assigned to developer-owned host tests, device integration tests,
  or scripts out of manual procedures. Distinguish existing replacement coverage
  from planned or deferred coverage; removing a case does not establish a pass.
- Verify actual assertions prove the outcome. A response-format check that
  accepts failure does not prove successful operation.
- Retain a case for its distinct human verification value. Adding a display or
  restart observation does not justify repeating a developer-owned workflow or
  an outcome already checked by another manual case.
- Share cases across interchangeable sensors; note sensor-specific differences
  within the case. Require separate executions only for different expected
  behavior or explicit release-plan requirements.
- Keep product procedures in its cases or linked documentation. Assess every
  affected product when shared components change.
- When automation replaces a case, record the replacement coverage or planned
  follow-up in the change summary. Follow the agreed release execution rules;
  those rules need not be changed in the template's branch.

## QA Audience and Tools

Write procedures that QA can execute with device controls and documented GUI
tools, without programming knowledge or a firmware development environment.
This applies to setup, test steps, evidence collection, and cleanup.
Name established QA tools; assume QA knows their standard workflows. Do not
repeat basic tool setup instructions or add links to familiar tool guides. Keep
case-specific actions, observations, and timing in the case.

| Workflow | Procedure Requirements |
| --- | --- |
| Device controls and ordinary test equipment | Describe the physical actions and visible or audible results, including bounded waits. |
| API requests through Postman or an equivalent GUI client | Supply the method, URL, authentication, headers, request body, and how to find device-specific values. Check the resulting device behavior. |
| Firmware logs through the AirGradient logging tool | Specify the event or value to observe and the observation period. Request only relevant evidence. |
| Cloud dashboard and browser setup | Identify the device, setting, or form and the outcome to observe on the device or dashboard. |
| Bluetooth pairing and inspection through a GUI | Name the available tool and provide the supported actions and any prepared inputs needed. |

Use the product's mDNS hostname for routine local API requests. Use IP addresses
only when the scenario specifically depends on them, such as provisioning or
static-IP verification.

Do not require QA to check out the repository, install programming dependencies,
create virtual environments, write or run code or scripts, build firmware, or
execute automated test suites. Describe API requests so they can be entered in
a GUI without shell commands. Supplied JSON request bodies are test inputs;
generating or decoding payloads with code is developer work.

Tool choice alone does not make a check suitable for VHUB. API response-code
matrices, protocol schemas, encoding, and deterministic calculations belong in
developer tests even when a GUI can exercise them. API requests and logs are
valid ways to trigger or observe a manual behavior check; a response or log
alone must not stand in for a physical outcome the case claims to verify.

For a case that mixes QA observations and developer work, preserve the useful
manual scenario and assign the scripted portion to developer coverage. If a
required operation has no documented QA tool or reproducible procedure, report
the gap and defer that scenario. Do not invent a GUI workflow, hide code behind
a prerequisite, or label a deferred check as covered.

## Template Structure

All products use the structured case format defined in this guide. Each template
contains the following fields, with individual cases in `tests`.

| Field | Purpose |
| --- | --- |
| `schema_version` | Declares the format understood by the importer. |
| `rev` | Records the revision associated with the template update. |
| `product` | Contains the product `slug`, `name`, and supported `variants`. |
| `tests` | Contains that product's test cases. |

### Case Fields

All fields below are required. Use empty arrays where explicitly allowed.

| Field | Type | Content |
| --- | --- | --- |
| `id` | String | Stable, unique identifier within the product template, such as `settings.display.unit-persistence`. |
| `title` | String | Short scenario name. |
| `objective` | String | What the case verifies and why it matters. |
| `category` | String | Main grouping. |
| `sub_category` | String | Grouping within the category. |
| `applicability` | String array | Nonempty list of applicable variants declared by the product. |
| `setup` | String array | Case-specific equipment, access, test inputs, and device prerequisites. One independently checkable prerequisite per entry; `[]` if none. Preparation actions belong in `steps`. |
| `steps` | Object array | Nonempty, ordered list of actions and expected results. |
| `cleanup` | String array | Restoration actions, including after an interrupted or failed run; `[]` if none. |
| `references` | String array | Supporting document links; `[]` if none. Use repository-root-relative paths or full URLs. |

### Step Fields

| Field | Type | Required | Content |
| --- | --- | --- | --- |
| `action` | String | Yes | What the tester does. |
| `expected_result` | String | Yes | Observable result that determines whether the step passes. |
| `evidence` | String array | No | Instructions for what to record or attach for this step. Omit when unnecessary. |

There is no separate case-level `expected_result`, `evidence`, or `notes` in the
case format. Expectations belong beside their actions. Evidence fields
contain collection instructions; actual results, device identifiers, firmware
versions, and uploaded evidence belong to the release execution record.

## Writing Rules

A tester should be able to execute a case without designing the experiment or
asking its author what counts as a pass.

| Write Cases That | Avoid |
| --- | --- |
| Verify one coherent scenario with a clear outcome. | Combining unrelated checks because they share a menu or component. |
| List each required resource or eligibility condition in its own `setup` entry. | Packing independent prerequisites into a sentence, hiding preparation actions in setup, or repeating them in both places. |
| Put preparation, baseline capture, and test actions in execution order. | Assuming another case left the device in a particular state. |
| Pair each action with its observable expectation. | Mixing instructions into expected results. |
| Specify concrete inputs, bounded waits, and observable timing start points. | “Works correctly,” “wait a while,” or “retry until it passes.” |
| Derive behavior and limits from the product's requirements and implementation. | Inventing deadlines, log messages, commands, or test tools. |
| Request evidence useful for establishing or investigating the result. | Requiring large log dumps for every visual check. |
| Restore recorded settings and remove only test-created data. | Implicit cleanup or deleting unrelated data. |
| Link exact tool procedures and keep acceptance criteria in the case. | Making QA inspect firmware source to determine a pass. |

### Setup, Steps, and Cleanup

`setup` answers “What does this case specifically require?” Examples include a
controllable network, a test account, or a client with a documented
capability. Name the tool and link its instructions when a special operation is
required, such as interrupting a transfer or decoding an export.

Put each independently checkable prerequisite in its own array entry. Do not
join separate conditions with semicolons or pack them into a paragraph. Keep
qualifications with the resource they describe, such as “A computer with
Bluetooth and the documented pairing GUI.” Device eligibility, network access, and
expendable data are separate prerequisites.

Include only prerequisites the case needs. Do not repeat the product or variant
already declared by the template and `applicability`, or general release
preparation such as installing the release firmware and recording its version.
Do not add “an initialized device” to every case. Include a starting state only
when it affects this scenario's execution or result; steps that establish that
state make a matching setup entry unnecessary. Use `setup: []` when no specific
prerequisites are needed. Repeated entries must be justified by each case, not
copied as a standard checklist.

Make prerequisites verifiable: use a
documented battery range or observable charge state when it matters, rather
than “sufficient charge.” Do not invent a threshold for a case that does not
need one. Connecting equipment, selecting a mode, saving evidence, and filling
in device-specific request values belong in `steps`, not `setup`.

`steps` answers “What do I do?” Powering off, disconnecting a cable, selecting a
mode, recording a setting, and creating test data are actions. Write each action
once at the point it is performed. Preparation steps can have simple expected
results that confirm readiness for the next action.

`cleanup` answers “What must I restore?” Record original values before changing
them. Include restoration after failure or interruption, using only baselines
actually captured. If restoration fails, record the remaining state. Destructive
cases must identify expendable devices or data in setup; do not promise to
restore data the procedure intentionally erases.

### Scope and Evidence

Keep related actions together when they prove one outcome, such as changing a
setting and checking persistence after restart. Split cases when they need
different preparation or would be investigated and rerun independently. Do not
turn every button press or internal code branch into a separate case.

Use logs when they establish an otherwise hidden result or help diagnose a
failure. Exact log text should be a pass condition only when that text is itself
required or is the documented way to observe the behavior. Preserve functional
coverage without binding every case to incidental implementation wording.

For comparisons across interfaces, explain how to identify the same measurement
and account for conversion, rounding, and updates during capture. For faults or
timing-sensitive scenarios, supply a reproducible trigger and a bounded
observation period. If those are unavailable, resolve the procedure before
treating the case as ready for release execution.

An expected rejection passes a negative test. Missing prerequisites block a run;
they do not establish a pass. Required checks cannot be skipped in a passing
run. Record failures before retries, and retain the original result.

## Generic Examples

These examples illustrate writing patterns, not requirements for any shipping
product. `ExampleVariant`, menu names, endpoints, values, and time limits below are
fictional. Replace them with verified product details when authoring a case.

### Visible Output

This compact JSON example shows the complete case shape. It assumes a product
with a documented indicator diagnostic; no firmware implementation is implied.
It needs no special prerequisites, so `setup` is empty.

```json
{
  "id": "diagnostics.indicator.green",
  "title": "Indicator diagnostic displays green",
  "objective": "Verify that the indicator can produce the requested green output.",
  "category": "Diagnostics",
  "sub_category": "Indicators",
  "applicability": ["ExampleVariant"],
  "setup": [],
  "steps": [
    {"action": "Open Diagnostics and select Green Indicator.", "expected_result": "The indicator turns green within two seconds of selection."},
    {"action": "Select Exit.", "expected_result": "The diagnostic closes and the previous indicator behavior returns."}
  ],
  "cleanup": ["If interrupted, exit the diagnostic to restore normal operation."],
  "references": []
}
```

### Setting Persistence

Keep the menu interaction, physical display, and restart in one scenario. A
configuration write/read round trip alone belongs in developer tests. Link the
product's menu and restart procedures when adapting this example.

```json
{
  "id": "settings.display.unit-persistence",
  "title": "The displayed temperature unit survives restart",
  "objective": "Verify menu selection, visible units, and persistence on the device.",
  "category": "Settings",
  "sub_category": "Display",
  "applicability": ["ExampleVariant"],
  "setup": [],
  "steps": [
    {"action": "Open Settings > Temperature Unit and record the selected unit.", "expected_result": "The current choice is visible and recorded."},
    {"action": "Select the opposite unit and return Home. Observe for five seconds.", "expected_result": "Home uses the selected unit with readable labels and no overlapping text."},
    {"action": "Select Settings > Restart. After Home returns, inspect its unit and reopen Temperature Unit, allowing 30 seconds from restart selection.", "expected_result": "Home and the menu retain the selected unit.", "evidence": ["Save a post-restart Home photo."]}
  ],
  "cleanup": ["Restore the recorded unit through Settings > Temperature Unit and confirm it on Home, including after interruption."],
  "references": []
}
```

### API-Triggered Physical Output

This fictional diagnostic is invoked through a GUI API client. Acceptance of
the request does not prove that the physical indicator works. Link the actual
client procedure and diagnostic contract when adapting this example.

```json
{
  "id": "diagnostics.indicator.remote",
  "title": "The remote indicator action lights the physical LED",
  "objective": "Verify the physical indicator responds to the remote diagnostic and returns to its previous state.",
  "category": "Diagnostics",
  "sub_category": "Indicators",
  "applicability": ["ExampleVariant"],
  "setup": ["A computer with Postman or an equivalent GUI API client that resolves .local names on the test LAN."],
  "steps": [
    {"action": "Read the mDNS hostname from About Device and record it as <device-hostname>. Record the indicator's current state.", "expected_result": "The intended device and its original indicator state are identified."},
    {"action": "In the API client, send POST http://<device-hostname>/diagnostics/indicator with Content-Type: application/json, no authentication, and body {\"color\":\"green\"}, replacing <device-hostname> with the recorded mDNS hostname. Watch the device for ten seconds from Send.", "expected_result": "The client reports acceptance, the physical LED turns green within two seconds, and its previous state returns within ten seconds.", "evidence": ["Record a video of the indicator sequence; save the response if the action fails."]}
  ],
  "cleanup": ["If the diagnostic remains active, select Diagnostics > Exit on the device and confirm the recorded indicator state returns."],
  "references": []
}
```

## Maintenance

Review the relevant product template before opening each pull request. Update it
only when firmware changes behavior that a tester can observe through the
device, hardware, serial logs, network interfaces, or server data.

Inspect existing coverage before adding cases. Update a case when its scenario
is unchanged; add a case for a distinct scenario. Preserve unrelated checks and
case IDs. When a split, merge, or removal is necessary, explain the old-to-new
coverage mapping in the change summary so reviewers can detect lost coverage.
Identify developer checks that already exist separately from planned or deferred
work; do not claim automation results merely because coverage was assigned to it.

Set `rev` to the current seven-character Git HEAD when template content changes:

```sh
git rev-parse --short=7 HEAD
```

This is the repository's current revision convention. Several uncommitted edits
at the same HEAD can share a value; `rev` alone is not proof of an immutable
snapshot. Do not change it for a review that leaves template content unchanged.

## Validation

For a template update, validate JSON syntax and use a compatible schema validator
when one is available. This repository does not currently provide a VHUB schema
validator. Syntax alone does not validate the case structure or prove that a
hardware test passes.

Check unique IDs, required fields and types defined in this guide, applicability
against the product variants, nonempty actions and expectations, and reference
targets. Check that baseline capture precedes mutation and that cleanup covers
changes made by the procedure. Review the diff for lost or duplicated coverage.
Read every setup entry as a checklist item: it must express one relevant,
verifiable, case-specific prerequisite and contain no execution instructions.
Remove product/applicability repetition and general release-preparation items.
Check that the
steps establish temporary state and do not repeat setup as preparation work.

Review the complete procedure against [QA Audience and Tools](#qa-audience-and-tools),
including cleanup and evidence collection. Confirm API requests are usable in a
GUI, log observations are concrete, and no step requires a development environment
or code execution. Check that a physical pass condition is supported by a human
observation and that the case adds value beyond existing manual and developer
coverage.

The author is responsible for delivering cases that QA can execute as written,
with confirmed prerequisites, available tools, and clear pass conditions.
Validate Markdown changes with the repository's pre-commit hooks.
