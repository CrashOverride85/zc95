# Building

## Docker
If you just want to build the current dev release, the quickest and easiest way to compile the firmware is to use the docker image ([source](../misc/Docker/)).

Steps to build the dev branch, adjust paths as required:
```
cd /data
git clone -b dev https://github.com/CrashOverride85/zc95.git --recurse-submodules
docker run -v /data/zc95:/src --rm ghcr.io/crashoverride85/zc95-build:1.1.0
```
All going well, complied firmware images should appear in /data/zc95/source/CompiledUF2/

### pico2
This project has been designed around and tested with the original pico (rp2040). However if you really want to build for the pico2, add `pico2` as the first and only command line argument to the container, i.e.:
```
docker run -v <path to repo>:/src --rm ghcr.io/crashoverride85/zc95-build:1.1.0 pico2
```
I do not recommend using pico2's for the zc95:

- This project is likely going to continue to target the original pico(w) for as long as it is readily available in order to remain compatible with exiting zc95's, so the advantages of the pico2 (and there are many) will be wasted
- It doesn't include the bootloader; so firmware upgrades via serial - and likely later from the UI - won't be possible
- I'm not going to be routinely testing future changes even build for the pico2, much less work as intended
- I've barely tested it. On 2026/04/16, it does build from the dev branch, boots up, and appears functional at first glace 🤷‍♂️️

You have been warned.

## Local setup

Quick start building notes for Linux, tested on a fresh install of Debian Bookworm, but should be very similar for any Debian derived distro.
For other environments, see the [pico getting started guide][gs].

This is the bare minimum to get to the point where you can build a binary to copy to the Pico.

### Install build tools
```
apt-get install git cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib python3 python3-pycryptodome
```

### Setup pico-sdk
```
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=`pwd`
cd ..
```

### Get zc95 source & build
```
git clone https://github.com/CrashOverride85/zc95.git --recursive
cd zc95/source/zc95/
cmake .
make

cd ../zc624/
cmake .
make
```
This should result in the two pico binaries being built that can be copied to the pico. 
* zc95/source/zc95/zc95.uf2 - for main board
* zc95/source/zc624/OutputZc.uf2 - for output board

## Debugging
For small fixes/changes/tweaks the above is fine, but requires manually copying to Pico via USB, and doesn't allow for debugging, so very quickly gets tedious for larger changes.

The pico-sdk integrates really well with Visual Studio Code & an SWD debugger, allowing single keypress build & upload, along with setting breakpoints, stepping through code, etc., so that would be my recommendation for a development environment. 
For the SWD debugger, I'm using a J-Link as that's what I had, but a [Picoprobe][pp] seems to be the Raspberry Pi recommend option, and well documented elsewhere.

Describing that setup is documented in the [pico getting started guide][gs], see Chapter 7 & Appendix A.

It's probably worth noting that the code is using both cores, so bare that in mind if unexpected things happen when debugging, setting break points, etc.

[gs]: https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf
[pp]: https://www.raspberrypi.com/products/debug-probe/

## BTStack patch
In order to get one of the bluetooth remotes to work (#7 in [the bluetooth notes](./Bluetooth.md), and potentially later ones), in `pico-sdk/lib/btstack/src/ble/sm.c` I needed to comment this out:
```
        default:
            // Unexpected PDU
            log_info("Unexpected PDU %u in state %u", packet[0], sm_conn->sm_engine_state);
        //    sm_pdu_received_in_wrong_state(sm_conn);
            break;
    }

    // try to send next pdu
    sm_trigger_run();
}
```
(in `sm_pdu_handler()`, around line 4788)
