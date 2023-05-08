# sensorrd

`sensorrd` is a replacement for the problematic and abandoned `sensord`
component of the `lm-sensors` package.
When it disappeared from Debian, I wrote `sensorrd` to do what I was
using `sensord` for:

- To log alarms to syslog
- To populate values in RRD

`sensorrd` does all its logging to syslog (facility daemon).
The alarms are formatted to match what `sensord` did.

## Usage
```
sensorrd [<rrdpath>]
```

In most cases, though, one should use systemd to run it.
If the RRD path is omitted, it only logs to syslog.

## RRD

If the specified RRD doesn't exist, `sensorrd` will create it
based on the available sensors.
The RRD should be compatible with ones created by and/or used by
`sensord`.
The method of RRD updates is more robust than `sensord`,
as it does not depend on the order of each DS in the RRD.
The default RRAs are as follows:
```
RRA:AVERAGE:0.5:1:1800    -- 30 hours of minutes
RRA:AVERAGE:0.5:5:2880    -- 10 days of 5 minutes
RRA:AVERAGE:0.5:60:960    -- 40 days of hours
RRA:AVERAGE:0.5:1440:3653 -- 10 years of days
```

## Installation

The basic procedure I used is:
```
make MODE=opt
sudo cp sensorrd /usr/local/sbin
sudo cp sensorrd.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl start sensorrd
sudo systemctl enable sensorrd
```

### Dependencies

At least on Debian, these packages are needed for compilation:

- `libsensors4-dev`
- `librrd-dev`
