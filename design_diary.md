# Design Diary

## Project
RemoteOps — IE3090 Network Programming

## Registration Number
IT24101730

## Development Summary

I developed RemoteOps step by step using C and BSD sockets on Linux.

I first created a TCP Agent that listens on personalised port 9410 and a Controller that connects to it. I then added reliable newline-based TCP framing because TCP is a byte stream and one recv() call may not contain exactly one command.

Authentication was implemented using the personalised token OPS-1730. All Agent OK and ERR responses include SID:0371.

I implemented SYSINFO using Linux /proc files, LISTPROC using the ps command, and EXEC using a strict whitelist containing DATE, UPTIME, DISKFREE, HOSTNAME, and WHOAMI.

PUT and GET were implemented using exact byte counting so file contents are transferred correctly over TCP. Uploaded files are stored in ./agentfiles/IT24101730/.

UDP monitoring was added using a separate pthread which periodically sends system information to the Controller. MONITOR STOP and QUIT safely stop the monitoring thread.

Logging was added using remoteops_IT24101730.log. A mutex protects log writes when multiple Controllers are connected.

For concurrency, I selected a thread-per-Controller design. The main Agent thread continues accepting TCP connections while each Controller is handled by its own pthread. I tested the Agent with 5 simultaneous Controllers.

I also added SIGPIPE protection, SO_REUSEADDR, graceful disconnect handling, and cleanup of monitoring threads.

The final implementation was tested for authentication, system information, processes, restricted EXEC, file transfer, UDP monitoring, logging, QUIT, disconnect handling, and concurrent Controllers.


# Reflection

Developing the RemoteOps assignment helped me understand how network programming works in practice. Before starting this assignment, I had only a basic understanding of sockets, TCP, UDP, and client-server communication. During the implementation, I learned how an Agent can act as a TCP server and how a Controller can connect as a TCP client.

One of the most important lessons was that TCP is a byte stream. I learned that a single recv() call may return only part of a command or multiple commands together. Because of this, I implemented line framing so the Agent processes complete newline-terminated commands correctly.

I also learned how authentication can be used to protect remote commands. In my implementation, the Controller must send the personalised token OPS-1730 before using other commands. I also used the personalised SID 0371 in Agent responses and UDP monitoring messages.

The PUT and GET features helped me understand exact byte transfer over TCP. Instead of treating file contents like normal text commands, I had to count the exact file size and make sure the correct number of bytes was received or sent.

UDP monitoring helped me understand the difference between TCP and UDP. TCP was used for reliable commands and file transfers, while UDP was used for periodic monitoring messages. I used a separate pthread for monitoring so it could run while the TCP session remained active.

The concurrency requirement was another important learning experience. I changed the Agent to use one pthread per Controller so multiple Controllers could connect at the same time. I tested the Agent with five simultaneous Controllers.

I also added logging, graceful disconnect handling, SIGPIPE protection, and SO_REUSEADDR. These features helped me understand that a network program must handle unexpected situations, not only successful connections.

AI was used as a learning and coding support tool during development. I evaluated the generated suggestions, tested them myself, and kept a prompt log. The assignment improved my confidence in C programming, socket programming, debugging, and testing network applications.
