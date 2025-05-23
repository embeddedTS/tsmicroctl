# tsmicroctl
This project is used to control the supervisory microcontroller on these platforms:
* TS-7100
* TS-7180
* TS-7800-V2

# Build instructions:
Install dependencies:

    apt-get update && apt-get install git build-essential meson pkgconf libgpiod-dev -y

Download, build, and install on the unit:

    git clone https://github.com/embeddedTS/tsmicroctl.git
    cd tsmicroctl
    meson setup builddir
    meson compile -C builddir
    meson install -C builddir

# Usage
    Usage: tsmicroctl [OPTION] ...
    embeddedTS microcontroller utility
    
      -e, --enable             Enable charging
      -d, --disable            Disables any further charging
      -w, --wait-pct <percent> Enable charging and block until charged to a set percent
      -b, --daemon <percent>   Monitor power_fail# and issue "reboot" if the supercaps fall below percent
      -i, --info               Print current information about supercaps
      -c, --current <mA>       Permanently set max charging mA
      -s, --sleep <seconds>    Turns off power to everything for a specified number of seconds
      -h, --help               This message
