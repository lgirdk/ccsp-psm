#!/bin/sh
##################################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
#  Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#####################################################################################

#zhicheng_qiu@cable.comcast.com
#bbhm patch for 2.1s11

. /etc/device.properties
if [ "$BOX_TYPE" = "XB3" ]; then
SYSCFG_DB_FILE="/nvram/syscfg.db"
else
SYSCFG_DB_FILE="/opt/secure/data/syscfg.db"
fi

usage() 
{
	echo "Usage: $0 -f <bbhm file path> "
	echo "Example: $0 -f /tmp/bbhm_cur_cfg.xml"
}

if [ \"$2\" == \"\" ] ; then
	usage;
	exit 0;
fi

if [ -f $2 ] ; then

	grep "<Record name=\"dmsb.l2net.5.Members.WiFi\" type=\"astr\">ath9</Record>" $2
	if [  "$?" == "0" ] ; then
	cp $2 /tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Members.WiFi\" type=\"astr\">ath9<\/Record>"/"<Record name=\"dmsb.l2net.5.Members.WiFi\" type=\"astr\">ath14 ath15<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Port.2.LinkName\" type=\"astr\" \/>"/"<Record name=\"dmsb.l2net.5.Port.2.LinkName\" type=\"astr\">ath14<\/Record>"/ >/tmp/b1
	cat /tmp/b1 | sed s/"<Record name=\"dmsb.l2net.5.Port.2.LinkType\" type=\"astr\" \/>"/"<Record name=\"dmsb.l2net.5.Port.2.LinkType\" type=\"astr\">WiFi<\/Record>"/ >/tmp/b2
	cat /tmp/b2 | sed s/"<Record name=\"dmsb.l2net.5.Port.2.Name\" type=\"astr\" \/>"/"<Record name=\"dmsb.l2net.5.Port.2.Name\" type=\"astr\">ath14<\/Record>"/ >/tmp/b1
	cp /tmp/b1 $2
	rm /tmp/b1
	rm /tmp/b2
	fi

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

        grep "<Record name=\"dmsb.hotspot.gre.1.LocalInterfaces\" type=\"astr\">Device.WiFi.SSID.5.,Device.WiFi.SSID.6.,Device.WiFi.SSID.9.,Device.WiFi.SSID.10.<\/Record>" $2
        if [  "$?" == "1" ] ; then
        cp $2 /tmp/b1
        cat /tmp/b1 | sed s/"<Record name=\"dmsb.hotspot.gre.1.LocalInterfaces\" type=\"astr\">Device.WiFi.SSID.5.,Device.WiFi.SSID.6.<\/Record>"/"<Record name=\"dmsb.hotspot.gre.1.LocalInterfaces\" type=\"astr\">Device.WiFi.SSID.5.,Device.WiFi.SSID.6.,Device.WiFi.SSID.9.,Device.WiFi.SSID.10.<\/Record>"/ >/tmp/b2
        cp /tmp/b2 $2
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
			REDIRCTEXISTS=`cat $SYSCFG_DB_FILE | grep redirection_flag | cut -f2 -d=`
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
	# Check if Hotspot Max Num of STA is updated.
        grep "<Record name=\"dmsb.hotspot.max_num_sta_set\" type=\"astr\">1<\/Record>" $2
	if [ "$?" == "1" ];then
                                cp $2 /tmp/b1
                                cat /tmp/b1 | sed s/"<Record name=\"dmsb.hotspot.max_num_sta_set\" type=\"astr\">0<\/Record>"/"<Record name=\"dmsb.hotspot.max_num_sta_set\" type=\"astr\">1<\/Record>"/ >/tmp/b2
                                cp /tmp/b2 $2
                                rm /tmp/b1
                                rm /tmp/b2
		if [ "$IS_BCI" = "yes" ]; then
			grep "<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.5.BssMaxNumSta\" type=\"astr\">15<\/Record>" $2
			if [  "$?" == "1" ] ; then
				cp $2 /tmp/b1
				cat /tmp/b1 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.5.BssMaxNumSta\" type=\"astr\">5<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.5.BssMaxNumSta\" type=\"astr\">15<\/Record>"/ >/tmp/b2
				cat /tmp/b2 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.6.BssMaxNumSta\" type=\"astr\">5<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.6.BssMaxNumSta\" type=\"astr\">15<\/Record>"/ >/tmp/b1
                                cat /tmp/b1 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.BssMaxNumSta\" type=\"astr\">30<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.BssMaxNumSta\" type=\"astr\">15<\/Record>"/ >/tmp/b2
                                cat /tmp/b2 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.10.BssMaxNumSta\" type=\"astr\">30<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.10.BssMaxNumSta\" type=\"astr\">15<\/Record>"/ >/tmp/b1
				cp /tmp/b1 $2
				rm /tmp/b1
				rm /tmp/b2
			fi
		else
                        grep "<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.BssMaxNumSta\" type=\"astr\">5<\/Record>" $2
                        if [  "$?" == "1" ] ; then
                                cp $2 /tmp/b1
                                cat /tmp/b1 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.BssMaxNumSta\" type=\"astr\">30<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.9.BssMaxNumSta\" type=\"astr\">5<\/Record>"/ >/tmp/b2
                                cat /tmp/b2 | sed s/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.10.BssMaxNumSta\" type=\"astr\">30<\/Record>"/"<Record name=\"eRT.com.cisco.spvtg.ccsp.tr181pa.Device.WiFi.AccessPoint.10.BssMaxNumSta\" type=\"astr\">5<\/Record>"/ >/tmp/b1
                                cp /tmp/b1 $2
                                rm /tmp/b1
                                rm /tmp/b2
                        fi
		fi

	fi
fi



