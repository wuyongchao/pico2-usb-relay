# Security policy

## Supported versions

Security fixes are applied to the current `main` branch. There are no
long-term-support branches yet.

## Reporting a vulnerability

Please do not open a public issue for a vulnerability. On the version published
under a GitHub repository, use its private security advisory feature:

1. Open the repository's **Security** tab.
2. Select **Report a vulnerability**.
3. Include the affected commit, a reproduction, impact, and any suggested fix.

If private reporting is unavailable, contact the maintainer through the
contact method shown on their GitHub profile. This source archive does not
include a private contact address. Please allow time for a response before
disclosing the issue publicly.

## Scope

This project includes firmware that controls physical outputs. Relevant
security or safety reports include:

- Memory-safety issues in report parsing or descriptor handling.
- Commands that can set or clear unintended relay channels.
- Flash persistence or validation failures.
- A build or release artifact that can be modified undetected.
- Documentation that could lead someone to wire a load unsafely.

The project cannot provide electrical safety certification or guarantee
suitability for a particular load. Users are responsible for isolation,
enclosures, fuses, creepage distances, and regulatory compliance.
