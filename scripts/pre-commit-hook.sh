#!/bin/bash
# Pre-commit hook for ninjaUSB-util
# This script runs code formatting checks before commits
# 
# To install this hook, copy it to .git/hooks/pre-commit:
# cp scripts/pre-commit-hook.sh .git/hooks/pre-commit
# chmod +x .git/hooks/pre-commit

echo "🔧 Running pre-commit checks..."

# Change to repository root
cd "$(git rev-parse --show-toplevel)"

# Check if there are any staged C++ files
STAGED_CPP_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp)$' || true)

if [ -z "$STAGED_CPP_FILES" ]; then
    echo "ℹ️  No C++ files staged for commit"
    exit 0
fi

echo "📁 Found staged C++ files:"
echo "$STAGED_CPP_FILES"

# Check formatting for staged files
echo "🔍 Checking code formatting..."
FORMAT_ISSUES=0

for file in $STAGED_CPP_FILES; do
    if [ -f "$file" ]; then
        if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
            echo "❌ Formatting issues in: $file"
            FORMAT_ISSUES=1
        else
            echo "✅ Properly formatted: $file"
        fi
    fi
done

if [ $FORMAT_ISSUES -eq 1 ]; then
    echo ""
    echo "❌ Pre-commit check failed: Code formatting issues detected"
    echo "💡 Fix formatting with: ./scripts/format-code.sh"
    echo "💡 Or format specific files: echo '$STAGED_CPP_FILES' | xargs clang-format -i"
    echo "💡 Then stage the changes: git add ."
    echo ""
    echo "Alternatively, you can skip this check with: git commit --no-verify"
    exit 1
fi

echo "✅ Pre-commit checks passed"
exit 0
