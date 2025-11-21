#!/bin/bash
set -e

# Compile (dacă vrei să nu recompili de fiecare dată, comentează liniile de compile)
gcc server.c -o server
gcc proxy.c -o proxy
gcc client.c -o client

# Start processes in background
./server &
SERVER_PID=$!
echo "Started server (pid=$SERVER_PID)"

./proxy &
PROXY_PID=$!
echo "Started proxy (pid=$PROXY_PID)"

./client &
CLIENT_PID=$!
echo "Started client (pid=$CLIENT_PID)"

PIDS=($SERVER_PID $PROXY_PID $CLIENT_PID)

# Trap signals to clean up everything
cleanup() {
  echo "Cleaning up..."
  for p in "${PIDS[@]}"; do
    if kill -0 "$p" 2>/dev/null; then
      kill "$p" 2>/dev/null || true
    fi
  done
  wait 2>/dev/null || true
  exit
}
trap cleanup SIGINT SIGTERM

# Wait for ANY process to exit
wait -n

# If we get here, at least one process exited; kill the rest
echo "One process exited — killing all others..."
for p in "${PIDS[@]}"; do
  if kill -0 "$p" 2>/dev/null; then
    kill "$p" 2>/dev/null || true
  fi
done

# Give processes a moment then force kill any leftovers
sleep 1
for p in "${PIDS[@]}"; do
  if kill -0 "$p" 2>/dev/null; then
    kill -9 "$p" 2>/dev/null || true
  fi
done

echo "All stopped."
