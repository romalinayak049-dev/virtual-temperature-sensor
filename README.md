# Virtual Temperature Sensor Using a Linux Character Device Driver

A virtual temperature sensor implemented as a Linux kernel module (character
device `/dev/vtempsensor`) plus a small user-space C monitoring application.
No physical hardware is needed. Language: C. Platform: Linux.

## Overview
- `driver/vtempsensor.c` - kernel module; registers `/dev/vtempsensor` (miscdevice)
  and keeps the sensor state (temperature, threshold) as integers in degrees C.
- `app/monitor.c` - interactive terminal program using `open()`, `read()`,
  `write()`, `close()` on the device.
- `docs/architecture.md` - architecture, kernel/user space, driver lifecycle.

```text
User-Space Application (monitor)
        | open/read/write
        v
/dev/vtempsensor
        |
        v
Linux Character Device Driver (vtempsensor.ko)
        |
        v
Virtual Temperature Sensor
```

## Project Structure
```text
virtual-temperature-driver/
├── driver/
│   ├── vtempsensor.c
│   └── Makefile
├── app/
│   ├── monitor.c
│   └── Makefile
├── docs/
│   └── architecture.md
├── README.md
├── LICENSE
└── .gitignore
```

## Device Interface
| Operation | Behavior |
|---|---|
| `read` | `temperature=25 C` / `threshold=40 C` / `state=NORMAL` (one per line) |
| `write "35"` | sets temperature to 35 C |
| `write "threshold=45"` | sets warning threshold to 45 C |

Values are integers from -50 to 150 C; anything else is rejected with an error.
State is `NORMAL` below the threshold, `WARNING` at or above it, and `CRITICAL`
at or above the fixed 60 C critical limit.

## Prerequisites (Ubuntu 22.04 or later)
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

## Build
```bash
cd driver && make        # produces vtempsensor.ko
cd ../app && make        # produces monitor
cd ..
```

## Load the Driver
```bash
sudo insmod driver/vtempsensor.ko
lsmod | grep vtempsensor
ls -l /dev/vtempsensor
dmesg | tail
```
Expected kernel messages: `vtempsensor: module loaded` and
`vtempsensor: character device registered: /dev/vtempsensor`.

## Try It
```bash
cat /dev/vtempsensor
echo 35 | sudo tee /dev/vtempsensor
cat /dev/vtempsensor
./app/monitor
```
The device is created with mode 0666, so `sudo` is not required for `monitor`.
If the driver is not loaded, `monitor` prints:
`Error: Unable to read /dev/vtempsensor ... Make sure the kernel module is loaded.`

## Monitor Menu
1. Read Temperature  2. Increase (+5 C)  3. Decrease (-5 C)
4. Set Threshold  5. Show Status  6. Exit

## Unload the Driver
```bash
sudo rmmod vtempsensor
dmesg | tail      # shows "vtempsensor: module unloaded"
```

## Clean
```bash
cd driver && make clean
cd ../app && make clean
```

## Status and Limitations
- The sensor is a software simulation; values exist only while the module is loaded.The
  user-space app compiles cleanly with `gcc -Wall -Wextra`.

## Name- Romali Nayak


