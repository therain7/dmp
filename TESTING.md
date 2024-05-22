The module was thoroughly tested manually. [fio](https://github.com/axboe/fio) was used to produce read & write workloads.

Sample test workflow included the following:
- Creating multiple virtual devices with `dmsetup create` using `dmp` target
- Performing read & write workloads on them using `fio` (sample workflow configurations presented below)
- Getting stats from the module through `sysfs` and comparing them to workload created

All tests indicated that stats were gathered and presented correctly.
Also simple testing was performed to verify that I/O operations on virtual devices reach their underlying targets.
Testing was performed on Linux machine running `Fedora 40. Linux 6.8.9`.


## fio sample configurations

Random reads `32k block size` to `/dev/mapper/dmp1` for 30 seconds
```bash
sudo fio --filename=/dev/mapper/dmp1 --direct=1 \
     --rw=randread --bs=32k --ioengine=libaio --iodepth=256 \
     --runtime=30 --numjobs=4 --time_based --group_reporting \
     --name=iops-test-job --eta-newline=1 --readonly
```

Random reads and writes `16k block size` to `/dev/mapper/dmp1` for 30 seconds
```bash
sudo fio --filename=/dev/mapper/dmp1 --direct=1 \
     --rw=randrw --bs=16k --ioengine=libaio --iodepth=256 \
     --runtime=30 --numjobs=4 --time_based --group_reporting \
     --name=iops-test-job --eta-newline=1
```
