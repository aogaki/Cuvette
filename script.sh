#!/bin/bash

for matName in plastic quartz glass
do
    for angle in 30 60 90 120 150 180 210 240 270 300 330 360
    do
	echo "/BI/Geometry/RotationY $angle" > tmp.mac
	echo "/run/beamOn 10000000" >>tmp.mac	
	./$matName -m tmp.mac
	hadd -f $matName$angle.root result_t*
    done
done
