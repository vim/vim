#!/usr/bin/python
#
# Server that will accept connections from a Vim channel.
# Run this server and then in Vim you can open the channel:
#  :let handle = ch_open('localhost:8765', 'json')
#
# Then Vim can send requests to the server:
#  :let response = ch_sendexpr(handle, 'hello!')
#
# See ":help channel-demo" in Vim.
#
# This requires Python 2.6 or later.

from __future__ import print_function
import json
import socket
import sys
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
                    elif decoded[1] == 'make change':
                        # Send two ex commands at the same time, before
                        # replying to the request.
                        cmd = '["ex","call append(\\"$\\",\\"added1\\")"]'
                        cmd += '["ex","call append(\\"$\\",\\"added2\\")"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-works':
                        # Send an eval request.  We ignore the response.
                        cmd = '["eval","\\"foo\\" . 123", -1]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-fails':
                        # Send an eval request that will fail.
                        cmd = '["eval","xxx", -2]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-bad':
                        # Send an eval request missing the third argument.
                        cmd = '["eval","xxx"]'
                        print("sending: {}".format(cmd))
                        self.request.sendall(cmd.encode('utf-8'))
                        response = "ok"
                    elif decoded[1] == 'eval-result':
                        # Send back the last received eval result.
                        response = last_eval
                    elif decoded[1] == '!quit!':
                        # we're done
                        sys.exit(0)
                    elif decoded[1] == '!crash!':
                        # Crash!
                        42 / 0
                    else:
                        response = "what?"

                    encoded = json.dumps([decoded[0], response])
                    print("sending: {}".format(encoded))
                    self.request.sendall(encoded.encode('utf-8'))

                # Negative numbers are used for "eval" responses.
                elif decoded[0] < 0:
                    last_eval = decoded

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

if __name__ == "__main__":
    HOST, PORT = "localhost", 0

    server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
    ip, port = server.server_address

    # Start a thread with the server -- that thread will then start one
    # more thread for each request
    server_thread = threading.Thread(target=server.serve_forever)

    # Exit the server thread when the main thread terminates
    server_thread.daemon = True
    server_thread.start()

    # Write the port number in Xportnr, so that the test knows it.
    f = open("Xportnr", "w")
    f.write("{}".format(port))
    f.close()

    # Block here
    print("Listening on port {}".format(port))
    server.serve_forever()
