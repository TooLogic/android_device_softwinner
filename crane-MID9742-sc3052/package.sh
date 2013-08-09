#!/bin/bash
cd $PACKAGE
if [ "$1" = "-d" ]; then
	echo "--------debug version, have uart printf-------------"
	./pack -c sun4i -p crane -b MID9742-sc3052 -d card0
else
	echo "--------release version, donnot have uart printf-------------"
	./pack -c sun4i -p crane -b MID9742-sc3052 -d uart0
fi
cd -