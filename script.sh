#!/bin/bash

echo "/run/beamOn 10000000" > tmp.mac

./cuvetteGlass --grid -m tmp.mac
hadd -f glass.root result_t*

./cuvettePlastic --grid -m tmp.mac
hadd -f plastic.root result_t*

./cuvetteQuartz --grid -m tmp.mac
hadd -f quartz.root result_t*
