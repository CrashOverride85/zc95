import websocket # pip3 install websocket-client
import threading
import json 

class ZcWs:
  def __init__(self, connection_string):
    self.ws = websocket.WebSocketApp(connection_string,
                              on_open    = self.__on_open,
                              on_message = self.__on_message,
                              on_error   = self.__on_error,
                              on_close   = self.__on_close)
    
    self.__recv_waiting = False
    self.__pending_recv_message = ""
    self.__recv_event = threading.Event()
    self.__waiting_for_msgId = 0
    self.__connection_wait_event = threading.Event()
  
  
  def __on_message(self, ws, message):
    if self.__recv_waiting:
      result = json.loads(message)
      if "MsgCount" in result and result["MsgCount"] == self.__waiting_for_msgId:
        self.__pending_recv_message = message
        self.__recv_event.set()
      
    else:    
      print("< " + message)

  def __on_error(self, ws, error):
    if str(error) != "None":
      print("Websocket error: " + str(error))

  def __on_close(self, ws, close_status_code, close_msg):
    print("Websocket connection closed")
    quit()

  def __on_open(self, ws):
    print("Connection opened")
    self.__connection_wait_event.set()
    
  def wait_for_connection(self):
    self.__connection_wait_event.wait()
    
  def send(self, message):
    self.ws.send(message)
    
  # note: not at all thread safe
  def recv(self, msgId):
    self.__waiting_for_msgId = msgId
    self.__recv_waiting = True
    
    if self.__recv_event.wait(timeout=2): # timeout is seconds
      retval = self.__pending_recv_message
    else:
      retval = None
    
    self.__recv_waiting = False
    self.__recv_event.clear()
    self.__waiting_for_msgId = 0
    return retval
  
  def run_forever(self):
    self.ws.run_forever(ping_interval=6)
    
  def stop(self):
    self.ws.close()
