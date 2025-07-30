#!/bin/bash
# Format C++ source code using clang-format
# Usage: ./scripts/format-code.sh [--check]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Change to project root
cd "$PROJECT_ROOT"

# Check if clang-format is available
if ! command -v clang-format >/dev/null 2>&1; then
    echo "❌ clang-format not found. Please install clang-format."
    echo "On Ubuntu: sudo apt install clang-format"
    echo "On macOS: brew install clang-format"
    exit 1
fi

# Check if .clang-format exists
if [ ! -f ".clang-format" ]; then
    echo "❌ .clang-format configuration file not found in project root"
    exit 1
fi

echo "🔧 Using clang-format version: $(clang-format --version | head -1)"

# Find all C++ source files
FILES=$(find src tests -name "*.cpp" -o -name "*.hpp" 2>/dev/null | sort)

if [ -z "$FILES" ]; then
    echo "ℹ️  No C++ files found to format"
    exit 0
fi

echo "📁 Found $(echo "$FILES" | wc -l) C++ files to process"

if [ "$1" = "--check" ] || [ "$1" = "-c" ]; then
    # Check mode: verify formatting without making changes
    echo "🔍 Checking code formatting (dry-run mode)..."
    
    FORMAT_ISSUES=0
    for file in $FILES; do
        if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
            echo "❌ Formatting issues in: $file"
            FORMAT_ISSUES=1
            
            # Show a preview of the formatting changes
            echo "   Preview of expected changes:"
            echo "   =========================================="
            clang-format "$file" | diff -u "$file" - | head -20 || true
            echo "   =========================================="
        else
            echo "✅ Properly formatted: $file"
        fi
    done
    
    if [ $FORMAT_ISSUES -eq 1 ]; then
        echo ""
        echo "❌ Code formatting check failed!"
        echo "💡 Run './scripts/format-code.sh' to fix formatting automatically"
        echo "💡 Or run: find src tests -name \"*.cpp\" -o -name \"*.hpp\" | xargs clang-format -i"
        exit 1
    else
        echo "✅ All C++ files are properly formatted"
        exit 0
    fi
else
    # Format mode: apply formatting changes
    echo "🎨 Formatting C++ source code..."
    
    for file in $FILES; do
        echo "Formatting: $file"
        clang-format -i "$file"
    done
    
    echo "✅ Formatting completed for all C++ files"
    echo "💡 You can verify the changes with: git diff"
    echo "💡 Run './scripts/format-code.sh --check' to verify formatting"
fi
