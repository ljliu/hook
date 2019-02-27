#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "$0 version"
    exit 1
fi

ver=$1

if [[ ! -d "${ver}" ]]; then
    mkdir -p "${ver}"
fi

rm -rf ${ver}/*

mkdir -p "${ver}/sdk_config"
cp third_party/mobvoisds/libs/armeabi-v7a                 ${ver}/ -r
cp third_party/mobvoisds/mobvoi.tar                       ${ver}/sdk_config/
cp third_party/mobvoidsp/libs/armeabi-v7a/*               ${ver}/armeabi-v7a/
cp third_party/mobvoidsp/cp210x.ko                        ${ver}/
cp qualcomm_demo/dsp_config                               ${ver}/ -r
cp qualcomm_demo/dsp_post_config                          ${ver}/ -r
cp qualcomm_demo/demo.sh                                  ${ver}/
cp qualcomm_demo/cpuinfo.sh                               ${ver}/
cp build.android-armeabi-v7a.Release/qualcomm_online_demo ${ver}/

echo "adb remount

adb push qualcomm_online_demo /system/bin/
adb push armeabi-v7a/ /system/lib/

adb shell rmmod cp210x
adb push cp210x.ko /system/lib/

adb shell mkdir -p /sdcard/mobvoi/dsp/
adb push dsp_config/ /sdcard/mobvoi/dsp/

adb shell mkdir -p /sdcard/mobvoi/dsp_post/
adb push dsp_post_config/ /sdcard/mobvoi/dsp_post/

adb push sdk_config/mobvoi.tar /sdcard/
adb shell tar -xvf /sdcard/mobvoi.tar  -C /sdcard/

adb push demo.sh /system/bin/
adb push cpuinfo.sh /system/bin/
adb shell sync
" > ${ver}/push.sh

chmod +x ${ver}/push.sh

tar zcf ${ver}.tgz ${ver}

ls -l ${ver}
