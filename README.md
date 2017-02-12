# freebsd-wiki
wiki pages

0. Prepare your environment.
1. Install devel/riscv64-xtoolchain-gcc, emulators/qemu-riscv, emulators/riscv-isa-sim packages

2. Build FreeBSD world
```
svnlite co http://svn.freebsd.org/base/head freebsd-riscv
```

3. Build 32mb rootfs image
```
cd freebsd-riscv
setenv DESTDIR /home/${USER}/riscv-world
make TARGET_ARCH=riscv64 -DNO_ROOT -DWITHOUT_TESTS DESTDIR=$DESTDIR installworld
make TARGET_ARCH=riscv64 -DNO_ROOT -DWITHOUT_TESTS DESTDIR=$DESTDIR distribution
fetch https://raw.githubusercontent.com/bukinr/riscv-tools/master/image/basic.files
tools/tools/makeroot/makeroot.sh -s 32m -f basic.files riscv.img $DESTDIR
```

4. Prepare kernel config. Modify sys/riscv/conf/GENERIC, uncomment following lines:
```
options 	MD_ROOT
options 	MD_ROOT_SIZE=32768	# 32MB ram disk
makeoptions	MFS_IMAGE=/path/to/img
options 	ROOTDEVNAME=\"ufs:/dev/md0\"
```

5. Build FreeBSD kernel
```
for Spike:
make TARGET_ARCH=riscv64 KERNCONF=SPIKE buildkernel
for QEMU:
make TARGET_ARCH=riscv64 KERNCONF=QEMU buildkernel
```

6. Build BBL
```
git clone https://github.com/freebsd-riscv/riscv-pk
cd riscv-pk
mkdir build && cd build
setenv CFLAGS "-nostdlib"
../configure --host=riscv64-unknown-freebsd11.0 --with-payload=path_to_freebsd_kernel
gmake LIBS=''
unsetenv CFLAGS
```

7. Run Spike simulator
```
spike -m1024 -p2 /path/to/bbl
```

8. Run QEMU emulator
```
qemu-system-riscv64 -m 2048M -kernel /path/to/bbl -nographic
```
