#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

#Compile new writer application
make CROSS_COMPILE=${CROSS_COMPILE}
FSDIR=${OUTDIR}/rootfs

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Build Kernel
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${FSDIR}" ]
then
	echo "Deleting rootfs directory at ${FSDIR} and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

#Create Filesystem Skeleton
mkdir ${FSDIR}
mkdir -p ${FSDIR}/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var}
mkdir -p ${FSDIR}/usr/{bin,lib,sbin}
mkdir -p ${FSDIR}/var/log
mkdir -p ${FSDIR}/home/conf

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
else
    cd busybox
fi

# Make and install busybox
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd ${FSDIR}

# Add library dependencies to rootfs
echo "Library dependencies"
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)

SHARED_LIBS=$(${CROSS_COMPILE}readelf -a bin/busybox | grep -oE "Shared library: .*" | grep -oP "[a-z\-0-9]+\.so\.[0-9]+")
INTERPRETER=$(${CROSS_COMPILE}readelf -a bin/busybox | grep -oE "program interpreter: .*" | grep -oP "[a-z\-0-9]+\.so\.[0-9]+")

echo "${SHARED_LIBS}" | while read lib; do
    find ${SYSROOT} -name "$lib" -exec cp {} ${FSDIR}/lib64 \;
done

echo "${INTERPRETER}" | while read lib; do
    find ${SYSROOT} -name "$lib" -exec cp {} ${FSDIR}/lib \;
done

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/{autorun-qemu.sh,finder.sh,finder-test.sh,writer} ${FSDIR}/home/
cp ${FINDER_APP_DIR}/conf/{username.txt,assignment.txt} ${FSDIR}/home/conf
cd 

# Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# Chown the root directory
sudo chown -R root:root ${FSDIR}

# Clean and build the writer utility
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# Create initramfs.cpio.gz
cd ${OUTDIR}
gzip -f initramfs.cpio
