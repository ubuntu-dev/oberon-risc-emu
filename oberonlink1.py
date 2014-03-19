#!/usr/bin/env python
# encoding: utf-8
"""
oberonlink1.py

Created by Ulrich Hoffmann on 2014-03-17.
"""

import sys
import getopt
import os
import exceptions
import time

help_message = '''\
Usage: operonlink1.py [options] send|receive [filename]*

options:
   -h --help    this help
   -v --verbose progress output\
'''

RISC_SERIAL_INPUT="/tmp/risc-serial-input"
RISC_SERIAL_OUTPUT="/tmp/risc-serial-output"

BlkLen = 255
REQ = 0x20
REC = 0x21
SND = 0x22
ACK = 0x10
NAK = 0x11

def receive(risc_output):
    while 1:
        try:
            ch=os.read(risc_output,1)
            return ord(ch)
        except OSError, e:
            if e.errno!=35: # EAGAIN
                raise
            time.sleep(0.1)
            
def send(risc_input, bytes):
    while 1:
        try:
            os.write(risc_input, bytes)
            return
        except OSError, e:
            if e.errno!=35: # EAGAIN
                raise
            time.sleep(0.1)

def expectAck(risc_output):
    x=receive(risc_output)
    if x!=ACK:
        raise exceptions.Exception("ack expected",x)

def sendFile((risc_input, risc_output), verbose, filename):
    if verbose:
        print "sending", filename
    send(risc_input, chr(REC))
    send(risc_input, filename+chr(0))
    f=open(filename)
    s=f.read() # .replace("\012","\015")
    expectAck(risc_output)
    while len(s)>BlkLen:
        send(risc_input, chr(BlkLen))
        send(risc_input, s[:BlkLen])
        s=s[BlkLen:]
        expectAck(risc_output)
    send(risc_input, chr(len(s)))
    send(risc_input, s)
    expectAck(risc_output)

def receiveFile((risc_input, risc_output), verbose, filename):
    if verbose:
        print "receiving", filename
    send(risc_input, chr(SND))
    send(risc_input, filename+chr(0))
    x=receive(risc_output)
    if x==NAK:
        raise exceptions.Exception("No such file", filename)
    b=[]
    try:
        l=receive(risc_output)
        while l == BlkLen:
            for i in range(l):
                b.append(chr(receive(risc_output)))
            send(risc_input, chr(ACK))
            l=receive(risc_output)
    
        for i in range(l):
            b.append(chr(receive(risc_output)))
        send(risc_input, chr(ACK))
        open(filename, "w").write("".join(b))
    except:
        send(risc_input, chr(NAK))
        raise
    
def openFifos():
    risc_input=os.open(RISC_SERIAL_INPUT, os.O_WRONLY | os.O_NONBLOCK)
    risc_output=os.open(RISC_SERIAL_OUTPUT, os.O_RDONLY | os.O_NONBLOCK)
    return (risc_input, risc_output)

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg


def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "vh", ["verbose", "help"])
        except getopt.error, msg:
            raise Usage(msg)

        # option processing
        verbose = False
        for option, value in opts:
            if option in ("-v", "--verbose"):
                verbose = True
            if option in ("-h", "--help"):
                raise Usage(help_message)

        if len(args)==0:
            print >> sys.stderr, str(help_message)
            return 3
            
        direction=args[0]
        args=args[1:]

        if direction not in [ "send", "receive" ]:
            raise Usage, ("verb must be 'send' or 'receive', not '%s' "
                          % (direction,))

    except Usage, err:
        print >> sys.stderr, sys.argv[0].split("/")[-1] + ": " + str(err.msg)
        return 2

    io=openFifos()
    if len(args)>0:
        for f in args:
            if direction == "send":
               sendFile(io,verbose,f)
            else:
               receiveFile(io,verbose,f)

if __name__ == "__main__":
    sys.exit(main())
