#!/bin/bash
# by Paul Colby (http://colby.id.au), no rights reserved ;)
 

function scale()
{
	DIFF_USAGE_SCALED=$((DIFF_USAGE*40/100))
}


function generate_bitstream()
{
	# reset bit stream	
	bitstream=""
	
	# generate 0's
	for (( i=1; i<=40-DIFF_USAGE_SCALED; i++ ))
	do
		bitstream="${bitstream}0"
	done

	# generate 1's
	for (( i=1; i<=DIFF_USAGE_SCALED; i++ ))
	do
		bitstream="${bitstream}1"
	done
}


function bitstream_to_hex()
{
	

	# ########## get 5 byte out of 40 bits ##########
	byte_1=`echo "$bitstream" | cut -c1-8`
	byte_2=`echo "$bitstream" | cut -c9-16`
	byte_3=`echo "$bitstream" | cut -c17-24`
	byte_4=`echo "$bitstream" | cut -c25-32`
	byte_5=`echo "$bitstream" | cut -c33-40`


	# ########## fix the wiring bug (last byte needs to be put to front) ##########
	# byte 1	
	bit=` echo "$byte_1" | cut -c1-1`
	rest=`echo "$byte_1" | cut -c2-8`

	byte_1="${rest}${bit}"


	# byte 2
	bit=` echo "$byte_2" | cut -c1-1`
	rest=`echo "$byte_2" | cut -c2-8`

	byte_2="${rest}${bit}"


	# byte 3
	bit=` echo "$byte_3" | cut -c1-1`
	rest=`echo "$byte_3" | cut -c2-8`

	byte_3="${rest}${bit}"


	# byte 4
	bit=` echo "$byte_4" | cut -c1-1`
	rest=`echo "$byte_4" | cut -c2-8`

	byte_4="${rest}${bit}"


	# byte 5
	bit=` echo "$byte_5" | cut -c1-1`
	rest=`echo "$byte_5" | cut -c2-8`

	byte_5="${rest}${bit}"




	# ########## convert to hex ##########
	byte_1_hex=`echo "ibase=2;$byte_1" | bc`		# to decimal
	byte_1_hex=`echo "ibase=10;obase=16;$byte_1_hex" | bc`	# to hex
	str_length=`echo ${#byte_1_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_1_hex="0${byte_1_hex}"
	fi


	byte_2_hex=`echo "ibase=2;$byte_2" | bc`		# to decimal
	byte_2_hex=`echo "ibase=10;obase=16;$byte_2_hex" | bc`	# to hex
	str_length=`echo ${#byte_2_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_2_hex="0${byte_2_hex}"
	fi


	byte_3_hex=`echo "ibase=2;$byte_3" | bc`		# to decimal
	byte_3_hex=`echo "ibase=10;obase=16;$byte_3_hex" | bc`	# to hex
	str_length=`echo ${#byte_3_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_3_hex="0${byte_3_hex}"
	fi


	byte_4_hex=`echo "ibase=2;$byte_4" | bc`		# to decimal
	byte_4_hex=`echo "ibase=10;obase=16;$byte_4_hex" | bc`	# to hex
	str_length=`echo ${#byte_4_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_4_hex="0${byte_4_hex}"
	fi


	byte_5_hex=`echo "ibase=2;$byte_5" | bc`		# to decimal
	byte_5_hex=`echo "ibase=10;obase=16;$byte_5_hex" | bc`	# to hex
	str_length=`echo ${#byte_5_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_5_hex="0${byte_5_hex}"
	fi

}





EXPECTED_ARGS=2
E_BADARGS=65


if [ $# -ne $EXPECTED_ARGS ]
then
  echo "Usage: `basename $0` {wlan device} {output device} [output value]"
  exit $E_BADARGS
fi


# Parameter
wlan_iface=$1	# wlan interface
out_device=$2	# output device
bitstream=""	# will hold the bit stream




PREV_TOTAL=0
PREV_IDLE=0

 
while true; do
  CPU=(`cat /proc/stat | grep '^cpu '`) # Get the total CPU statistics.
  unset CPU[0]                          # Discard the "cpu" prefix.
  IDLE=${CPU[4]}                        # Get the idle CPU time.
 
  # Calculate the total CPU time.
  TOTAL=0
  for VALUE in "${CPU[@]}"; do
    let "TOTAL=$TOTAL+$VALUE"
  done
 
  # Calculate the CPU usage since we last checked.
  let "DIFF_IDLE=$IDLE-$PREV_IDLE"
  let "DIFF_TOTAL=$TOTAL-$PREV_TOTAL"
  let "DIFF_USAGE=(1000*($DIFF_TOTAL-$DIFF_IDLE)/$DIFF_TOTAL+5)/10"
  #echo -en "\rCPU: $DIFF_USAGE%  \b\b"



  scale
  generate_bitstream
  bitstream_to_hex




  echo -e "\n\n----------------------------------------------------------------"

  echo -e "\rCPU: $DIFF_USAGE%  \b\b"

  # output
  echo -e "CPU scaled: $DIFF_USAGE_SCALED/40"
  echo -e "bitstream: $bitstream "

  echo -e "          bit    : hex"
  echo -e "byte 1: $byte_1 : $byte_1_hex"
  echo -e "byte 2: $byte_2 : $byte_2_hex"
  echo -e "byte 3: $byte_3 : $byte_3_hex"
  echo -e "byte 4: $byte_4 : $byte_4_hex"
  echo -e "byte 5: $byte_5 : $byte_5_hex"


  # write output to char device
  echo "${byte_1_hex}${byte_2_hex}${byte_3_hex}${byte_4_hex}${byte_5_hex}" > $out_device

   

  # Remember the total and idle CPU times for the next check.
  PREV_TOTAL="$TOTAL"
  PREV_IDLE="$IDLE"
 
  # Wait before checking again.
  sleep 1
done

