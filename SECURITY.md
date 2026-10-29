# Security policy

## Supported versions

No public release exists yet. This local snapshot has no security-maintenance guarantee. ROS Noetic is EOL; this project does not extend ROS upstream security support.

## Reporting

Report sensitive vulnerabilities privately to the upstream maintainer address already recorded in package.xml: **ybmq02@163.com**. Do not open a public issue containing credentials, exploitable deployment details or unnecessary flight logs. No response-time guarantee is offered.

Include the affected snapshot, environment, minimal reproduction, impact and suggested mitigation. Non-sensitive correctness issues belong in normal issue reports once an independent repository is approved.

The planner is not a flight-safety component. Treat maps, YAML/XMLRPC parameters, services and acknowledgment inputs as trusted deployment inputs. Finite/bounded geometry and a FREE in-bounds core start are required. Input-hardening and stale-data limitations are documented in docs/ENGINEERING_AUDIT.md.

No security service, bug bounty, disclosure platform or private-reporting feature is claimed to exist for an unpublished repository.
