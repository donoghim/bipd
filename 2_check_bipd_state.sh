echo "-------- check bipd process"
ps -ef | grep -w -H --color bipd.exe

echo "-------- check bipd listening"
sudo netstat -npl | grep -w -H --color bipd.exe

echo "-------- check port 22001 usage"
sudo netstat -npl | grep -w -H --color 22001
