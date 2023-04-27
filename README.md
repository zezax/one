# Repository: Zezax/One

There is only one repository; so, it is called `one`.

Most of what's here is C++, Python, and shell scripts.
The bias is toward Linux/Unix infrastructure and administration.

The largest sub-project is [RED](quol/red/README.md),
a DFA-based regular expression engine.

Here's a bit of a directory map:

- one
  - [quol](quol/)
    - [red](quol/red/) - regular expression DFA
    - [flume](quol/flume/) - tail many log files and email or update RRD for matching lines
    - [loggle](quol/loggle/) - tail a log file and email matching lines
    - [bin](quol/bin/)
      - hist - histogram utility
      - means - average utility
      - netblock - find smallest CIDR subnet containing a list of IPv4s
      - plotxy - front-end to gnuplot
      - rrd-add-col - add a column to an RRD
    - [sbin](quol/sbin/)
      - allsnap - call mksnap for a bunch of partitions
      - backup-desktop - script to rsync from always-on computer to backup host
      - backup-laptop - script to rsync from laptop to backup host
      - cyrus-backup - back up mail spool using LVM snapshot
      - df-check - email about disks about to fill up
      - dmarc-recv - receive DMARC report email
      - dmarc-rrd - update RRD with DMARC reports from dmarc-recv
      - grey-scan - scan the postgrey database and update RRD
      - hdtemp - update RRD with disk temperatures
      - mksnap - use rsync to make daily snapshots of files via hard links
      - mythtv-transcoder - make h.264 .mp4 files from shows
      - newcert - get new certs from letsencrypt.org automatically
      - rbl-check - check for listing in DNSBL email blocking lists
      - ups-rrd - update RRD with UPS battery stats
