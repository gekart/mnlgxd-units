# mnlgxd-units
This project contains an effective structure for configure managing logue-sdk unit files and also some of my public KORG minilogue xd units.

## logue-sdk setup

The logue-sdk is included as a git submodule (any changes in that submodule will not be tracked within this project). You need to initialize the sdk be executing setup-logue-sdk-osx.sh on a Mac (on other systems you will need to adopt the file).

## logue-cli setup

Get the logue-cli from https://www.dropbox.com/s/kyjexw6l6ydxzi7/logue-cli-osx-0.07-0b.zip. Unzip that file and move the file logue-cli to the local /usr/local/bin.

## unit structure

Units are kept each in its own folder with their own separate makefiles. By keeping multiple units in this project, you are effectively sharing the logue-sdk among them. Assign a slot to each unit's project.mk in order to automatically upload it to the synth.

## makefile

A global makefile is located at the top level directory, that will execute each individual unit's makefile.
