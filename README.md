# one

This is the first and currently only repository; so it's called "one".

Most of what's in here is C++, Python, and shell scripts used to
administer individual Linux servers.

Here's a bit of a map:
```
+ one
  + quol
    + bin
      - hist - histogram utility
      - means - average utility
      - netblock - find smallest CIDR subnet containing a list of IPv4s
      - plotxy - front-end to gnuplot
      - rrd-add-col - add a column to an RRD
    + sbin
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
    + loggle - tail a log file and email matching lines
    + flume - tail many log files and email or update RRD for matching lines
```
