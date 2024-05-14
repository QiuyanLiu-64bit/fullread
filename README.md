# FullRead

This is a prototype of FullRead.

## 1 Preparation

Ubuntu 20.04 LTS

```bash
mkdir /home/ych
cd /home/ych
```

### 1.1 Common

```bash
sudo apt update && sudo apt -y install gcc g++ cmake make net-tools gdb git iperf openssh-client openssh-server unzip autoconf nasm libtool python3-pip openssl libssl-dev cgroup-tools hdparm expect
```

```bash
pip install numpy scipy
```

### 1.2 ISA-L

```bash
# /home/ych/
git clone https://github.com/intel/isa-l.git
cd isa-l
./autogen.sh
./configure && make && sudo make install
```

Easily clone `5` identical machines

### 1.3 IP Configuration

**Client:** `192.168.7.100`

**Servers:** `192.168.7.101` ~ ``192.168.7.106`

### 1.4 Clone This Repository

```bash
# /home/ych
git clone https://github.com/QiuyanLiu-64bit/fullread.git
```

## 2 Run

### 2.1 Configure Cluster

```bash
cd /home/ych/ec_test && cd script && chmod a+x * 
ssh-keygen # Just use default configuration
./ssh-auto-send.exp 101 106

# mount another disk if available
# ./mount_nvme.sh 101 106

./create_env_storagenodes.sh 101 106

cd /home/ych/ec_test/script && ./make.sh 101 106
```

### 2.2Test

```bash
dd if=/dev/urandom of=test_file/write/64MB_src bs=16M count=4

# Write
./start_all_storagenodes.sh 101 106
./start_client.sh -w 64MB_src 64MB_dst

# Read
./start_all_storagenodes.sh 101 106
./start_client.sh -r 64MB_src 64MB_dst

# FullRead
./start_all_storagenodes.sh 101 106
./start_client.sh -fr 64MB_src 64MB_dst
```

## 3 Environmental configuration

### 3.1 Macro definition

`include/ec_config.h`

```c
#define EC_K 4                   // k of k+m EC
#define EC_M 2                   // m of k+m EC, larger than 1
#define EC_N 6                   // n of k+m EC

#define CHUNK_SIZE 16777216     // unit Byte

#define HETEROGENEOUS 0 // 1, heterogeneous; 0, homogeneous. Belongs to read test.

#define DEBUGGING_COMMENTS 0 // 1, print debugging comments; 0, do not print debugging comments

```

`test_file/read/read_opt.py`

For `heterogeneous` environment.

```bash
python3 ./read_opt.py /home/ych/ec_test/test_file/read/ec_heterogeneous_read_dst.txt <data_size> <n> <k> <io_list>
# python3 ./read_opt.py /home/ych/ec_test/test_file/read/ec_heterogeneous_read_dst.txt 67108864 6 4 1 1 1 1 1 1 
```

