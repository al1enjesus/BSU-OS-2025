echo "=== Testing Kernel Modules Concurrent Access ==="
echo "Note: Run with sudo for write operations"

echo -e "\n1. Checking if modules are loaded..."
lsmod | grep -E "(hello_module|proc_module|stats_module)" || echo "Modules not loaded - load them first"

echo -e "\n2. Testing multiple process reading from /proc/sys_stats..."
for i in {1..5}; do
    (cat /proc/sys_stats > /dev/null 2>&1 && echo "  Process $i: read stats OK") &
done
wait

echo -e "\n3. Testing concurrent writing to /proc/my_config..."
for i in {1..3}; do
    (echo "concurrent_test_$i" | sudo tee /proc/my_config > /dev/null 2>&1 && echo "  Process $i: write config OK") &
done
wait

echo -e "\n4. Testing read during write..."
(echo "read_during_write" | sudo tee /proc/my_config > /dev/null 2>&1) &
(cat /proc/my_config > /dev/null 2>&1 && echo "  Read during write: OK") &
wait


echo -e "\n5. Testing rate limiting (should see warnings in dmesg)..."
for i in {1..5}; do
    echo "rapid_write_$i" | sudo tee /proc/my_config > /dev/null 2>&1
    sleep 0.05
done

echo -e "\n6. Testing input validation..."
echo -e "normal_text" | sudo tee /proc/my_config > /dev/null 2>&1 && echo "  Normal text: OK"
echo -e "text_with\ttabs" | sudo tee /proc/my_config > /dev/null 2>&1 && echo "  Text with tabs: OK"
echo -e "text_with\nnewlines" | sudo tee /proc/my_config > /dev/null 2>&1 && echo "  Text with newlines: OK"

echo -e "\n7. Final state check..."
echo "Current /proc/my_config content:"
cat /proc/my_config
echo -e "\nCurrent system stats:"
cat /proc/sys_stats | head -3

echo -e "\n=== Concurrent access test completed ==="
echo "Check dmesg for any kernel warnings or errors: sudo dmesg | tail -10"


