# echo "------- show BIP log at young's VM Ubuntu"
# sudo tail -f /var/log/bipd.log | grep bipLog
# sudo tail -f /var/log/bipd.log

echo "------- show BIP log at QSS machine"
journalctl -u bipd -f
# journalctl -u bipd -n 40

