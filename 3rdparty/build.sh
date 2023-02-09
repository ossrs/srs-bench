#!/bin/bash
WORK_DIR=`pwd`
OBJ_DIR=${WORK_DIR}/objs
TRD_DIR=${WORK_DIR}/3rdparty

OS_IS_OSX=$(uname -s |grep -q Darwin && echo YES)
OS_IS_LINUX=$(uname -s |grep -q Linux && echo YES)
if [[ $OS_IS_OSX == YES ]];  then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 1);
else
    JOBS=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1)
fi

gcc --version >/dev/null 2>/dev/null; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install gcc"; exit $ret;
fi
g++ --version >/dev/null 2>/dev/null; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install g++"; exit $ret;
fi
make --version >/dev/null 2>/dev/null; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install make"; exit $ret;
fi
unzip -v >/dev/null 2>/dev/null; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install unzip"; exit $ret;
fi
tclsh <<< "exit" >/dev/null 2>&1; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install tclsh"; exit $ret;
fi
cmake --version >/dev/null 2>/dev/null; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install cmake"; exit $ret;
fi
pkg-config --version >/dev/null 2>/dev/null; ret=$?; if [[ 0 -ne $ret ]]; then
    echo "Please install pkg-config"; exit $ret;
fi

mkdir -p ${OBJ_DIR}/build/out

#####################################################################################
# Install openssl
#####################################################################################

if [[ -f ${OBJ_DIR}/build/out/openssl/lib/libssl.a ]]; then
    rm -rf ${OBJ_DIR}/openssl && cp -rf ${OBJ_DIR}/build/out/openssl ${OBJ_DIR}/ &&
    echo "The openssl-1.1.1t is ok."
else
    echo "Building openssl-1.1.1t."
    rm -rf ${OBJ_DIR}/build/openssl-1.1.1t ${OBJ_DIR}/build/out/openssl ${OBJ_DIR}/openssl &&
    pwd && 
    tar xf ${TRD_DIR}/openssl-1.1.1t.tar.gz -C ${OBJ_DIR}/build/ &&
    (
        cd ${OBJ_DIR}/build/openssl-1.1.1t &&
        ./config --prefix=${OBJ_DIR}/build/out/openssl -no-shared -no-threads -DOPENSSL_NO_HEARTBEATS
    ) &&
    make -C ${OBJ_DIR}/build/openssl-1.1.1t --jobs ${JOBS} &&
    make -C ${OBJ_DIR}/build/openssl-1.1.1t install_sw &&
    cp -rf ${OBJ_DIR}/build/out/openssl ${OBJ_DIR}/openssl &&
    echo "The openssl-1.1.1t is ok."
fi

# check status
ret=$?; if [[ $ret -ne 0 ]]; then echo "Build openssl-1.1.1t failed, ret=$ret"; exit $ret; fi

#####################################################################################
# Install SRT
#####################################################################################

# Always disable c++11 for libsrt, because only the srt-app requres it.
LIBSRT_OPTIONS="--enable-apps=0  --enable-static=1 --enable-c++11=0 --enable-shared=0"
if [[ -f ${OBJ_DIR}/build/out/srt/lib/libsrt.a ]]; then
    rm -rf ${OBJ_DIR}/srt && cp -rf ${OBJ_DIR}/build/out/srt ${OBJ_DIR}/ &&
    echo "The srt-1.5.1 is ok."
else
    if [[ ! -d ${OBJ_DIR}/openssl/lib/pkgconfig ]]; then
        echo "Build srt-1.5.1 failed, openssl pkgconfig no found."
        exit -1
    fi
    echo "Building srt-1.5.1" &&
    rm -rf ${OBJ_DIR}/build/srt-1.5.1 ${OBJ_DIR}/build/out/srt ${OBJ_DIR}/srt &&
    tar xf ${TRD_DIR}/srt-1.5.1.tar.gz -C ${OBJ_DIR}/build/ &&
    (
        cd ${OBJ_DIR}/build/srt-1.5.1 &&
        env PKG_CONFIG_PATH=${OBJ_DIR}/openssl/lib/pkgconfig \
            ./configure --prefix=${OBJ_DIR}/build/out/srt --enable-apps=0 --enable-static=1 --enable-c++11=0 --enable-shared=0
    ) &&
    make -C ${OBJ_DIR}/build/srt-1.5.1 --jobs=${JOBS} &&
    make -C ${OBJ_DIR}/build/srt-1.5.1 install &&
    if [[ -d ${OBJ_DIR}/build/out/srt/lib64 ]]; then
        cp -rf ${OBJ_DIR}/build/out/srt/lib64 ${OBJ_DIR}/build/out/srt/lib
    fi &&
    cp -rf ${OBJ_DIR}/build/out/srt ${OBJ_DIR}/ &&
    echo "The srt-1.5.1 is ok."
fi

# check status
ret=$?; if [[ $ret -ne 0 ]]; then echo "Build srt-1.5.1 failed, ret=$ret"; exit $ret; fi

