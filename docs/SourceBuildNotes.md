# Building source

Quick start building notes for Linux, tested on a fresh install of Debian Bookworm, but should be very similar for any Debian derived distro.
For other environments, see the [pico getting started guide][gs].

This is the bare minimum to get to the point where you can build a binary to copy to the Pico.

## Install build tools
```
apt-get install git cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib
```

## Setup pico-sdk
```
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=`pwd`
cd ..
```

## Get zc95 source & build
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

# Debugging
For small fixes/changes/tweaks the above is fine, but requires manually copying to Pico via USB, and doesn't allow for debugging, so very quickly gets tedious for larger changes.

The pico-sdk integrates really well with Visual Studio Code & an SWD debugger, allowing single keypress build & upload, along with setting breakpoints, stepping through code, etc., so that would be my recommendation for a development environment. 
For the SWD debugger, I'm using a J-Link as that's what I had, but a [Picoprobe][pp] seems to be the Raspberry Pi recommend option, and well documented elsewhere.

Describing that setup is documented in the [pico getting started guide][gs], see Chapter 7 & Appendix A.

It's probably worth noting that the code is using both cores, so bare that in mind if unexpected things happen when debugging, setting break points, etc.

[gs]: https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf
[pp]: https://www.raspberrypi.com/products/debug-probe/

# BTStack patch
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
