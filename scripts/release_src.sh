#!/bin/bash
set -euo pipefail

# Install the build frontend with: python3 -m pip install build
# Update the version in pyproject.toml, then run this script from any directory.
# The source distribution in dist/ includes the metadata required by PyPI.

cd "$(dirname "${BASH_SOURCE[0]}")/.."
python3 -m build --sdist
