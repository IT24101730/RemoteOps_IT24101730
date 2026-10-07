## Entry 1 — Understanding the Assignment
Date: 01/10/2026
Tool: ChatGPT

Prompt:
"I’m ready to start the IE3090 RemoteOps assignment. Consider as im a beginner. Start from Step by step and guide me carefully. "

How I used the output:
I used the explanation to understand the RemoteOps system, the roles of the Agent and Controller, the use of TCP and UDP, and the main assignment requirements.

What I checked or changed:
I checked the explanation with the assignment brief before continuing because I wanted to make sure I was following the actual requirements.

## Entry 2 — Personalisation Calculations
Date: 01/10/2026
Tool: ChatGPT

Prompt:
"Continue the next steps and calculate my personalised values using registration number IT24101730."

How I used it:
I used ChatGPT to calculate the personalised values needed for my program. I then used these values when creating my project.

Values I used:
- Agent port: 9410
- Agent file: agent_730.c
- Controller file: controller_730.c
- Makefile: Makefile_730
- SID: 0371
- Authentication token: OPS-1730
- Log file: remoteops_IT24101730.log
- Storage path: ./agentfiles/IT24101730/
- ZIP file: IE3090_IT24101730.zip

What I checked or changed:
I checked the calculations myself using the formulas in the assignment brief before using them.

## Entry 3 — Development Environment Setup
Date: 03/10/2026
Tool: ChatGPT

Prompt:
"I already work with CentOS 10 in my VM. Can I use this for the assignment?"

How I used the output:
I confirmed that CentOS 10 is suitable because the assignment requires C, gcc, Linux, and standard BSD sockets. I used the suggested commands to verify gcc, make, git, and ss.

Changes / evaluation:
I used CentOS-specific commands instead of Ubuntu commands because my development environment is CentOS 10.

## Entry 4 — Creating the Project Structure
Date: 03/10/2026
Tool: ChatGPT

Prompt:
"Continue with Step 4 and help me create the RemoteOps project folders and personalised files."

How I used it:
I followed the commands to create my RemoteOps project folder, C source files, Makefile, README, design diary, prompt log and the folder for uploaded files.

What I checked or changed:
Before creating the files, I checked that the filenames and storage folder matched my registration number.

## Entry 5 — Git and GitHub Setup
Date: 04/10/2026
Tool: ChatGPT

Prompt:
"Help me initialize Git, create my first commit, connect my GitHub repository, and fix the GitHub authentication and repository history issues."

How I used it:
I used the instructions to set up Git for my project and connect it to my GitHub repository. I also had some problems with authentication and the repository history, so I used ChatGPT to understand the errors and fix them.

What I checked or changed:
I did each command one at a time and checked the results using git status, git log, and git remote -v. After everything worked, I pushed the project to GitHub

## Entry 6 — Personalised Makefile
Date: 05/10/2026
Tool: ChatGPT

Prompt:
"Help me create and test Makefile_730 for agent_730.c and controller_730.c."

How I used the output:
I created a Makefile that compiles both programs with gcc using -Wall, -Wextra, and -pthread. I also tested the build, ran both placeholder programs, tested the clean target, committed the changes, and pushed them to GitHub.

Changes / evaluation:
I ran make and both executables myself and confirmed the expected output before committing the Makefile.

## Entry 7 — Basic TCP Agent
Date: 05/10/2026
Tool: ChatGPT

Prompt:
"Continue the next Steps and help me build the basic TCP Agent on my personalised port 9410."

How I used the output:
I used the guidance to replace the Agent placeholder with a basic TCP server using socket(), bind(), listen(), and accept().

Changes / evaluation:
I compiled the code using my personalised Makefile, ran the Agent, verified that it listened on port 9410 using ss, and tested a basic TCP connection before committing the code.

## Entry 8 — TCP Controller Connection
Date: 05/10/2026
Tool: ChatGPT

Prompt:
"Continue Step 8 and help me build controller_730.c as the TCP client."

How I used the output:
I used the guidance to create the Controller TCP client using socket(), inet_pton(), and connect(). The Controller was configured to connect to the Agent at 127.0.0.1 on my personalised TCP port 9410.

Changes / evaluation:
I compiled the Agent and Controller using Makefile_730 and tested them in two terminals. 
I confirmed that the Controller displayed a successful connection message and that the Agent displayed the Controller IP address. 
I also checked the Git status and confirmed that the Controller connection code was committed and pushed successfully.

## Entry 9 — Reliable TCP Line Framing
Date: 06/10/2026
Tool: ChatGPT

Prompt:
"Continue Step 9 and help me implement reliable TCP line framing."

How I used the output:
I used the guidance to implement a buffered recv_line() function in agent_730.c. The function reads newline-terminated TCP commands without assuming that one recv() call contains exactly one full command.

Changes / evaluation:
I tested two commands sent in one TCP stream and confirmed that the Agent separated them correctly. I also tested a command sent in separate pieces and confirmed that the Agent waited until the newline arrived before returning the complete command.

What I learned:
TCP is a byte stream, so one recv() call may return part of a command or multiple commands. The application must perform its own message framing using a delimiter such as '\n'.

## Entry 10 — Authentication
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement AUTH OPS-1730 with SID:0371."

How I used the output:
I used the guidance to add authentication to the Agent and include SID:0371 in the responses.

Changes / evaluation:
I tested successful AUTH, failed AUTH, and SYSINFO before authentication.

## Entry 11 — SID Response Helper
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me make SID responses consistent using a helper function."

How I used the output:
I added send_response() so Agent responses automatically include SID:0371 and a newline.

Changes / evaluation:
I re-tested successful AUTH, failed AUTH, and AUTH_REQUIRED responses.

## Entry 12 — SYSINFO
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement the SYSINFO command."

How I used it:
I used Linux /proc files to return CPU load, memory usage and uptime.

Evaluation:
I tested SYSINFO after AUTH and before AUTH.

## Entry 13 — LISTPROC
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement the LISTPROC command."

How I used it:
I used ps through popen() to return a snapshot of running processes.

Evaluation:
I tested LISTPROC after authentication and before authentication.

## Entry 14 — EXEC Whitelist
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement the restricted EXEC whitelist."

How I used it:
I added support for DATE, UPTIME, DISKFREE, HOSTNAME and WHOAMI only.

Evaluation:
I tested EXEC DATE successfully and confirmed EXEC LS was rejected.

## Entry 15 — PUT Upload
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement PUT file upload."

How I used it:
I added exact byte-counted upload to ./agentfiles/IT24101730/.

Evaluation:
I uploaded a test file and verified the stored file was identical.

## Entry 16 — GET Download
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement GET file download."

How I used it:
I added exact byte-counted GET transfer and Controller-side file saving.

Evaluation:
I downloaded the uploaded file and verified it was byte-for-byte identical using SHA-256 and cmp.

## Entry 17 — UDP Monitoring Start
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement MONITOR START over UDP."

How I used it:
I added a UDP monitoring thread that sends SYSINFO statistics every 2 seconds.

Evaluation:
I tested MONITOR START on UDP port 9500 and received repeated datagrams containing SID:0371.

## Entry 18 — UDP Monitoring Stop
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement MONITOR STOP."

How I used it:
I added logic to stop and join the UDP monitoring thread.

Evaluation:
I confirmed UDP messages stopped after MONITOR STOP and the Agent returned OK MONITOR_STOPPED.

## Entry 19 — QUIT
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me implement QUIT."

How I used it:
I added logic to stop active UDP monitoring, send OK BYE, and close the TCP session cleanly.

Evaluation:
I tested QUIT normally and while UDP monitoring was active.

## Entry 20 — Logging
Date: 07/10/2026
Tool: ChatGPT

Prompt:
"Help me add logging to remoteops_IT24101730.log."

How I used it:
I added timestamped logging for connections, commands and file transfers.

Evaluation:
I tested several commands and checked the generated personalised log file.
