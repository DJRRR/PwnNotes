# Interactive 

## Send and Receive File
```
Example: upload a file(e.g. binary) to the challenge server
My server:
nc -vlp 9876 < ./my_bin
Challenge server:
nc my_server_ip 9876 > ./my_bin
```
OR
```
Example: upload a file(e.g. binary) to the challenge server
base64 ./my_bin |tmux loadb -
cat <<EOF > ./new_bin (cat until EOF)
tmux: ctrl+a+}
base64 -d ./new_bin
```

