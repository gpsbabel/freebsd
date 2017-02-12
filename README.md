## Prepare your environment.
On FreeBSD/amd64 machine install the following packages:
```
sudo pkg install riscv64-xtoolchain-gcc qemu-riscv riscv-isa-sim
```

## Build FreeBSD world
```
svnlite co http://svn.freebsd.org/base/head freebsd-riscv
cd freebsd-riscv
setenv DESTDIR /home/${USER}/riscv-world
make TARGET_ARCH=riscv64 -DNO_ROOT -DWITHOUT_TESTS DESTDIR=$DESTDIR installworld
make TARGET_ARCH=riscv64 -DNO_ROOT -DWITHOUT_TESTS DESTDIR=$DESTDIR distribution
```

## Build 32mb rootfs image
```
cd freebsd-riscv
fetch https://raw.githubusercontent.com/bukinr/riscv-tools/master/image/basic.files
tools/tools/makeroot/makeroot.sh -s 32m -f basic.files riscv.img $DESTDIR
```

## Prepare kernel config. Modify sys/riscv/conf/GENERIC, uncomment following lines and specify the path to your riscv.img:
```
options 	MD_ROOT
options 	MD_ROOT_SIZE=32768	# 32MB ram disk
makeoptions	MFS_IMAGE=/path/to/riscv.img
options 	ROOTDEVNAME=\"ufs:/dev/md0\"
```

## Build FreeBSD kernel
```
cd freebsd-riscv
for Spike:
make TARGET_ARCH=riscv64 KERNCONF=SPIKE buildkernel
for QEMU:
make TARGET_ARCH=riscv64 KERNCONF=QEMU buildkernel
```

## Build BBL
```
git clone https://github.com/freebsd-riscv/riscv-pk
cd riscv-pk
mkdir build && cd build
setenv CFLAGS "-nostdlib"
../configure --host=riscv64-unknown-freebsd11.0 --with-payload=path_to_freebsd_kernel
gmake LIBS=''
unsetenv CFLAGS
```

## Run Spike simulator
```
spike -m1024 -p2 /path/to/bbl
```

## Run QEMU emulator
```
qemu-system-riscv64 -m 2048M -kernel /path/to/bbl -nographic
```

Additional information available on [FreeBSD Wiki](http://wiki.freebsd.org/riscv)
