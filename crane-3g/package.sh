cd $PACKAGE
if [ "$1" = "-d" ]; then
	echo "--------debug version, have uart printf-------------"
	./pack -c sun4i -p crane -b 3g -d card0
else
	echo "--------release version, donnot have uart printf-------------"
	./pack -c sun4i -p crane -b 3g -d uart0
fi
cd -      

