# Copyright (C) 2018  Jacek Danecki <jacek.m.danecki@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import math
from decimal import Decimal

iter = 0

def f(zx, zy):
    jx = math.fsum([math.pow(zx,2),-math.pow(zy,2), 0.15])
    jy = math.fsum([zx * zy, zx * zy, -0.6])
    d = math.pow(jx,2) + math.pow(jy,2)
    return [jx, jy,d]

def c(zx, zy):
    global iter
    print ("zx={} zy={}".format(zx, zy))
    (jx, jy, d) = f(zx, zy)
#    print ("jx={} jy={} d={} iter={}".format(jx, jy, d, iter))
    if (d > 4):
        return iter
    iter = iter+1
    c(jx, jy)

#c(-0.09509506076574325562, -0.50412118434906005859)
#c(-0.00000000000000000001, -0.00000000000000000001)
c(Decimal('-0.00000000000000000001'), Decimal('-0.00000000000000000001'))

print "iter={}".format(iter)

