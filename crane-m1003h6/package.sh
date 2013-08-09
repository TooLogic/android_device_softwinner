#!/bin/bash

cd $PACKAGE
if [ "$1" = "-d" ]; then
	echo "--------debug version, have uart printf-------------"
	./pack -c sun4i -p crane -b m1003h6 -d card0 
else
	echo "--------release version, donnot have uart printf-------------"
	./pack -c sun4i -p crane -b m1003h6 -d uart0
fi
cd -


