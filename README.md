# samplemem

`samplemem` allows checking whether a large range of memory contains the same byte over and over by sampling a given number of blocks.

The samples are spaced equidistantly over the whole file.

## Usage

```
samplemem <block_device> <block_size> <value_to_check> <n_samples>
```

Where:
 - `block_device`: file to check by sampling
 - `block_size`: size of a block
 - `value_to_check`: byte containing the value that has to be the same for the whole block
 - `n_samples`: number of samples to check

Output:
 - `block_number` of each checked block that failed the check

### Example
E.g.: To check if the drive at `/dev/sdd` only contains the value `0x55` by checking the memory using `1024` samples and a block size of `4096`: 

Example:
```
sudo ./samplemem /dev/sdd 4096 0x55 1024
```
