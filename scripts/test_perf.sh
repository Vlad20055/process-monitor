#!/bin/sh
BIN=build/process_monitor
TEST_RUNS=${1:-5}
EXPECT_MAX=${2:-1.0}

if [ ! -x "$BIN" ]; then
  echo "Executable $BIN not found. Build first.";
  exit 2;
fi

total=0
i=1
while [ $i -le "$TEST_RUNS" ]; do
  start=$(date +%s.%N)
  "$BIN" --once >/dev/null 2>&1
  end=$(date +%s.%N)
  elapsed=$(echo "$end - $start" | bc -l)
  echo " run $i: ${elapsed}s"
  total=$(echo "$total + $elapsed" | bc -l)
  i=$((i+1))
done

avg=$(echo "scale=6; $total / $TEST_RUNS" | bc -l)
echo "Average time: ${avg}s (expect <= ${EXPECT_MAX}s)"
ok=$(echo "$avg <= $EXPECT_MAX" | bc -l)
if [ "$ok" -eq 1 ]; then
  echo "PERF TEST: PASS"
else
  echo "PERF TEST: FAIL"
  exit 1
fi
