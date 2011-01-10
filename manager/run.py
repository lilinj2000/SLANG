#! /usr/bin/python

import logging
import logging.handlers
from manager import Manager
import manager
from config import Config

# set up logging
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

lformat = logging.Formatter('%(asctime)s %(levelname)s %(name)s: %(message)s')

ls = logging.handlers.SysLogHandler(address='/dev/log')
ls.setFormatter(lformat)
ls.setLevel(logging.DEBUG)
logger.addHandler(ls)

lc = logging.StreamHandler()
lc.setLevel(logging.DEBUG)
lc.setFormatter(lformat)
logger.addHandler(lc)

# read config
c = Config("../probed/settings.xml")

# start up
logger.debug("Starting up...")

m = Manager()
m.run()
