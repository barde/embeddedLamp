#!/bin/bash


function scale()
{
	signal_scaled=$((signal*40/70))
}


function generate_bitstream()
{
	# reset bit stream	
	bitstream=""
	
	# generate 0's
	for (( i=1; i<=40-signal_scaled; i++ ))
	do
		bitstream="${bitstream}0"
	done

	# generate 1's
	for (( i=1; i<=signal_scaled; i++ ))
	do
		bitstream="${bitstream}1"
	done

}


function bitstream_to_hex()
{
	# get 51byte out of 40 bits	
	byte_1=`echo "$bitstream" | cut -c1-8`
	byte_2=`echo "$bitstream" | cut -c9-16`
	byte_3=`echo "$bitstream" | cut -c17-24`
	byte_4=`echo "$bitstream" | cut -c25-32`
	byte_5=`echo "$bitstream" | cut -c33-40`


	# WORKAROUND  move last bit to the front
#byte_1="00000001"
#byte_2="00000001"
#byte_3="00000001"
#byte_4="00000001"
#byte_5="00000001"



	#bit=` echo "$byte_1" | cut -c8-8`
	#rest=`echo "$byte_1" | cut -c1-7`
	bit=` echo "$byte_1" | cut -c1-1`
	rest=`echo "$byte_1" | cut -c2-8`




	echo "byte before: $byte_1"
	echo "bit: $bit"
	echo "rest: $rest"
	#byte_1="${bit}${rest}"
	byte_1="${rest}${bit}"
	echo "byte after: $byte_1"



	#bit=` echo "$byte_2" | cut -c8-8`
	#rest=`echo "$byte_2" | cut -c1-7`
	bit=` echo "$byte_2" | cut -c1-1`
	rest=`echo "$byte_2" | cut -c2-8`

	byte_2="${rest}${bit}"
#byte_2="${bit}${rest}"

	#bit=` echo "$byte_3" | cut -c8-8`
	#rest=`echo "$byte_3" | cut -c1-7`
	bit=` echo "$byte_3" | cut -c1-1`
	rest=`echo "$byte_3" | cut -c2-8`

#	byte_3="${bit}${rest}"
	byte_3="${rest}${bit}"

	#bit=` echo "$byte_4" | cut -c8-8`
	#rest=`echo "$byte_4" | cut -c1-7`
	bit=` echo "$byte_4" | cut -c1-1`
	rest=`echo "$byte_4" | cut -c2-8`

	byte_4="${rest}${bit}"
#byte_4="${bit}${rest}"

	#bit=` echo "$byte_5" | cut -c8-8`
	#rest=`echo "$byte_5" | cut -c1-7`
	bit=` echo "$byte_5" | cut -c1-1`
	rest=`echo "$byte_5" | cut -c2-8`

	byte_5="${rest}${bit}"
#byte_5="${bit}${rest}"



	# convert to hex
	byte_1_hex=`echo "ibase=2;$byte_1" | bc`				# to decimal
	byte_1_hex=`echo "ibase=10;obase=16;$byte_1_hex" | bc`	# to hex
	str_length=`echo ${#byte_1_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_1_hex="0${byte_1_hex}"
	fi


	byte_2_hex=`echo "ibase=2;$byte_2" | bc`				# to decimal
	byte_2_hex=`echo "ibase=10;obase=16;$byte_2_hex" | bc`	# to hex
	str_length=`echo ${#byte_2_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_2_hex="0${byte_2_hex}"
	fi


	byte_3_hex=`echo "ibase=2;$byte_3" | bc`				# to decimal
	byte_3_hex=`echo "ibase=10;obase=16;$byte_3_hex" | bc`	# to hex
	str_length=`echo ${#byte_3_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_3_hex="0${byte_3_hex}"
	fi


	byte_4_hex=`echo "ibase=2;$byte_4" | bc`				# to decimal
	byte_4_hex=`echo "ibase=10;obase=16;$byte_4_hex" | bc`	# to hex
	str_length=`echo ${#byte_4_hex}`
	if [ $str_length -eq 1 ]
	then
		byte_4_hex="0${byte_4_hex}"
	fi


	byte_5_hex=`echo "ibase=2;$byte_5" | bc`				# to decimal
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
  echo "Usage: `basename $0` {wlan device} {output device}"
  exit $E_BADARGS
fi


# Parameter
wlan_iface=$1	# wlan interface
out_device=$2	# output device
bitstream=""	# will hold the bit stream



# loop
while true; do
	quality="`iwconfig $wlan_iface | grep Link`"

	signal="`echo $quality | grep -Po  'Quality=(\d)+' | grep -Po  '(\d)+' `"

	# scale the signal quality
	scale

	# generate the bit stream for the scaled signal quality
	generate_bitstream

	# convert bitstream to hex stream
	bitstream_to_hex

	echo "$signal_scaled"
	echo "$bitstream"

	echo "byte 1: $byte_1 : $byte_1_hex"
	echo "byte 2: $byte_2 : $byte_2_hex"
	echo "byte 3: $byte_3 : $byte_3_hex"
	echo "byte 4: $byte_4 : $byte_4_hex"
	echo "byte 5: $byte_5 : $byte_5_hex"
 


	#echo "$signal_scaled" >> $out_device

	echo "${byte_1_hex}${byte_2_hex}${byte_3_hex}${byte_4_hex}${byte_5_hex}"
	echo "${byte_1_hex}${byte_2_hex}${byte_3_hex}${byte_4_hex}${byte_5_hex}" > $out_device



	sleep 1
done


exit 

