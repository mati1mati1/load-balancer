#!/usr/bin/env bash
set -e

echo "Starting backend..."
nc -lk 9100 > backend_output.txt &
BACK_PID=$!

sleep 1

echo "Starting load balancer..."
./build/load_balancer config/config.json &
LB_PID=$!

sleep 1

echo "Sending request..."
echo "ping" | nc 127.0.0.1 9000

sleep 1
kill $LB_PID
kill $BACK_PID

echo "Backend output:"
cat backend_output.txt
