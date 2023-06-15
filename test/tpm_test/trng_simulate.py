#!/usr/bin/env python3
# Copyright 2023 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Module for simulation TRNG output for NIST entropy testing"""

import sys
from scipy.stats import invgauss

# NIST requires 1000000 samples
numcolls = 1000000
# number of bits in sample
bits = int(1)
bitmask = (1 << bits) - 1

if len(sys.argv) != 2:
    print("Use: python3 trng_simulate.py file_prefix")
    sys.exit(1)

out_file = str(sys.argv[1])

# create bytearray for lsbs
lsb_bytearray = [None]*(int(numcolls))
lsb_count = [0]*(int(1 << bits))

# determine shape of inverse gaussian distribution
mu = 0.785
loc = 0
scale = 1525
timeout = 0xfff

# confirm that mean, var, skew closely match model expectations
mean, var, skew, kurt = invgauss.stats(mu, loc=loc, scale=scale, moments='mvsk')

print(f"mu = {mu:.3f}\nloc = {loc:.3f}\nscale = {scale:.3f}\n"
      f"mean = {mean:.3f}\nvar = {var:.3f}\nskew = {skew:.3f}\n"
      f"kurt = {kurt:.3f}\nlamda = {mean*mean*mean/var:.3f}")

# create numcolls collapse values, generate 2x to handle timeouts
r = invgauss.rvs(mu, loc=loc, scale=scale, size=numcolls*2)

# basic stats tracking
byteval = 0
arrloc = 0
timeouts = 0
print(f"Raw generated length = {len(r)} max = {max(r)} min = {min(r)}")
for rval in r:
    if int(rval) > timeout:
        timeouts += 1
        continue
    # grab the lsb from the current collapse
    lsbval = int(rval) & bitmask
    lsb_count[lsbval] += 1
    # store lsbval as byte, e.g. 8'b0000_0000 or 8'b0000_0001
    lsb_bytearray[arrloc] = lsbval
    arrloc += 1
    if arrloc >= numcolls:
        break

print(f"timeouts = {timeouts}")
print(f"bin size = {len(lsb_bytearray)}\t"
      f"max = {max(lsb_bytearray)}\tvalue min = {min(lsb_bytearray)}")
for i, count in enumerate(lsb_count):
    print(f"P[{i}] = {count / numcolls}")
with open(f"{out_file}", "wb") as fbin:
    fbin.write(bytearray(lsb_bytearray))
    fbin.close()
