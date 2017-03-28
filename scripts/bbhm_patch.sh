#!/bin/sh
#zhicheng_qiu@cable.comcast.com
#bbhm patch for 2.1s11

usage() 
{
	echo "Usage: $0 -f <bbhm file path> "
	echo "Example: $0 -f /nvram/bbhm_cur_cfg.xml"
}

if [ \"$2\" == \"\" ] ; then
	usage;
	exit 0;
fi

if [ -f $2 ] ; then

	grep "<Record name=\"dmsb.l2net.2.Members.WiFi\" type=\"astr\">ath2 ath3<\/Record>" $2
	if [  "$?" == "1" ] ; then
	#bbhm=$2;
	cp $2 /tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.2.Members.WiFi\" type=\"astr\">ath2<\/Record>"/"<Record name=\"dmsb.l2net.2.Members.WiFi\" type=\"astr\">ath2 ath3<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Name\" type=\"astr\">brlan4<\/Record>"/"<Record name=\"dmsb.l2net.5.Name\" type=\"astr\">brlan7<\/Record>"/ >/tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Vid\" type=\"astr\">104<\/Record>"/"<Record name=\"dmsb.l2net.5.Vid\" type=\"astr\">107<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Members.WiFi\" type=\"astr\">ath8 ath9<\/Record>"/"<Record name=\"dmsb.l2net.5.Members.WiFi\" type=\"astr\">ath14 ath15<\/Record>"/ >/tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Port.1.Pvid\" type=\"astr\">104<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.1.Pvid\" type=\"astr\">107<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Port.1.Name\" type=\"astr\">brlan4<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.1.Name\" type=\"astr\">brlan7<\/Record>"/ >/tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Port.2.Pvid\" type=\"astr\">104<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.2.Pvid\" type=\"astr\">107<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Port.2.Name\" type=\"astr\">ath8<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.2.Name\" type=\"astr\">ath14<\/Record>"/ >/tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Port.2.LinkName\" type=\"astr\">ath8<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.2.LinkName\" type=\"astr\">ath14<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Port.3.Pvid\" type=\"astr\">104<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.3.Pvid\" type=\"astr\">107<\/Record>"/ >/tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Port.3.Name\" type=\"astr\">ath9<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.3.Name\" type=\"astr\">ath15<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Port.3.LinkName\" type=\"astr\">ath9<\/Record>"/"<Record name=\"dmsb.l2net.5.Port.3.LinkName\" type=\"astr\">ath15<\/Record>"/ >/tmp/b1
	cp /tmp/b1 $2
	rm /tmp/b1
	rm /tmp/b2
	fi

	grep "<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.ApIsolationEnable\" type=\"astr\">1<\/Record>" $2
	if [  "$?" == "1" ] ; then
	cp $2 /tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.ApIsolationEnable\" type=\"astr\">0<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.ApIsolationEnable\" type=\"astr\">1<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.10.ApIsolationEnable\" type=\"astr\">0<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.10.ApIsolationEnable\" type=\"astr\">1<\/Record>"/ >/tmp/b1
	cp /tmp/b1 $2
        rm /tmp/b1
        rm /tmp/b2
	fi

	# Check if bbhm has Notify flag present
	NOTIFYPRESENT=`cat $2 | grep NotifyWiFiChanges`
	REDIRCTEXISTS=""

	# If Notify flag is not present then we will add it as per the syscfg DB value
	if [ "$NOTIFYPRESENT" = "" ]
	then
		REDIRECT_VALUE=`syscfg get redirection_flag`
		if [ "$REDIRECT_VALUE" = "" ]
		then
			#Just making sure if syscfg command didn't fail
			REDIRCTEXISTS=`cat /nvram/syscfg.db | grep redirection_flag | cut -f2 -d=`
		fi

		if [ "$REDIRECT_VALUE" = "false" ] || [ "$REDIRCTEXISTS" = "false" ];
		then
		
			echo " Apply Notifywifichanges false"
			cat $2 | sed '10 a \ \ \ <Record name=\"eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges\" type=\"astr\">false</Record>' > /tmp/b2
			cp /tmp/b2 $2
			rm /tmp/b2
			exit 0;
		
		elif [ "$REDIRECT_VALUE" = "true" ] || [ "$REDIRCTEXISTS" = "true" ];
		then
			echo " Apply Notifywifichanges true"
			cat $2 |sed '10 a \ \ \ <Record name=\"eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges\" type=\"astr\">true</Record>' > /tmp/b2
			cp /tmp/b2 $2
			rm /tmp/b2
			exit 0;
		fi
	fi


fi



