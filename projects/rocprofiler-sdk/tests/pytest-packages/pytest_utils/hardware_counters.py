#
# Tests helpers for hardware counter collection.
#

import os

import pytest


def has_kfd_profiler_interface():
    """Return whether a native KFD device node is available to the test process."""
    return os.path.exists("/dev/kfd")


def skip_if_hw_counter_values_unavailable(values):
    """Fail when expected hardware counter output is missing or non-positive.

    Runtime capability checks must skip before result validation. Observed
    output must never be used to infer that counter collection is unsupported.
    """
    found_value = False
    for value in values:
        found_value = True
        try:
            numeric_value = float(value)
        except (TypeError, ValueError):
            pytest.fail(f"hardware counter value is not numeric: {value!r}")

        if not numeric_value > 0:
            pytest.fail(f"hardware counter value is not positive: {value!r}")

    if not found_value:
        pytest.fail("no hardware counter values were reported")
