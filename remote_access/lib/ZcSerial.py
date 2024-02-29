import time
from serial import Serial
from serial.threaded import ReaderThread, Protocol, LineReader
import threading
import json 
import sys
import queue

class SerialReader(Protocol):
    on_receive = None
    on_open    = None
    on_close   = None

    def __init__(self):
      self.__state = 'IDLE'
      self.__message = bytearray() 

    def connection_made(self, transport):
      self.on_open()
      
    def connection_lost(self, exc):
      self.on_close()

    def data_received(self, data):
      for byte in data:
        if self.__state == "IDLE":
          if byte == 0x02: # STX
            self.__state = "RECV"
            
        elif self.__state == "RECV":
          if byte == 0x03: # ETX
            message_string = self.__message.decode("utf-8")
            
            self.on_receive(message_string)
            
            # reset for next message
            self.__message = bytearray()
            self.__state = "IDLE"
            
          else:
            self.__message.append(byte)


class ZcSerial:
  def __init__(self, serial_port, rcv_queue, debug):
    print("Opening: " + serial_port)
    self.serial = Serial(serial_port, baudrate=115200, timeout=0)
    
    serial_reader = SerialReader
    serial_reader.on_receive = self.__on_message
    serial_reader.on_open  = self.__on_open
    serial_reader.on_close = self.__on_close
    
    self.__recv_waiting = False
    self.__pending_recv_message = ""
    self.__recv_event = threading.Event()
    self.__waiting_for_msgId = 0
    self.__connection_wait_event = threading.Event()
    self.__rcv_queue = rcv_queue
    self.debug = debug
    
    self.__reader = ReaderThread(self.serial, serial_reader)
    self.__reader.start()
  
  def __on_message(self, message):
    if self.debug:
      print("< " + message)
      
    result = json.loads(message)
    if self.__recv_waiting and "MsgId" in result and result["MsgId"] == self.__waiting_for_msgId:
        self.__pending_recv_message = message
        self.__recv_event.set()
    else:
      self.__rcv_queue.put(result)

  def __on_close(self):
    print("Connection closed")

  def __on_open(self):
    print("Connection opened")
    self.__connection_wait_event.set()
    
  def wait_for_connection(self):
    self.__connection_wait_event.wait()
    
  def send(self, message):
    if self.debug:
      print("> " + message)

    self.serial.write(b'\x02')     # STX
    self.serial.write(message.encode('utf-8'))
    self.serial.write(b'\x03')     # ETX
    
  # note: not at all thread safe
  def recv(self, msgId):
    self.__waiting_for_msgId = msgId
    self.__recv_waiting = True
    
    if self.__recv_event.wait(timeout=6): # timeout is seconds
      retval = self.__pending_recv_message
    else:
      retval = None
    
    self.__recv_waiting = False
    self.__recv_event.clear()
    self.__waiting_for_msgId = 0
    
    return retval
  
  def run_forever(self):
    pass # might do some sort of ping here one day to check connection is still good

  def stop(self):
    self.serial.write(b'\x04')     # EOT, causes the zc95 to stop a pattern if one is running, and reset the connection (clears any state, etc.)
    self.__reader.stop()
    self.serial.close()
