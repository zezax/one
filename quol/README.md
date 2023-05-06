# Project: Zezax/One/Quol

The name of this project directory is `quol`, because it needed a name.
There is an obscure animal called the quoll,
but I wanted a four-letter name becasuse I'm lazy.
It sounds like the beginning of "quality".

The largest sub-project is `RED`, the DFA regex engine.
Much of the rest is biased toward Debian using systemd.
There is a fair amount of RRD.

Directory map:

- [red](red/) - regular expression DFA library
- [flume](flume/) - tail many log files and email or update RRD for matching lines
- [sensorrd](sensorrd/) - lm-sensors logger to syslog and RRD
- [loggle](loggle/) - tail a log file and email matching lines
- [bin](bin/)
  - `hist` - histogram utility
  - `means` - average utility
  - `netblock` - find smallest CIDR subnet containing a list of IPv4s
  - `plotxy` - front-end to `gnuplot`
  - `rrd-add-col` - add a column to an RRD
  - `rrd-reorder-col` - add, drop, and/or reorder RRD columns
- [sbin](sbin/)
  - `allsnap` - call `mksnap` for a bunch of partitions
  - `backup-desktop` - script to `rsync` from always-on computer to backup host
  - `backup-laptop` - script to `rsync` from laptop to backup host
  - `cyrus-backup` - back up mail spool using LVM snapshot
  - `df-check` - email about disks close to filling up
  - `dmarc-recv` - receive DMARC report email
  - `dmarc-rrd` - update RRD with DMARC reports from `dmarc-recv`
  - `grey-scan` - scan the postgrey database and update RRD
  - `hdtemp` - update RRD with disk temperatures
  - `lv-grow` - increase the size of an LVM logical volume
  - `mksnap` - use `rsync` to make daily snapshots of files via hard links
  - `mythtv-transcoder` - make h.264 `.mp4` files from shows
  - `newcert` - get new certs from `letsencrypt.org` automatically
  - `rbl-check` - check for listing in DNSBL email blocking lists
  - `ups-rrd` - update RRD with UPS battery stats
