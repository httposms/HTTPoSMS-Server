```
HTTPOSMS-SERVER(1)                             General Commands Manual                             HTTPOSMS-SERVER(1)

NAME
       httposms-server - HTTP tunnel over SMS server

SYNOPSIS
       httposms-server [OPTIONS]...  [DEVICE]

DESCRIPTION
       Using  the GPRS/GSM modem represented by DEVICE tunnel HTTP traffic over SMS from requests to the modem By de‐
       fault the process will attempt to daemonize, and write its' output to the file "/tmp/httposms.log"

OPTIONS
       --version, -v
              Print the package version and exit

       --no-daemon, -n
              Do not attempt to daemonize the process

       --log-level, -l
              The log severity [DEBUG, INFO, WARN, FAIL]

       --device, -d
              The device file for the GPRS/GSM module, defaults to "/dev/ttyAMA0"

       --help, -h
              Print this message and exit

BUGS
       Send all bug reports https://github.com/httposms/httposms-server

                                          httposms-server-v0.0.0-prerelease                        HTTPOSMS-SERVER(1)
```
