echo 1
insmod /system/lib/cp210x.ko
echo 2
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq
cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq
cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq

echo 3
chmod 666 /sys/devices/system/cpu/cpu0/online
chmod 666 /sys/devices/system/cpu/cpu1/online
chmod 666 /sys/devices/system/cpu/cpu2/online
chmod 666 /sys/devices/system/cpu/cpu3/online
chmod 666 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 4
echo 1 > /sys/devices/system/cpu/cpu0/online
chmod 444 /sys/devices/system/cpu/cpu0/online
echo 5
echo 1 > /sys/devices/system/cpu/cpu1/online
chmod 444 /sys/devices/system/cpu/cpu1/online
echo 6
echo 1 > /sys/devices/system/cpu/cpu2/online
chmod 444 /sys/devices/system/cpu/cpu2/online
echo 7
echo 1 > /sys/devices/system/cpu/cpu3/online
chmod 444 /sys/devices/system/cpu/cpu3/online
echo 8
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
chmod 444 /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 9
sleep 1
echo 10
/system/bin/qualcomm_online_demo /sdcard/
