#!/usr/bin/python

import sys

j = 0
i = 0
with open(sys.argv[1]) as f:
	for line in f:
		i = i + int(line)
		j += 1
print i/j
		
