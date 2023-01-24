import websocket # pip3 install websocket-client
import threading

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
    
    self.__connection_wait_event = threading.Event()
  
  
  def __on_message(self, ws, message):
    if self.__recv_waiting:
      self.__pending_recv_message = message
      self.__recv_event.set()
      
    else:    
      print(message)

  def __on_error(self, ws, error):
    print(error)

  def __on_close(self, ws, close_status_code, close_msg):
    print("### closed ###")


  def __on_open(self, ws):
    print("Connection opened")
    self.__connection_wait_event.set()
    
  def wait_for_connection(self):
    self.__connection_wait_event.wait()
    
  def send(self, message):
    self.ws.send(message)
    
  def recv(self):
    self.__recv_waiting = True
    
    if self.__recv_event.wait(timeout=2): # timeout is seconds
      retval = self.__pending_recv_message
    else:
      retval = None
    
    self.__recv_waiting = False
    self.__recv_event.clear()
    return retval
  
  def run_forever(self):
    self.ws.run_forever(ping_interval=6)
    
  def stop(self):
    self.ws.close()

if __name__ == "__main__":
  zs = ZcWs("ws://192.168.1.136/stream")

  zs.ws.run_forever()
