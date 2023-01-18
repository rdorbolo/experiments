Use the curl command to PUT (upload the files)

 curl -v -T index.html  192.168.1.27/index.html

busybox test server

 busybox httpd -p 8080 -f -v
