#!/usr/bin/env bash
# run_tests.sh — integration test runner for setmark.so
#
# Runs the test binary multiple times with different SO_MARK values via
# LD_PRELOAD.  Exit code 0 = all tests passed (or skipped), 1 = failure.
#
# Requires CAP_NET_ADMIN (typically sudo) for the mark-value tests; the
# other tests still run and pass without elevated privileges.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIB="$ROOT/setmark.so"
TEST="$ROOT/tests/test_mark"

PASS=0
FAIL=0
SKIP=0

if [ ! -f "$LIB" ]; then
    printf 'Error: %s not found — run "make" first.\n' "$LIB" >&2
    exit 1
fi

if [ ! -f "$TEST" ]; then
    printf 'Error: %s not found — run "make test-bin" first.\n' "$TEST" >&2
    exit 1
fi

run_scenario() {
    local label="$1"
    local mark="$2"

    printf '\n--- %s ---\n' "$label"
    local ret=0
    SO_MARK="$mark" LD_PRELOAD="$LIB" "$TEST" || ret=$?

    case $ret in
    0) PASS=$((PASS + 1)) ;;
    77) SKIP=$((SKIP + 1)) ;;
    *) FAIL=$((FAIL + 1)) ;;
    esac
}

echo '=== Socket Marker — integration tests ==='

run_scenario "default mark (1234)" 1234
run_scenario "custom mark 100" 100
run_scenario "custom mark 65535" 65535
run_scenario "max 32-bit mark (0xFFFFFFFF)" 4294967295

printf '\n%s\n' '=================================='
printf 'Scenarios: %d passed, %d failed, %d skipped\n' \
    "$PASS" "$FAIL" "$SKIP"

[ "$FAIL" -eq 0 ]
