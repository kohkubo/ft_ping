#!/bin/bash

# Ping Error Test Script
# 実際のpingコマンドとft_pingの出力を比較テストする

set -e

# テスト結果ディレクトリを作成
mkdir -p test_results

# カラー出力の設定
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Ping Error Test Cases ==="
echo

# ft_pingをビルド
echo "Building ft_ping..."
make clean > /dev/null 2>&1
make ft_ping > /dev/null 2>&1

if [ ! -f "./ft_ping" ]; then
    echo -e "${RED}Error: ft_ping binary not found${NC}"
    exit 1
fi

# テストケース1: 存在しないホスト名
echo -e "${YELLOW}Test 1: Nonexistent hostname${NC}"
echo "Testing: nonexistent.example.com"
echo "--- Real ping output ---"
ping -c 1 nonexistent.example.com 2>&1 | head -3 | tee test_results/real_ping_nonexistent.txt
echo
echo "--- ft_ping output ---"
./ft_ping nonexistent.example.com 2>&1 | head -3 | tee test_results/ft_ping_nonexistent.txt
echo
echo

# テストケース2: 無効なIPアドレス
echo -e "${YELLOW}Test 2: Invalid IP address${NC}"
echo "Testing: 999.999.999.999"
echo "--- Real ping output ---"
ping -c 1 999.999.999.999 2>&1 | head -3 | tee test_results/real_ping_invalid_ip.txt
echo
echo "--- ft_ping output ---"
./ft_ping 999.999.999.999 2>&1 | head -3 | tee test_results/ft_ping_invalid_ip.txt
echo
echo

# テストケース3: 引数なし
echo -e "${YELLOW}Test 3: No arguments${NC}"
echo "Testing: ping with no arguments"
echo "--- Real ping output ---"
ping 2>&1 | head -3 | tee test_results/real_ping_no_args.txt
echo
echo "--- ft_ping output ---"
./ft_ping 2>&1 | head -3 | tee test_results/ft_ping_no_args.txt
echo
echo

# テストケース4: 無効なオプション
echo -e "${YELLOW}Test 4: Invalid option${NC}"
echo "Testing: -invalid-option"
echo "--- Real ping output ---"
ping -invalid-option 2>&1 | head -3 | tee test_results/real_ping_invalid_option.txt
echo
echo "--- ft_ping output ---"
./ft_ping -invalid-option 2>&1 | head -3 | tee test_results/ft_ping_invalid_option.txt
echo
echo

# テストケース5: 権限テスト（一般ユーザーの場合）
echo -e "${YELLOW}Test 5: Permission test (if not root)${NC}"
if [ "$EUID" -ne 0 ]; then
    echo "Testing: permission error (not root)"
    echo "--- ft_ping output ---"
    ./ft_ping 127.0.0.1 2>&1 | head -3 | tee test_results/ft_ping_permission.txt
    echo
else
    echo "Running as root - skipping permission test"
fi
echo

# テストケース6: 正常ケース（比較用）
echo -e "${YELLOW}Test 6: Valid case (for comparison)${NC}"
echo "Testing: 127.0.0.1"
echo "--- Real ping output ---"
ping -c 1 127.0.0.1 2>&1 | head -5 | tee test_results/real_ping_valid.txt
echo
echo "--- ft_ping output (with timeout) ---"
timeout 3 ./ft_ping 127.0.0.1 2>&1 | head -5 | tee test_results/ft_ping_valid.txt || echo "Timeout or error occurred"
echo
echo

# テストケース7: ホスト名が長すぎる
LONG_HOSTNAME="$(printf 'a%.0s' {1..300}).example.com"
echo -e "${YELLOW}Test 7: Hostname too long${NC}"
echo "Testing: $LONG_HOSTNAME"
echo "--- Real ping output ---"
ping -c 1 "$LONG_HOSTNAME" 2>&1 | head -3 | tee test_results/real_ping_long_hostname.txt
echo
echo "--- ft_ping output ---"
./ft_ping "$LONG_HOSTNAME" 2>&1 | head -3 | tee test_results/ft_ping_long_hostname.txt
echo
echo

# 結果の比較と分析
echo -e "${GREEN}=== Test Results Analysis ===${NC}"
echo

# エラーメッセージの比較関数
compare_error_messages() {
    local test_name="$1"
    local real_file="$2"
    local ft_file="$3"

    echo -e "${YELLOW}$test_name${NC}"

    if [ -f "$real_file" ] && [ -f "$ft_file" ]; then
        echo "Real ping:"
        cat "$real_file"
        echo
        echo "ft_ping:"
        cat "$ft_file"
        echo

        # 基本的な比較
        if grep -q "cannot resolve" "$real_file" && grep -q "cannot resolve" "$ft_file"; then
            echo -e "${GREEN}✓ Both show 'cannot resolve' error${NC}"
        elif grep -q "missing host operand" "$ft_file" && grep -q "usage:" "$real_file"; then
            echo -e "${GREEN}✓ Both show usage/argument error${NC}"
        elif grep -q "socket creation failed" "$ft_file"; then
            echo -e "${YELLOW}! ft_ping shows socket creation error (expected if not root)${NC}"
        else
            echo -e "${RED}✗ Error messages differ significantly${NC}"
        fi
        echo "---"
    else
        echo -e "${RED}✗ Test files not found${NC}"
    fi
    echo
}

# 各テストケースの比較
compare_error_messages "Nonexistent hostname" "test_results/real_ping_nonexistent.txt" "test_results/ft_ping_nonexistent.txt"
compare_error_messages "Invalid IP address" "test_results/real_ping_invalid_ip.txt" "test_results/ft_ping_invalid_ip.txt"
compare_error_messages "No arguments" "test_results/real_ping_no_args.txt" "test_results/ft_ping_no_args.txt"
compare_error_messages "Invalid option" "test_results/real_ping_invalid_option.txt" "test_results/ft_ping_invalid_option.txt"

# 長すぎるホスト名の比較
compare_error_messages "Hostname too long" "test_results/real_ping_long_hostname.txt" "test_results/ft_ping_long_hostname.txt"

echo
# テストケース8: 空文字列のホスト名
EMPTY_HOSTNAME=""
echo -e "${YELLOW}Test 8: Empty hostname${NC}"
echo "Testing: (empty string)"
echo "--- Real ping output ---"
ping "$EMPTY_HOSTNAME" 2>&1 | head -3 | tee test_results/real_ping_empty_hostname.txt
echo
echo "--- ft_ping output ---"
./ft_ping "$EMPTY_HOSTNAME" 2>&1 | head -3 | tee test_results/ft_ping_empty_hostname.txt
echo

echo
# テストケース9: 不正な文字（スラッシュ）を含むホスト名
INVALID_HOSTNAME="invalid/host"
echo -e "${YELLOW}Test 9: Invalid character in hostname${NC}"
echo "Testing: $INVALID_HOSTNAME"
echo "--- Real ping output ---"
ping "$INVALID_HOSTNAME" 2>&1 | head -3 | tee test_results/real_ping_invalid_char.txt
echo
echo "--- ft_ping output ---"
./ft_ping "$INVALID_HOSTNAME" 2>&1 | head -3 | tee test_results/ft_ping_invalid_char.txt
echo

echo
# テストケース10: 非ASCII文字（日本語）を含むホスト名
NONASCII_HOSTNAME="テスト.example.com"
echo -e "${YELLOW}Test 10: Non-ASCII hostname${NC}"
echo "Testing: $NONASCII_HOSTNAME"
echo "--- Real ping output ---"
ping "$NONASCII_HOSTNAME" 2>&1 | head -3 | tee test_results/real_ping_nonascii.txt
echo
echo "--- ft_ping output ---"
./ft_ping "$NONASCII_HOSTNAME" 2>&1 | head -3 | tee test_results/ft_ping_nonascii.txt
echo

echo
# テストケース11: 引数が多すぎる
MANY_ARGS=""
for i in {1..100}; do MANY_ARGS+="127.0.0.1 "; done
echo -e "${YELLOW}Test 11: Too many arguments${NC}"
echo "Testing: 100 arguments"
echo "--- Real ping output ---"
ping $MANY_ARGS 2>&1 | head -3 | tee test_results/real_ping_many_args.txt
echo
echo "--- ft_ping output ---"
./ft_ping $MANY_ARGS 2>&1 | head -3 | tee test_results/ft_ping_many_args.txt
echo

echo
# 比較
compare_error_messages "Empty hostname" "test_results/real_ping_empty_hostname.txt" "test_results/ft_ping_empty_hostname.txt"
compare_error_messages "Invalid character in hostname" "test_results/real_ping_invalid_char.txt" "test_results/ft_ping_invalid_char.txt"
compare_error_messages "Non-ASCII hostname" "test_results/real_ping_nonascii.txt" "test_results/ft_ping_nonascii.txt"
compare_error_messages "Too many arguments" "test_results/real_ping_many_args.txt" "test_results/ft_ping_many_args.txt"

echo -e "${GREEN}=== Error Test Completed ===${NC}"
echo "Test results saved in test_results/ directory"
echo
echo "Summary of expected behaviors:"
echo "1. DNS resolution errors should be handled gracefully"
echo "2. Invalid arguments should show usage information"
echo "3. Socket creation requires root privileges"
echo "4. Error messages should be informative and consistent"
