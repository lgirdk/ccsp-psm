echo -e "Starting tests ..."

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Set new values
# format: psmcli  set <obj1 name> <obj1 value> <obj2 name> <obj2 value> ...
SET_TEST1=`psmcli  set ccsp.landevice.1.ipaddr 192.168.100.2 ccsp.landevice.2.ipaddr 192.168.200.2 ccsp.landevice.3.ipaddr 192.168.300.2 ccsp.landevice.1.macaddr 10:78:d2:e5:aa:34`

if [ $? -eq 100 ]
then
        echo -e "${GREEN}1: set success"
else
        echo -e "${RED}1: set fail"
fi

# Set detail
# format: psmcli  setdetail <obj 1 datatype> <obj1 name> <obj1 value> <obj2 datetype> <obj2 name> <obj2 value> ...
SETDETAIL_TEST2=`psmcli  setdetail string ccsp.landevice.1.ipaddr 192.168.100.2 string ccsp.landevice.2.ipaddr 192.168.200.2 string ccsp.landevice.3.ipaddr 192.168.300.2 string ccsp.landevice.1.macaddr 10:78:d2:e5:aa:34`

if [ $? -eq 100 ]
then
        echo -e "${GREEN}2: setdetail success"
else
        echo -e "${RED}2: set fail"
fi

# Get test
# format: psmcli  get <obj1 name> <obj2 name> <obj3 name> ...
GET_TEST3=`psmcli  get ccsp.landevice.1.ipaddr`

if [ "$GET_TEST3" = "192.168.100.2" ]
then
        echo -e "${GREEN}3: get success"
else
        echo -e $GET_TEST3
fi

GET_TEST4=`psmcli  get ccsp.landevice.2.ipaddr`

if [ "$GET_TEST4" =  "192.168.200.2" ]
then
        echo -e "${GREEN}4: getdetail success"
else
        echo -e "${RED}4: fail, $GET_TEST4"
fi

# getdetail test
# format: psmcli  getdetail <obj1 name> <obj2 name> ... 
GET_TEST5=`psmcli  getdetail ccsp.landevice.3.ipaddr`

index=0
for x in $GET_TEST5
do
        if [ $index -eq 0 ]
        then
                if [ "$x" = "string" ]
                then
                        echo -e "${GREEN}5: getdetail success"
				else
				        echo -e "${RED}5: fail, $x"
        		fi
        elif [ $index -eq 1 ]
        then
                if [ "$x" = "192.168.300.2" ]
                then
                        echo -e "${GREEN}6: getdetail success"
				else
				        echo -e "${RED}6: fail, $x"
                fi
        fi
        index=$((index+1))
done

# get -e test
# format: psmcli  get -e <obj1 env var> <obj1 name> <obj2 env var> <obj2 name> ...
eval `psmcli  get -e LANDEVICE_1_IP ccsp.landevice.1.ipaddr LANDEVICE_2_IP ccsp.landevice.2.ipaddr`

if [ "$LANDEVICE_1_IP" = "192.168.100.2" ] && [ "$LANDEVICE_2_IP" = "192.168.200.2" ]
then
        echo -e "${GREEN}7: get -e success"
else
        echo -e "${RED}7: fail, $LANDEVICE_1_IP"
        echo -e "${RED}7: fail, $LANDEVICE_2_IP"
fi

# getdetail -e test
# format: psmcli  getdetail -e <obj1 env var> <obj1 name> <obj2 env var> <obj2 name> ...
eval `psmcli  getdetail -e LANDEVICE_1_IP ccsp.landevice.1.ipaddr LANDEVICE_2_IP ccsp.landevice.2.ipaddr`

if [ "$LANDEVICE_1_IP" = "192.168.100.2" ] && [ "$LANDEVICE_2_IP_TYPE" = "string" ]
then
        echo -e "${GREEN}8: getdetail -e success"
else
        echo -e "${RED}8: getdetail -e fail"
fi

# getallinst test
# format: psmcli  getallinst <obj name>
GET_ALL_INSTANCE_TEST9=`psmcli  getallinst ccsp.landevice. | tr ' ' '\n' | sort -n | tr '\n' ' ' | xargs`

index=0
for x in $GET_ALL_INSTANCE_TEST9
do
        if [ $index -eq 0 ]
        then
                if [ "$x" = "1" ]
                then
                        echo -e "${GREEN}9: getallinst success part 1"
				else
				        echo -e "${RED}9: fail, getallinst fail part 1"
                fi
        elif [ $index -eq 1 ]
        then
                if [ "$x" = "2" ]
                then
                        echo -e "${GREEN}9: getallinst success part 2"
				else
				        echo -e "${RED}9: fail, getallinst fail part 2"
                fi
        elif [ $index -eq 2 ]
        then
                if [ "$x" = "3" ]
                then
                        echo -e "${GREEN}9: getallinst success part 3"
				else
				        echo -e "${RED}9: fail, getallinst fail part 3"
                fi
        fi
        index=$((index+1))
done

# getinstcnt test
# format: psmcli  getinstcnt <obj1 name> <obj2 name> ...
GET_INSTANCE_CNT_TEST10=`psmcli  getinstcnt ccsp.landevice.`
if [ "$GET_INSTANCE_CNT_TEST10" = "3" ]
then
        echo -e "${GREEN}10: getinstcnt success"
else
        echo -e "${RED}10: fail, $GET_INSTANCE_CNT_TEST10"
fi

# del test
DEL_TEST11=`psmcli  del ccsp.landevice.1.ipaddr ccsp.landevice.2.ipaddr ccsp.landevice.3.ipaddr ccsp.landevice.1.macaddr`
if [ $? -eq 100 ]
then
	echo -e "${GREEN}11: del success"
else
    echo -e "${RED}11: del fail "
fi

echo -e "${NC} End of tests"