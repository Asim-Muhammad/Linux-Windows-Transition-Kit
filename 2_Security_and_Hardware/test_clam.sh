sudo systemctl stop clamav-clamonacc
sudo rm -f /var/log/clamav/clamonacc.log
sudo systemctl start clamav-clamonacc
sleep 2
cat /var/log/clamav/clamonacc.log
