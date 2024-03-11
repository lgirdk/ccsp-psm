echo -e "Starting tests....."

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

CURFILE=/tmp/bbhm_lite_cur_cfg.xml

#Get single param value
GET_TEST1=`psmcli get dmsb.BridgeNamePrefix`

if [ "$GET_TEST1" = "brlan" ]
then
        echo -e "${GREEN}1: Success- Get single param"
else
        echo -e "${RED}1: Fail - Get single param : $GET_TEST1" 
fi

#Get multiple param values

GET_TEST2=`psmcli get dmsb.BridgeNamePrefix dmsb.l2net.1.AllowDelete`
get_test2_arr=($GET_TEST2)


if [ "${get_test2_arr[0]}" = "brlan" ]; then
    if [ "${get_test2_arr[1]}" = "FALSE" ]; then
        echo -e "${GREEN}2: Success - Get multiple param values"
    else
        echo -e "${RED}2: Fail - Get multiple param values: 2nd param ${get_test2_arr[1]}"
    fi
else
    echo -e "${RED}2: Fail - Get multiple param values: 1st param ${get_test2_arr[0]}"
fi

#Set single param value

SET_TEST3=`psmcli set dmsb.l2net.1.AllowDelete TRUE`

if [ $? -eq 100 ]; then
    echo -e "${GREEN}3: Success - Set single param. psmcli return good."

    SET_TEST4=`psmcli get dmsb.l2net.1.AllowDelete`

    if [ "$SET_TEST4" = "TRUE" ]; then
        echo -e "${GREEN}4: Success - Set single param. confirmed via get"
    else
        echo -e "${RED}4: Fail - Set single param. Cannot confimr via get"
    fi
else
    echo -e "${RED}3: Fail - Set single param."
fi

#Set multiple params

SET_TEST5=`psmcli set dmsb.l2net.2.AllowDelete TRUE dmsb.l2net.3.AllowDelete TRUE`

if [ $? -eq 100 ]; then
    echo -e "${GREEN}5: Success - Set multiple params"
    result=`psmcli set dmsb.l2net.1.AllowDelete FALSE dmsb.l2net.2.AllowDelete FALSE dmsb.l2net.3.AllowDelete FALSE`
else
    echo -e "${RED}5: Fail - Set multiple params"
fi

# Add params

ADD_TEST6=`psmcli set test.custom.param.1 abc test.custom.param.2 10 test.custom.param.3 192.168.9.10`

if [ $? -eq 100 ]; then
    echo -e "${GREEN}6: Success - Add params. psmcli return good"

     ADD_TEST7=`psmcli get test.custom.param.1 test.custom.param.2 test.custom.param.3`
     add_test7_arr=($ADD_TEST7)

     pass=0

     if [ "${add_test7_arr[0]}" = "abc" ]; then
         if [ "${add_test7_arr[1]}" = "10" ]; then
             if [ "${add_test7_arr[2]}" = "192.168.9.10" ]; then
                 pass=1
                 echo -e "${GREEN}7: Success - Add params. Confirmed via get"
             fi
         fi
     fi

     if [ "$pass" = "0" ]; then
         echo -e "${RED}7: Fail - Add Params. Not confirmed via get"
     fi
else
    echo -e "${RED}6: Fail - Add params"
fi
        

# Delete single param

DEL_TEST8=`psmcli del test.custom.param.1`

if [ $? -eq 100 ]; then
    echo -e "${GREEN}8: Success - Delete single Param. psmcli return good"

    DEL_TEST9=`psmcli get test.custom.param.1`
    
    if [ "${DEL_TEST9}" = "" ]; then
        echo -e "${GREEN}9: Success - Delete single Param. Confirmed via get"
    else
        echo -e "${RED}9: Fail - Delete single Param. Not confirmed via get"
    fi
else
    echo -e "${RED}8: Fail - Delete single Param"
fi


#Delete multiple params

DEL_TEST10=`psmcli del test.custom.param.2 test.custom.param.3`

if [ $? -eq 100 ]; then
    echo -e "${GREEN}10: Success - Delete multiple Params. psmcli return good"

    DEL_TEST11=`psmcli get test.custom.param.2 test.custom.param.3`
  
    if [ "${DEL_TEST11}" = "" ]; then
        echo -e "${GREEN}11: Success - Delete multiple Param. Confirmed via get"
    else
        echo -e "${RED}11: Fail - Delete multiple Param. Not confirmed via get"
    fi
else
    echo -e "${RED}10: Fail - Delete multiple Param"
fi

IFS_BAK=${IFS}

IFS="
"

# Get all instances l2net

#### Code to dynamically get instances ######

#records_def=( $(grep "dmsb.l2net." /usr/ccsp/config/bbhm_def_cfg.xml))

#index=0

#for i in "${records_def[@]}"
#do
#        temp=`echo $i | cut -d \" -f2 | cut -d . -f -3`
#        instances[index]=${temp:11}
#        index=$((index+1))
#done

#records_cur=( $(grep "dmsb.l2net." /nvram/bbhm_cur_cfg.xml))

#for i in "${records_cur[@]}"
#do
#        temp=`echo $i | cut -d \" -f2 | cut -d . -f -3`
#        instances[index]=${temp:11}
#        index=$((index+1))
#done

#sorted_unique_inst=($(echo "${instances[@]}" | tr ' ' '\n' | sort -n | tr '\n' ' ' | xargs))

#### Code to dynamically get instances - End ######

instances="1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21"

GET_INST12=`psmcli getallinst dmsb.l2net.`
res_sorted_unique_inst=$(echo "${GET_INST12[@]}" | tr ' ' '\n' | sort -n | tr '\n' ' ' | xargs)

if [ "$instances" == "$res_sorted_unique_inst" ]; then
    echo -e "${GREEN}12: Success - Get all instances for l2net"
else
    echo -e "${RED}12: Fail - Get all instances for l2net"
fi

# Get all instances EthLink (non l2net)
instances="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17"

GET_INST13=`psmcli getallinst dmsb.EthLink.`
res_sorted_unique_inst=($(echo "${GET_INST13[@]}" | tr ' ' '\n' | sort -n | tr '\n' ' ' | xargs))

if [ "$instances" == "$res_sorted_unique_inst" ]; then
    echo -e "${GREEN}13: Success - Get all instances for EthLink (Non l2net)"
else
    echo -e "${RED}13: Fail - Get all instances for EthLink (Non 2net)"
fi

IFS=${IFS_BAK}

# Test New current config file operation

SET_EXIST=`psmcli set dmsb.l2net.1.AllowDelete TRUE`
SET_NEW=`psmcli set something.custom.1 5`

sleep 7

result=`grep "dmsb.l2net.1.AllowDelete" $CURFILE | cut -d '>' -f2 | cut -d '<' -f1`

if [ "$result" = "TRUE" ]; then
    echo -e "${GREEN}14: Success - Set EXISTING var to non default value, param shows up in cur config file"

    SET_EXIST_BACK=`psmcli set dmsb.l2net.1.AllowDelete FALSE`

    if [ $? -eq 100 ]; then
        result=`psmcli get dmsb.l2net.1.AllowDelete`

        if [ "$result" = "FALSE" ]; then
            echo -e "${GREEN}15: Success - Set EXISTING var back to default value"
        else
            echo -e "${RED}15: Fail - Set EXISTING var back to default value"
        fi

        sleep 7

        if [ -f $CURFILE ]; then

            result=`grep "dmsb.l2net.1.AllowDelete" $CURFILE`

            if [ "$result" = "" ]; then
                echo -e "${GREEN}16: Success - Set EXISTING var back to default value, param removed from cur config file"
            else
                echo -e "${RED}16: Fail - Set EXISTING var back to default, param still in cur config file"
            fi
        else
            echo -e "${GREEN}16: Success - Set EXISTING var back to default value, param removed from cur config file"
        fi
       
    else
        echo -e "${RED}15: Fail - Set EXISTING var back to default value"
    fi
else
        echo -e "${RED}14: Fail - Set EXISTING var to non default value, param does not show in cur config file"       
fi

#set back to default value in case some of the above code fails
SET_BACK=`psmcli set dmsb.l2net.1.AllowDelete FALSE`

result=`grep "something.custom.1" $CURFILE | cut -d '>' -f2 | cut -d '<' -f1`

if [ "$result" = "5" ]; then
    echo -e "${GREEN}17: Success - Set(Add) NEW param, param shows up in cur config file"

    DEL_NEW=`psmcli del something.custom.1`

    sleep 7

    result=`grep "something.custom.1" $CURFILE`

    if [ -f $CURFILE ]; then
        if [ "$result" = "" ]; then
            echo -e "${GREEN}18: Success - Delete NEW param, param removed from cur config file"
        else
            echo -e "${RED}18: Fail - Delete NEW param, param still in cur config file"
        fi
    else
        echo -e "${GREEN}18: Success - Delete NEW param, param removed from cur config file"
    fi
else
    echo -e "${RED}17: Fail - Set(Add) New param, param does not show up in cur config file"
fi

echo -e "${NC} Finished running tests"






