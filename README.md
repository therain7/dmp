# dmp - device mapper proxy

Linux [device mapper](https://en.wikipedia.org/wiki/Device_mapper) target.
It proxies all I/O requests to underlying device, gathers statistics and provides them through `sysfs`.

## Installation

To install `dmp` device mapper target to your system build the kernel module with `make` and then insert it using `insmod`.

```bash
  $ make
  $ sudo insmod dmp.ko
```

Now `dmp` target should be visible in the output of `dmsetup targets` command.

## Usage / Examples

### Create virtual device

To create new virtual device using `dmp` target run `dmsetup create` command as follows:
```bash
  $ sudo dmsetup create dmp1 --table "0 $num_sectors dmp /dev/some_device"
```
where `$num_sectors` is the size of a new device in sectors,
<br> and `/dev/some_device` is the underlying device to which requests would be proxied.

Created virtual device would be available at `/dev/mapper/dmp1`.

For more information please refer to `dmsetup` manual.

### Access provided stats

`dmp` provides following statistics for proxied I/O requests:
- Amount of read requests
- Amount of write requests
- Total amount of read & write requests
- Average size of read request
- Average size of write request
- Average size of all read & write requests.

This data is provided for each virtual device seperately as well as for all devices combined.

To access the stats provided please use `sysfs` interface as follows:
```bash
  $ cat /sys/module/dmp/stat/$device_name/read_reqs
```
where `$device_name` is the name of virtual device,
<br> and `read_reqs` is the specific attribute to get data from.

The following attributes are available: `read_reqs`, `write_reqs`, `total_reqs`, `read_avg_size`, `write_avg_size`, `total_avg_size`.

To access aggregate statistics for all devices use `all` as `$device_name`.
<br>

```bash
  $ cat /sys/module/dmp/stat/all/write_reqs
  400

  $ cat /sys/module/dmp/stat/all/write_avg_size
  4096

  $ cat /sys/module/dmp/stat/253:0/total_avg_size
  4096
```

## Testing

The module was thoroughly tested manually. See [TESTING](TESTING.md) for more information. 

## License

Distributed under the GPLv2 License. See [LICENSE](LICENSE) for more information.
