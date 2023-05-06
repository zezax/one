# Repository: Zezax/One

There is only one repository; so, it is called `one`.

Most of what's here is C++, Python, and shell scripts.
The bias is toward Linux/Unix infrastructure and administration.

The largest sub-project is [RED](quol/red/README.md),
a DFA-based regular expression engine.

Here's a high-level directory map:

- [quol](quol/)
  - [red](quol/red/) - regular expression DFA
  - [flume](quol/flume/) - tail many log files and email or update RRD for matching lines
  - [sensorrd](quol/sensorrd/) - lm-sensors logger to syslog and RRD
  - [loggle](quol/loggle/) - tail a log file and email matching lines
  - [bin](quol/bin/) - user-level scripts
  - [sbin](quol/sbin/) - sysadmin scripts
