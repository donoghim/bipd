# echo "------- show BIP log at young's VM Ubuntu"
# sudo tail -f /var/log/bipd.log | grep bipLog
# sudo tail -f /var/log/bipd.log

echo "------- show BIP log $1 line at QSS machine"
sudo journalctl -u bipd | tail -n 100
# sudo journalctl -u bipd | tail -n $1 

