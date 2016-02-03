# SecureGateWay
A communication tunnel using UDP and OpenSSL, self-created protocol, has the same function as SSH, available for further implementations of other usage.

When people use SSH in a web environment which Man-in-Middle attack might happen, the fact that they are using ssh can be compromised

Even worse, sometimes your tcp connection of SSH can be broken by the middle man or ISP using TCP RESET flag

Even when using obfuscated-openssh, there are certain characteristics of SSH after the protocol head, when authentiating the password. And ISP or middle man can break the tcp connection when those are detected.

So I wrote this Project, created a protocol based on UDP, which is immune to RESET flag and has almost no characteristic.



HOW TO INSTALL&USE:

client:

  put the client side in your computer (already tested in Mac OS X 10.10, should also work in Linux)
  
  cd Release

  sudo make install

server:
  
  put the server side in your server (already tested in Ubuntu 14.04, should also work in other Linux)
  
  cd Release
  
  sudo make install
  
  
First you need to go to the server side and generate a clientkey file,

SGWD -g clientkey

And copy the clientkey into your client computer.

Then start the server side

SGWD -p [port]

If you want the server side program to run permanently in background

nohup SGWD -p [port] 2>&1>SGWD_out.log

On the client side,

SGW -s [server ip address] -p [server port] -l [path to clientkey]

And enjoy!


DETAILS:

This is a communication tunnel using UDP.

By default, the server activates a /bin/sh process, and the client side controls the Pseudoterminal, the same as SSH

But you can also rewrite sendUnencryptedData(), processDecryptedData() on the server side, and sendUnencryptedMessage(), processDecryptedData() on the client side to implement the communication protocal in the way you like.

I just finished these codes recently, BUGs are very possible.

