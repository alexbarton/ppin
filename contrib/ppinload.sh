#!/bin/bash

[ -w /dev/ppin ] || exit 1
[ "$1" = "-v" ] && VERBOSE=1 || unset VERBOSE

export LC_ALL=C

function set_ppin_up_to()
{
	declare -i count="$1"
	declare -i n=0
	while [ $n -lt 8 ]; do
		[ $n -lt $count ] \
			&& echo "$n on" >/dev/ppin \
			|| echo "$n off" >/dev/ppin
	n=$n+1
	done
}

function load_ppin()
{
	if [ $load -lt 3 ]; then	#    < 0.03: 0 PINs	d=0.03
		set_ppin_up_to 0
	elif [ $load -lt 10 ]; then	# 0.03-0.09: 1 PIN	d=0.07
		set_ppin_up_to 1
	elif [ $load -lt 24 ]; then	# 0.10-0.24: 2 PIN	d=0.15
		set_ppin_up_to 2
	elif [ $load -lt 50 ]; then	# 0.25-0.49: 3 PINs	d=0.25
		set_ppin_up_to 3
	elif [ $load -lt 100 ]; then	# 0.50-0.99: 4 PINs	d=0.50
		set_ppin_up_to 4
	elif [ $load -lt 200 ]; then	# 1.00-1.99: 5 PINs	d=1.00
		set_ppin_up_to 5 
	elif [ $load -lt 400 ]; then	# 2.00-3.99: 6 PINs	d=2.00
		set_ppin_up_to 6
	elif [ $load -lt 800 ]; then	# 4.00-7.99: 7 PINs	d=4.50
		set_ppin_up_to 7
	else				#    > 8.00: 8 PINs
		set_ppin_up_to 8
	fi
}

while true; do
	declare load_raw=`cat /proc/loadavg | cut -d' ' -f1`
	declare -i load=`echo $load_raw | sed -e "s/\.//" | sed -e "s/^0*//g"`
	[ -n "$VERBOSE" ] && echo "Load: $load_raw"
	load_ppin
	sleep 1
done
