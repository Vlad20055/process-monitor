#!/bin/sh
if [ ! -x "build/process_monitor" ]; then
  echo "Executable build/process_monitor not found. Build first.";
  exit 2;
fi

sleep 60 &
pid=$!
echo $pid > .test_sleep_pid
echo "Spawned sleep with pid=$pid"
sleep 0.2

printf "kill %s\nquit\n" $pid | ./build/process_monitor --name-filter sleep >/dev/null 2>&1 || true

sleep 0.2
if ps -p $pid >/dev/null 2>&1; then
  echo "TERMINATION TEST: FAIL - process $pid still running"
  kill -9 $pid >/dev/null 2>&1 || true
  exit 1
else
  echo "TERMINATION TEST: PASS - process $pid terminated"
fi

rm -f .test_sleep_pid
