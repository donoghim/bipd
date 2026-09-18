# service description file
# /etc/systemd/system/bipd.service
echo "-------- bip action: $1"
if [ $1 == "load" ]; then
  sudo systemctl daemon-reload 
elif [ $1 == "start" ]; then
  sudo systemctl start bipd
elif [ $1 == "status" ]; then
  sudo systemctl status bipd
elif [ $1 == "restart" ]; then
  sudo systemctl restart bipd
elif [ $1 == "stop" ]; then
  sudo systemctl stop bipd
elif [ $1 == "enable" ]; then
  sudo systemctl enable bipd
elif [ $1 == "disable" ]; then
  sudo systemctl disable bipd
else
  echo "unknown action: $1"
fi
