#!/bin/bash
# Run all tests before flashing

set -e

echo "🧪 Running unit tests..."
pio test -e test

if [ $? -ne 0 ]; then
  echo "❌ Unit tests failed"
  exit 1
fi

echo ""
echo "🔨 Compiling..."
pio run

if [ $? -eq 0 ]; then
  echo ""
  echo "✅ All checks passed"
  echo "Ready to flash"
else
  echo ""
  echo "❌ Compilation failed"
  exit 1
fi
