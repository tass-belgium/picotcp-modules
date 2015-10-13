#!/bin/bash
rm -f /tmp/pico-modules-mem-report-*

./build/test/units/modunit_libhttp_client.elf || exit 1

MAXMEM=`cat /tmp/pico-modules-mem-report-* | sort -r -n |head -1`
echo
echo
echo
echo "MAX memory used: $MAXMEM"
rm -f /tmp/pico-modules-mem-report-*

echo "SUCCESS!" && exit 0
