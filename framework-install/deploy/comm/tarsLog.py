#!/usr/bin/python
# encoding: utf-8
import logging, os

class Logger:
    def __init__(self):
	return

    def debug(self, message):
        print message

    def info(self, message):
        print message

    def infoPrint(self,message):
        print message

    def war(self, message):
        print message

    def error(self, message):
        print message

    def cri(self, message):
        print message

loggermap = {}
def getLogger(logName="info.log"):
    if logName in loggermap:
        return loggermap[logName]
    else:
        logger = Logger()
        loggermap[logName] = logger
        return logger


if __name__ == '__main__':
    #logyyx = Logger('d://yyx.log', logging.ERROR, logging.DEBUG)
    log = getLogger()
    log.info("kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk")
