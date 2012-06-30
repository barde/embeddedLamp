#!/usr/bin/python

import sys
import csv

if len(sys.argv) < 2:
    print "usage: " + sys.argv[0] + " [output file]"
    sys.exit()

j = 0
i = 0
with open(sys.argv[1]) as f:
    binWriter = csv.writer(open(sys.argv[1] + ".csv", 'ab'))
    for line in f:
        binWriter.writerow(line)
        i = i + int(line)
        j += 1

print i/j
