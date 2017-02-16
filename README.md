# SIMPLE C&C BOTNET
This hackpack will walk you through implementing a basic framework that fundamentally characterizes a botnet. It might help if you've worked with C before. If you enjoyed the tutorial, be sure to star on the repo!

Note: Don't use anything you learn here for malevolent purposes. This hackpack is solely a case study of botnets for educational purposes. The concepts learned in this hackpack have far reaching use cases (basically anything having to do with networking). Most importantly, this hackpack is meant to be tested and deployed locally (so please don't share anything you build related to this hackpack with other hackers). Privacy is important so please respect it.

##What is a botnet?
Before you build a botnet, its important to understand what a botnet is. A botnet is a network of computers that are capable of recieving commands remotely and deploying them locally. Optionally, they can choose to relay information back to other nodes in the network. They've been used for everything from Distributed Denial of Service attacks to widely deploying spyware. You may have heard of many botnets in the past. Most prominent are perhaps the Mirai and Gameover Zeus which controlled 3.8 thousand and 3.6 million IoT devices respectively. There is alot of variance between how botnets implement certain tasks. But in order to build our botnet successfully, we need to ensure the following features in our working network.

Our botnet should:

1. Include a master node that controls all other nodes on the network
2. Deploy disguised malware/slave nodes on host computers
3. Transmit commands from the master node to the slave node, execute, and return an output back to master 

This structure is characteristic of what's known as Command & Control botnets. These botnets have one master server and many slave servers. However, this styles of botnets is anitquated and can easily be taken down by cutting the master domain. More recent and sophisticated botnets follow peer-to-peer architecture, where admin rights are distributed across either all nodes or a subset of nodes in the network. These botnets pose a large headache to security experts because there's no central point of control and can grow to millions of nodes. Taking down such botnets is an interesting read by itself. However, for the purposes of this hackpack, lets keep things simple. We'll implement a simple slave node for a C&C botnet.

![alt tag](https://www.usenix.org/legacy/event/sec08/tech/full_papers/gu/gu_html/img1.png)



##Implementation

###Master
This hackpack will primarily deal with implementing the client malware. For the master server, we can use an open-source TCP server callet Netcat. Netcat has nothing to do with botnets. It's just a convienient, established tool that we can re-purpose to send text packets to an from clients (which is all a master really is). I've slightly adjusted the netcat server and compiled it to a binary called "master". No more work needed here! Our master is ready for use.

###Slave
Lets move on to the more interesting bit: recieving and executing remote commands (We'll worry about disguising our malware later). The goal here is to make our slave node as simple as possible and adhere to the requirements detailed above. Note that many constants have been defined in lib/macros.h so feel free to use those too.

### 1. Initiation
Open the bot.c file. When starting a new node in our server, we should probably name it so master knows which client is server. There are many naming conventions that can be used. Using an IP address is probably the best because it's a unique identifier for each client. However, to make things more readable for regular humans, lets use the computer's username. Using the C function `getenv()` with argument `"USER"` returns whatever the computer has stored in the USER environment variable. This is one place the user's username is stored so lets use that. Also, now that your slave is running, lets find master. To do this, we must know our master's IP address. Every network device has an IP address. It's was responsible for identifying another node location addressing. Further, master can have many servers running at different ports. So, not only do we have to connect to master, but we also should also use the correct port. This port is specified on master and can be changed. In this hackpack, we want to test locally. So, we'll use your computer as our network. Every computer's local IP address (which "localhost" also resolves to) is `"128.0.0.1"`. In master, I specified it to run on port `9999`. With these three things (master ip address, master port, and slave name), we can start a communications pipeline between server and client called a channel. Pass these three arguments into the C function `init_channel()` to create a channel. Then, we need to allocate some space on the stack to hold incoming messages. Lets be quite generous here and use around 10KB of stack space. Call the stack pointer `msg` Allocated  Lastly, there's a `printf` statement to just indicate that all is going swell.


```
char* name = //Get the client's username and store it in name
int channel = //initiate a channel given SERVER, PORT, and name;
//Allocate stack space of size CMD_LENGTH to hold data of type char. Call the stack pointer msg
printf("%s joining the botnet\n", name);
```

### 2. Listening for messages
Once the slave is connected to the master, it needs to constantly be listening for messages and act immediately upon a command. So, lets use an infinite while loop to recieve and parse these messages. In bot.c, below the `printf` statement, add an infinite while loop that calls two functions: `recieve()` and `parse()` in that order. Both functions take the `channel` and `msg` stack buffer as arguments. You can find their function signatues in lib/utils.h. That's pretty much it for bot.c. Now, let's implement these two functions. Open connect.c and find the unfinished parse function. 

### 3. Executing commands

### 4. Disguising your malware

### 5. Extensions

