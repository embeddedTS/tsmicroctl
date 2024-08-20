# tsmicroctl
This project is used to update the manage the supervisory microcontroller on the:
* TS-7100
* TS-7180
* TS-7800-V2

# Build instructions:
Install dependencies:

    apt-get update && apt-get install git build-essential meson -y

Download, build, and install on the unit:

    git clone https://github.com/embeddedTS/tsmicroctl.git
    cd tsmicroctl
    meson setup builddir
    cd builddir
    meson compile
    meson install

# Usage
    Usage: tsmicroctl [OPTION] ...
    embeddedTS microcontroller utility
    
      -e, --enable             Enable charging
      -d, --disable            Disables any further charging
      -w, --wait-pct <percent> Enable charging and block until charged to a set percent
      -b, --daemon <percent>   Monitor power_fail# and issue "reboot" if the supercaps fall below percent
      -i, --info               Print current information about supercaps
      -c, --current <mA>       Permanently set max charging mA (default 100mA, max 1000)
      -s, --sleep <seconds>    Turns off power to everything for a specified number of seconds
      -h, --help               This message
