#!/usr/bin/python
#
# Server that will communicate with Vim through the netbeans interface.
# Used by test_netbeans.vim.
#
# This requires Python 2.6 or later.

from __future__ import print_function
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
            print("received: {0}".format(received))

            # Write the received line into the file, so that the test can check
            # what happened.
            with open("Xnetbeans", "a") as myfile:
                myfile.write(received)

            response = ''
            if received.find('README.txt') > 0:
                name = received.split('"')[1]
                response = '5:putBufferNumber!33 "' + name + '"\n'
                response += '5:setDot!1 3/19\n'
            elif received.find('disconnect') > 0:
                # we're done
                self.server.shutdown()
                return

            if len(response) > 0:
                self.request.sendall(response.encode('utf-8'))
                # Write the respoinse into the file, so that the test can knows
                # the command was sent.
                with open("Xnetbeans", "a") as myfile:
                    myfile.write('send: ' + response)

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

def writePortInFile(port):
    # Write the port number in Xportnr, so that the test knows it.
    f = open("Xportnr", "w")
    f.write("{0}".format(port))
    f.close()

if __name__ == "__main__":
    HOST, PORT = "localhost", 0

    server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
    ip, port = server.server_address

    # Start a thread with the server.  That thread will then start a new thread
    # for each connection.
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.start()

    writePortInFile(port)

    print("Listening on port {0}".format(port))

    # Main thread terminates, but the server continues running
    # until server.shutdown() is called.
    try:
        while server_thread.isAlive(): 
            server_thread.join(1)
    except (KeyboardInterrupt, SystemExit):
        server.shutdown()
