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
