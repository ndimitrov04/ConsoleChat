# ConsoleChat
Basic client/server combo program made in C++  

How to get this to run properly:  
1 - Run Server/x64/Debug/Server.exe to start the server application  
2 - Run 2 or more instances of Client/x64/Debug/Client.exe  

As long as the instances of the client and the instance of the server are on the same local network, the program will work. They may also be on the same computer, for more convienient of testing.  

Usage instructions:  
1 - When you open a chat client, it will ask for a name and a password.  
1.1 - When a name which does not exist in the server's login database (accounts.ngc) is entered, the server will request a password and a new account with that name will be created.  
1.2 - If a name which exists in the login database is entered, the server will ask for that account's password, letting the user in if the right password is given and kicking the user out if a wrong password is given.
2 - Every message after the login/registration will be treated as a text message and sent and displayed to all other clients.

![image](https://github.com/ndimitrov04/ConsoleChat/assets/165305475/40bb70ea-994a-48a0-9390-227a7a1aed3a)
