# Flume - Fast Log Unified Monitor & Emailer

A flume helps process logs, fast.  This flume updates `RRD` databases and
sends email based on the contents of logs.  It uses multiple threads to
read multiple logs simultaneously.  Flume uses regular expressions to
identify important lines in logs.  Files are read from the end in the
same way as `tail -F`.  Flume is quite suitable for system logs.

Configuration is done via a single tab-delimited text file.  Each line is
a directive.  The first value of each line is the verb, which may be `set`
or `scan`.

- `set` - provide configuration of flume components
- `scan` - identify an important pattern in a specific log file

Each `set` has the format:
```
set <TAB> <variable> <TAB> <value>
```

Each `scan` has the format:
```
scan <TAB> <file> <TAB> <regex> <TAB> <sink> ...
```

The defined sinks are:

- `ignore` - does nothing
- `rrd` - updates an RRD database
- `mail` - sends email

Additional arguments are:

- `rrd <TAB> <rrd_path> <TAB> <ds_name> <TAB> <increment>`
- `mail <TAB> <address> <TAB> <subject> <TAB> <message>` 

## Examples
```
set <TAB> actor.debug <TAB> 1
set <TAB> rrd.interval <TAB> 60
scan <TAB> /var/log/auth.log <TAB> sshd.*Invalid user <TAB> rrd <TAB> /var/log/example.rrd <TAB> invalcnt <TAB> 1
scan <TAB> /var/log/daemon.log <TAB> (?i)temperature alarm <TAB> mail <TAB> name@example.com <TAB> ALERT <TAB> \*
```
The regular expression is of the ECMAScript type, as implemented by
`std::regex`.  If it is preceded with the four characters `(?i)`, then it will
be interpreted as case-insensitive.

Substitution is performed on the remainder of the line.  The special sequence
backslash-star (`\*`) means the entire log line that matched.  Otherwise,
backslash-digit escapes such as `\1` or `\2` stand for the respective matched
parenthesized groups as per `sed` back-references.

### RRD

Note that flume does not create RRDs.  The included script `create-rrds`
may be adapted for this purpose.  Similarly, the `make-cgi` script might
be useful to create web pages with RRD graphs.

### Installation

Flume was developed on Debian, utilizing systemd.  Installation was as
follows:
```
cd src
make MODE=opt clean all
sudo cp flume /usr/local/sbin
cd ..
sudo cp systemd/flume.service /etc/systemd/system
sudo cp example.tsv /etc/flume.tsv
sudo emacs /etc/flume.tsv
sudo scripts/create-rrds
sudo systemctl daemon-reload
sudo systemctl start flume.service
sudo systemctl enable flume.service
```

### Settings

The default settings are probably fine.
Here is a list with descriptions and defaults for documentation purposes:

- `actor.debug` - debug output level for actor thread (0)
- `mail.debug` - debug output level for email sink (0)
- `mail.limit` - maximum messages of a given type to buffer before sending (100)
- `mail.sleep` - seconds between mail queue checks (10.0)
- `mail.interval` - maximum time a message can be buffered before sending (300.0)
- `rrd.debug` - debug output level for RRD sink (0)
- `rrd.interval` - seconds between RRD updates (60.0)
- `rrd.prefix` - string to put before rrd_path (/var/log/)
- `tailer.bufsize` - read buffer size; maximum bytes per line (8192)

### Implementation

Flume spawns a thread for each distinct log file to be scanned.  Each of
those threads performs the "tail" operation and checks each line against
all regular expression patterns defined for that log file.  When a match
is found, matching for that line stops.  Thus, the order of lines in the
TSV configuration file matters, and the `ignore` sink can be useful.

Each type of sink has its own thread.  This tends to reduce race conditions
when updating RRDs, and allows the `mail` sink to accumulate messages over
time.  The `mail` sink buffers email separately, based on address-subject
pairs.

Communication between tailers and sinks happens via a queue, mediated by
an actor.  Tailers use triggers to determine when a log line is interesting
and what to do with it.
