#!/usr/bin/env python

"""
pyboard interface

This module provides the Pyboard class, used to communicate with and
control the pyboard over a serial USB connection.

Example usage:

    import pyboard
    pyb = pyboard.Pyboard('/dev/ttyACM0')

Or:

    pyb = pyboard.Pyboard('192.168.1.1')

Then:

    pyb.enter_raw_repl()
    pyb.exec('pyb.LED(1).on()')
    pyb.exit_raw_repl()

Note: if using Python2 then pyb.exec must be written as pyb.exec_.
To run a script from the local machine on the board and print out the results:

    import pyboard
    pyboard.execfile('test.py', device='/dev/ttyACM0')

This script can also be run directly.  To execute a local script, use:

    ./pyboard.py /dev/ttyACM0 test.py

Or:

    python pyboard.py /dev/ttyACM0 test.py

"""

import sys
import pyboard

def main():
    import argparse
    cmd_parser = argparse.ArgumentParser(description='Load files onto the microbit.')
    cmd_parser.add_argument('-b', '--baudrate', default=115200, help='the baud rate of the serial device')
    cmd_parser.add_argument('-u', '--user', default='micro', help='the telnet login username')
    cmd_parser.add_argument('-p', '--password', default='python', help='the telnet login password')
    cmd_parser.add_argument('-w', '--wait', default=0, type=int, help='seconds to wait for USB connected board to become available')
    cmd_parser.add_argument('--name', nargs=1, help='name to save the file as, if different from the file name')
    cmd_parser.add_argument('device', nargs=1, help='the serial device or the IP address of the microbit')
    cmd_parser.add_argument('file', nargs=1, help='file to load')
    args = cmd_parser.parse_args()

    def exec_commands(cmds):
        try:
            pyb = pyboard.Pyboard(args.device[0], args.baudrate, args.user, args.password, args.wait)
            pyb.enter_raw_repl()
            for cmd in cmds:
                ret, ret_err = pyb.exec_raw(cmd, timeout=None, data_consumer=pyboard.stdout_write_bytes)
                if ret_err:
                    break
            pyb.exit_raw_repl()
            pyb.close()
        except Exception as er:
            print(er)
            sys.exit(1)
        except KeyboardInterrupt:
            sys.exit(1)
        if ret_err:
            pyboard.stdout_write_bytes(ret_err)
            sys.exit(1)

    with open(args.file[0], 'rb') as f:
        pyfile = f.read()
        if args.name:
            name = args.name[0]
        else:
            name = args.file[0]
        script = make_save_script(pyfile, name)
        exec_commands(script)

def make_save_script(pyfile, filename):
    '''Convert a file into a script that saves that file
    into the persistent script area on the microbit'''
    output = [
        '''
import file
fd = open("%s", "w")
f = fd.write''' % filename
    ]
    while pyfile:
        line = pyfile[:64]
        output.append('f(' + repr(line) + ')')
        pyfile = pyfile[64:]
    output.append('fd.close()')
    return output

if __name__ == "__main__":
    main()
