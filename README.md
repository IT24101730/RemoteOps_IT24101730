# RemoteOps

RemoteOps is a remote system monitoring and management tool developed for the IE3090 Network Programming individual assignment.

## Student Details

Registration Number: IT24101730

## Personalisation

- Agent TCP Port: 9410
- Agent Source: agent_730.c
- Controller Source: controller_730.c
- Makefile: Makefile_730
- SID: 0371
- Authentication Token: OPS-1730
- Log File: remoteops_IT24101730.log
- Storage: ./agentfiles/IT24101730/

## Features

- Multiple simultaneous Controllers
- AUTH before other commands
- SYSINFO
- LISTPROC
- Restricted EXEC whitelist
- PUT file upload
- GET file download
- UDP MONITOR START
- UDP MONITOR STOP
- QUIT
- Timestamped logging
- Graceful disconnect handling

## EXEC Whitelist

Only these EXEC commands are allowed:

- DATE
- UPTIME
- DISKFREE
- HOSTNAME
- WHOAMI

Other commands are rejected.

## Build

Compile:

```bash
make -f Makefile_730


