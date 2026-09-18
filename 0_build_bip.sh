echo "build bip server"
gcc bip_server.c -o bip_server.exe

echo "build bip server daemon" 
# gcc bipd_tls.c -o bipd.exe -ljson-c

# Single tx
gcc bipd_tx1.c -o bipd.exe -ljson-c

# Continuous tx for LGU test 
#gcc bipd_tx2.c -o bipd.exe -ljson-c

echo "build bip client"
gcc bip_client.c -o bip_client.exe
