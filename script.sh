#!/bin/bash

echo "/run/beamOn 50000000" > tmp.mac

for matName in plastic
do
    for((i=0;i<200;i++))
    do
	./$matName --grid -s -m tmp.mac
	hadd -f $matName$i.root /dev/shm/result_t*
    done
done

rm -f /dev/shm/result*
