#!/usr/bin/python
#
# Server that will accept connections from a Vim channel.
# Used by test_channel.vim.
#
# This requires Python 2.6 or later.

from __future__ import print_function
import json
import socket
import sys
import time
import threading

try:
    # Python 3
    import socketserver
except ImportError:
    # Python 2
    import SocketServer as socketserver

class ThreadedTCPRequestHandler(socketserver.BaseRequestHandler):

    def handle(self):
        print("=== socket opened ===")
        while True:
            try:
                received = self.request.recv(4096).decode('utf-8')
            except socket.error:
                print("=== socket error ===")
                break
            except IOError:
                print("=== socket closed ===")
                break
            if received == '':
                print("=== socket closed ===")
                break
            print("received: {}".format(received))

            # We may receive two messages at once. Take the part up to the
            # matching "]" (recognized by finding "][").
            todo = received
            while todo != '':
                splitidx = todo.find('][')
                if splitidx < 0:
                     used = todo
                     todo = ''
                else:
                     used = todo[:splitidx + 1]
                     todo = todo[splitidx + 1:]
                if used != received:
                    print("using: {}".format(used))

                try:
                    decoded = json.loads(used)
                except ValueError:
                    print("json decoding failed")
                    decoded = [-1, '']

                # Send a response if the sequence number is positive.
                if decoded[0] >= 0:
                    if decoded[1] == 'hello!':
                        # simply send back a string
                        response = "got it"
                    elif decoded[1].startswith("echo "):
                        # send back the argument
                        response = decoded[1][5:]
                    elif decoded[1] == 'make change':
                        # Send two ex commands at the same time, before
                        # replying to the request.
                        cmd = '["ex","call append(\\"$\\",\\"added1\\")"]'
                        cmd += '["ex","call append(\\"$\\",\\"added2\\")"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'do normal':
                        # Send a normal command.
                        cmd = '["normal","G$s more\u001b"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-works':
                        # Send an eval request.  We ignore the response.
                        cmd = '["expr","\\"foo\\" . 123", -1]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-fails':
                        # Send an eval request that will fail.
                        cmd = '["expr","xxx", -2]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-error':
                        # Send an eval request that works but the result can't
                        # be encoded.
                        cmd = '["expr","function(\\"tr\\")", -3]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-bad':
                        # Send an eval request missing the third argument.
                        cmd = '["expr","xxx"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'an expr':
                        # Send an expr request.
                        cmd = '["expr","setline(\\"$\\", [\\"one\\",\\"two\\",\\"three\\"])"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'call-func':
                        cmd = '["call","MyFunction",[1,2,3], 0]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'redraw':
                        cmd = '["redraw",""]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'redraw!':
                        cmd = '["redraw","force"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'empty-request':
                        cmd = '[]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-result':
                        # Send back the last received eval result.
                        response = last_eval
                    elif decoded[1] == 'call me':
                        cmd = '[0,"we called you"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'call me again':
                        cmd = '[0,"we did call you"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = ""
                    elif decoded[1] == 'send zero':
                        cmd = '[0,"zero index"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "sent zero"
                    elif decoded[1] == 'close me':
                        print("closing")
                        self.request.close()
                        response = ""
                    elif decoded[1] == 'wait a bit':
                        time.sleep(0.2)
                        response = "waited"
                    elif decoded[1] == '!quit!':
                        # we're done
                        self.server.shutdown()
                        return
                    elif decoded[1] == '!crash!':
                        # Crash!
                        42 / 0
                    else:
                        response = "what?"

                    if response == "":
                        print("no response")
                    else:
                        encoded = json.dumps([decoded[0], response])
                        print("sending: {}".format(encoded))
                        self.request.sendall(encoded.encode('utf-8'))

                # Negative numbers are used for "eval" responses.
                elif decoded[0] < 0:
                    last_eval = decoded

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

def writePortInFile(port):
    # Write the port number in Xportnr, so that the test knows it.
    f = open("Xportnr", "w")
    f.write("{}".format(port))
    f.close()

if __name__ == "__main__":
    HOST, PORT = "localhost", 0

    # Wait half a second before opening the port to test waittime in ch_open().
    # We do want to get the port number, get that first.  We cannot open the
    # socket, guess a port is free.
    if len(sys.argv) >= 2 and sys.argv[1] == 'delay':
        PORT = 13684
        writePortInFile(PORT)

        print("Wait for it...")
        time.sleep(0.5)

    server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
    ip, port = server.server_address

    # Start a thread with the server.  That thread will then start a new thread
    # for each connection.
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.start()

    writePortInFile(port)

    print("Listening on port {}".format(port))

    # Main thread terminates, but the server continues running
    # until server.shutdown() is called.
    try:
        while server_thread.isAlive(): 
            server_thread.join(1)
    except (KeyboardInterrupt, SystemExit):
        server.shutdown()
