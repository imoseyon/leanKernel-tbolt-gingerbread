#!/system/bin/sh
busybox mount -o remount,rw /system
ifile="/system/etc/init.d/01vdd_levels"
minfreq=`cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq`
maxfreq=`cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq`

while [ 1 ]; do
echo
echo "leanKernel speed tweaker"
echo "------------------------"
echo "1) normal: undervolted, overclocked to 1.4Ghz"
echo "2) extreme: extremely undervolted, overclocked to 1.4Ghz"
if [ $maxfreq -gt 1500000 ]; then
	echo "3) 1.92 undervolted, overclocked to 1.9Ghz"
	echo "4) 1.92X: extremely undervolted, overclocked to 1.9Ghz"
fi
echo "5) battsaver: extremely undervolted, not overclocked"
echo
echo "WARNING: Anything other than *normal* could make your phone unstable."
echo
if [ $minfreq -lt 200000 ]; then
	echo "6) Unlock 184Mhz"
	echo "7) Lock 184Mhz"
	echo
fi
echo "8) Choose InteractiveX governor"
echo "9) Choose SmartassV2 governor"
echo "10) Choose OndemandX governor"
echo
echo "11) View current settings"
echo "12) View time-in-state (shows how much time is spent at each slot)"
echo "13) Remove min_freq/max_freq/governor settings from init.d (if you want to use SetCPU)."
echo "14) Exit script"
echo
echo "You appear to be running "`grep Mode $ifile | awk '{ print $3 }'` "mode."
echo "---"
echo -n "Please enter a number: "
read option
case $option in
1)
# normal
	echo "#!/system/bin/sh" > $ifile
	echo "# Mode: normal" >> $ifile
	echo 'sysfile="/sys/devices/system/cpu/cpu0/cpufreq/vdd_levels"' >> $ifile
	chmod 555 $ifile
	[ $minfreq -lt 200000 ] && echo 'echo "184320 900" > $sysfile' >> $ifile
	echo 'echo "245760 950" > $sysfile' >> $ifile
	echo 'echo "368640 1000" > $sysfile' >> $ifile
	echo 'echo "768000 1050" > $sysfile' >> $ifile
	echo 'echo "1024000 1100" > $sysfile' >> $ifile
	echo 'echo "1222400 1150" > $sysfile' >> $ifile
	echo 'echo "1408000 1250" > $sysfile' >> $ifile
	echo 'echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	echo 'echo 1408000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' >> $ifile
	sh $ifile
;;
2)
# extreme
	echo "#!/system/bin/sh" > $ifile
	echo "# Mode: extreme" >> $ifile
	echo 'sysfile="/sys/devices/system/cpu/cpu0/cpufreq/vdd_levels"' >> $ifile
	chmod 555 $ifile
	[ $minfreq -lt 200000 ] && echo 'echo "184320 750" > $sysfile' >> $ifile
	echo 'echo "245760 825" > $sysfile' >> $ifile
	echo 'echo "368640 900" > $sysfile' >> $ifile
	echo 'echo "768000 1000" > $sysfile' >> $ifile
	echo 'echo "1024000 1100" > $sysfile' >> $ifile
	echo 'echo "1222400 1125" > $sysfile' >> $ifile
	echo 'echo "1408000 1175" > $sysfile' >> $ifile
	echo 'echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	echo 'echo 1408000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' >> $ifile
	sh $ifile
;;
3)
# 1.92
	echo "#!/system/bin/sh" > $ifile
	echo "# Mode: 1.92" >> $ifile
	echo 'sysfile="/sys/devices/system/cpu/cpu0/cpufreq/vdd_levels"' >> $ifile
	chmod 555 $ifile
	[ $minfreq -lt 200000 ] && echo 'echo "184320 900" > $sysfile' >> $ifile
	echo 'echo "245760 950" > $sysfile' >> $ifile
	echo 'echo "368640 1000" > $sysfile' >> $ifile
	echo 'echo "768000 1050" > $sysfile' >> $ifile
	echo 'echo "1024000 1100" > $sysfile' >> $ifile
	echo 'echo "1222400 1150" > $sysfile' >> $ifile
	echo 'echo "1408000 1250" > $sysfile' >> $ifile
	echo 'echo "1593600 1375" > $sysfile' >> $ifile
	echo 'echo "1766400 1400" > $sysfile' >> $ifile
	echo 'echo "1920000 1450" > $sysfile' >> $ifile
	echo 'echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	echo 'echo 1920000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' >> $ifile
	sh $ifile
;;
4)
# 1.92X
	echo "#!/system/bin/sh" > $ifile
	echo "# Mode: 1.92X" >> $ifile
	echo 'sysfile="/sys/devices/system/cpu/cpu0/cpufreq/vdd_levels"' >> $ifile
	chmod 555 $ifile
	[ $minfreq -lt 200000 ] && echo 'echo "184320 750" > $sysfile' >> $ifile
	echo 'echo "245760 825" > $sysfile' >> $ifile
	echo 'echo "368640 900" > $sysfile' >> $ifile
	echo 'echo "768000 1000" > $sysfile' >> $ifile
	echo 'echo "1024000 1100" > $sysfile' >> $ifile
	echo 'echo "1222400 1125" > $sysfile' >> $ifile
	echo 'echo "1408000 1175" > $sysfile' >> $ifile
	echo 'echo "1593600 1250" > $sysfile' >> $ifile
	echo 'echo "1766400 1300" > $sysfile' >> $ifile
	echo 'echo "1920000 1400" > $sysfile' >> $ifile
	echo 'echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	echo 'echo 1920000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' >> $ifile
	sh $ifile
;;
5)
# battsaver
	echo "#!/system/bin/sh" > $ifile
	echo "# Mode: battsaver" >> $ifile
	echo 'sysfile="/sys/devices/system/cpu/cpu0/cpufreq/vdd_levels"' >> $ifile
	chmod 555 $ifile
	[ $minfreq -lt 200000 ] && echo 'echo "184320 750" > $sysfile' >> $ifile
	echo 'echo "245760 825" > $sysfile' >> $ifile
	echo 'echo "368640 900" > $sysfile' >> $ifile
	echo 'echo "768000 1000" > $sysfile' >> $ifile
	echo 'echo "1024000 1100" > $sysfile' >> $ifile
	echo 'echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	echo 'echo 1024000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' >> $ifile
	sh $ifile
;;
6)
# unlock 184Mhz
	echo 'echo 184320 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	sh $ifile
;;
7)
# lock 184Mhz
	echo 'echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq' >> $ifile
	sh $ifile
;;
8)
	echo interactiveX > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
	echo 'echo interactiveX > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor' >> $ifile
;;
9)
	echo smartassV2 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
	echo 'echo smartassV2 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor' >> $ifile
;;
10)
	echo ondemandX > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
	echo 'echo ondemandX > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor' >> $ifile
;;
11)
  echo
  cat /sys/devices/system/cpu/cpu0/cpufreq/vdd_levels
  echo
  echo -n "min: "
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
  echo -n "max: "
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
  echo -n "governor: "
  cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
  echo "You appear to be running "`grep Mode $ifile | awk '{ print $3 }'` "mode, am i right?"
;;
12)
	echo
	cat /sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state
;;
13)
	sed -i /scaling_/d $ifile	
	echo "Governor and min/max freqs removed from $ifile - you can now use SetCPU to set min/max freqs and governor."
;;
*)
busybox mount -o remount,ro /system
echo "---"
echo "All set! Be careful about using setcpu - you can easily override the min/max settings."
echo
exit
;;
esac

echo
echo -n "Please hit Enter to continue:"
read key
done
