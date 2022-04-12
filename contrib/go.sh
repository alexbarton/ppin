#!/bin/bash

[ -w /dev/ppin ] || exit 1

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

while true; do
	for i in 0 1 2 3 4 5 6 7; do
		set_ppin_up_to $i
		sleep 1
	done
	for i in 7 6 5 4 3 2 1 0; do
		set_ppin_up_to $i
		sleep 1
	done
done
