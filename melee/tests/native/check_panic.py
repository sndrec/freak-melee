"""Verify the real native assertion path exits and flushes its diagnostic."""
import subprocess
import sys

result = subprocess.run(
    [sys.argv[1], "--panic"], capture_output=True, text=True, timeout=20
)
if result.returncode == 0:
    raise AssertionError("Native assertion unexpectedly returned successfully")
if "intentional native panic test" not in result.stdout:
    raise AssertionError(f"Assertion condition missing: {result.stdout!r}")
if "native panic observer: native_heap_test.c:73" not in result.stderr:
    raise AssertionError(f"Native panic callback missing: {result.stderr!r}")
if "in native_heap_test.c on line 73." not in result.stderr:
    raise AssertionError(f"Fatal location missing: {result.stderr!r}")
print("Native assertion terminated and flushed its diagnostic and callback")
