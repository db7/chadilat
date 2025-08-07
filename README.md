# Cheap and Dirty Logic Analyzer with Teensy

This is a cheap and dirty logic analyzer built with the Teensy 2.0 of my
ErgoDox keyboard.  The goal of this project was to confirm whether the IC of the
SurnQiee Trackpoint mouse talks PS/2 protocol with the microcontroller, and if
yes, which pin is the clock and which is the data.

## Hardware requirements:

- Teensy 2.0
- USB cable
- a few wires soldered to Port B
- UNIX host

## Software requirement

- avr-gcc
- make
- raylib
- `teensy_loader_cli`

## Usage

First, flash the firmware. Next, connect/solder the pins from Port B on the
target circuit. It might be useful to put a 1kΩ in between. Ensure that grounds
are connected and that target circuit operates at 5V as well.

Turn all on and connect to the computer USB. Start the `reader` like this:

    reader/reader /dev/ttyU0 | tee output.dat

To visualize the results use the `viewer`:


    cat output.dat | viewer/viewer

You can also pipe from `reader` into `viewer`.

You might need to mask the pins you want to read in the firmware or reader. Also
in the source of the viewer, you might set the pins to be plotted. There are no
configuration files, you have to check the source code.

In the specific circuit I am interested, I have connected the wires like this.

```
  Teensy 2.0                                      Mouse IC
+--------------+                             +----------------+
|          GND +-------------------+---------+ GND            |
|          PB0 +--- 1kΩ -----------|-+-------+ X0             |
|          PB1 +--- 1kΩ -----------|-|-+-----+ X1             |
|          PB2 +--- 1kΩ -----------|-|-|-+---+ X2             |
|              |                   | | | |   |                |
+----[USB]-----+                           --+ VCC            |
                                  Micro    --+ BTN1           |
                               Controller  --+ BTN2           |
                                  [USB]    --+ BTN3           |
                                             +----------------+
```
