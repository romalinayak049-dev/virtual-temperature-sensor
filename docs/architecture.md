# Architecture

```text
User-Space Application   (app/monitor.c)
        |
        | open / read / write / close
        v
/dev/vtempsensor
        |
        v
Linux Character Device Driver   (driver/vtempsensor.c)
        |
        v
Virtual Temperature Sensor   (integer state in kernel memory)
```

## User space
Where normal programs run with restricted privileges. `monitor` runs here. It
cannot touch kernel memory; it only uses system calls on a device file.

## Kernel space
Privileged environment where the kernel and its modules run. `vtempsensor.ko`
runs here and owns the sensor state (temperature, threshold).

## Character device
A device accessed as a stream of bytes through file operations. Here the driver
implements `open`, `read`, `write` and `release`.

## /dev/vtempsensor
The device file the driver creates (via `miscdevice`, dynamic minor number).
User programs open it like a normal file; the kernel routes each call to the
driver. Data crosses the boundary with `copy_from_user()` (write) and
`simple_read_from_buffer()` (read, which uses `copy_to_user()` internally).

## Driver lifecycle
```text
insmod -> vts_init() -> misc_register() -> /dev/vtempsensor appears -> ACTIVE
rmmod  -> vts_exit() -> misc_deregister() -> /dev/vtempsensor removed
```

## Data format
- Read returns: `temperature=25 C`, `threshold=40 C`, `state=NORMAL` (3 lines).
- Write `35` sets the temperature; write `threshold=45` sets the threshold.
- Values are integers in degrees C (range -50..150); no floating point in the kernel.
- State: `NORMAL` below threshold, `WARNING` at or above threshold,
  `CRITICAL` at or above the fixed 60 C critical limit.
