# Loggle - Syslog Watcher/Alerter

This is a modest C++ program that combines the functionality of
`tail -F` and `grep` with a buffer and email.  I wrote this so I could get
email alerts when my server got too hot.  I was already running `lm-sensors`
and `sensord`, but they don't send email.  This code is designed to be
lightweight and efficient.  It doesn't use the shell and doesn't spawn
processes unless it's sending mail.

The code is `loggle.cpp` and there is a `Makefile`.
There's also a systemd service definition and a defaults file.

The functionality of Loggle is largely subsumed by [Flume](../flume/).

## Command Line Options

- `-i` - ignore case when matching (must come first)
- `-e <regex>` - pattern to look for (must supply one or more)
- `-m <email>` - address to receive alerts (required)
- `-s <str>` - subject of email (default: LOGGLE)
- `-f <path>` - file to watch (default: /var/log/syslog)
- `-t <num>` - seconds between email messages (default: 300)
- `-b <num>` - maximum batch size for email (default 100)
- `-h <num>` - hit count before triggering alert (default: 1)
- `-c <num>` - line count before zeroing hits (default 2^31-1)
- `-fg` - do not become a daemon
