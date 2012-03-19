#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to stresent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 CNES
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistri will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
#
#

# Author: Julien BERNARD / Viveris Technologies <jbernard@toulouse.viveris.com>

import os

for lib in os.listdir(os.path.dirname(__file__)):
    if not lib.endswith(".py"):
        continue
    if os.path.basename(lib) == os.path.basename(__file__):
        continue
    __import__(os.path.basename(lib).rstrip('.py'),
               globals(), locals(), [''], -1)
