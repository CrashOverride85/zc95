from tkinter.ttk import *
from tkinter import *
from tkinter import messagebox
import argparse
import time
import threading
import websocket
import lib.ZcMessages as zc
import queue
from lib.ZcWs import ZcWs
from lib.ZcSerial import ZcSerial
from lib.MinMaxMenu import MinMaxMenu
from lib.PowerDisplay import PowerDisplay

class ZcPatternGui:
  def __init__(self, root, pattern_config, zc_patterns, rcv_queue, debug):
    self.root = root
    self.pattern_config = pattern_config
    self.zc_patterns = zc_patterns
    self.rcv_queue = rcv_queue
    self.debug = debug 
    root.title("ZC95")
    root.config(bg="red")
    root.resizable(False,False)
    self.InitDisplay(self.root)

  def AddMenuOptions(self, pattern_options_frame, row, menu_item):
    menu_type = menu_item["Type"]

    if menu_type == "MIN_MAX":
      min_max_frame = Frame(pattern_options_frame, width=200, height=400)
      min_max_frame.grid(row=row, column=1, padx=10, pady=5)

      progress_bar = Progressbar(min_max_frame, orient='horizontal', mode='determinate', length=200, maximum=menu_item["Max"] - menu_item["Min"])      
      progress_bar.grid(row=0, column=0, columnspan=3, padx=5, pady=5)

      current_val_var = StringVar()
      Label(min_max_frame, text=menu_item["Min"]    ).grid(row=1, column=0, padx=5, pady=5, sticky=W)
      Label(min_max_frame, textvariable = current_val_var).grid(row=1, column=1, padx=5, pady=5)
      Label(min_max_frame, text=menu_item["Max"]    ).grid(row=1, column=2, padx=5, pady=5, sticky=E)
      
      Button(min_max_frame, text="-", command=lambda: self.MinMaxButtonPushed(menu_item["Id"], "-")).grid(row=2, column=0, padx=5, pady=5, sticky=W)
      Button(min_max_frame, text="+", command=lambda: self.MinMaxButtonPushed(menu_item["Id"], "+")).grid(row=2, column=2, padx=5, pady=5, sticky=E)

      self.min_max_menus[menu_item["Id"]] = MinMaxMenu(menu_item["Min"], menu_item["Max"], menu_item["IncrementStep"], menu_item["Default"], progress_bar, current_val_var)
    
    if menu_type == "MULTI_CHOICE":
      radio_button_frame = Frame(pattern_options_frame, width=200, height=400)
      radio_button_frame.grid(row=row, column=1, padx=10, pady=5)    
      
      self.var_radio_buttons[menu_item["Id"]] = IntVar(radio_button_frame)
      self.var_radio_buttons[menu_item["Id"]].set(menu_item["Default"])
      choice_count = 0
      for choice in menu_item["Choices"]:
        Radiobutton(radio_button_frame, text=choice["Name"], variable=self.var_radio_buttons[menu_item["Id"]], value=int(choice["Id"]), command=lambda: self.MultiChoiceChanged(menu_item["Id"])).grid(row=choice_count, column=0, padx=5, pady=5)
        choice_count += 1
      
  def MinMaxButtonPushed(self, menu_id, direction):
    # If the +/- buttons are clicked, increment the progress bar in step_size steps 
    # in the appropriate direction, up to a limit of min_val/max_val
    
    if direction == "+":
      self.min_max_menus[menu_id].increment()
    else:
      self.min_max_menus[menu_id].decrement()
      
    self.zc_patterns.PatternMinMaxChange(menu_id, self.min_max_menus[menu_id].current_val)
      
  def MultiChoiceChanged(self, menu_id):
    choice_id = self.var_radio_buttons[menu_id].get()
    self.zc_patterns.PatternMultiChoiceChange(menu_id, choice_id)


  def InitDisplay(self, root):
    self.PowerDisplays = {}
    
    self.DrawPatternFrame(root, row=0, col=0)
      
    # 4 channel power graphs + sliders
    for channel in range(1, 5):
      self.PowerDisplays[channel] = PowerDisplay(self.root, channel)
      self.PowerDisplays[channel].draw(row=0, col=channel)
      
    if self.debug:
      self.debug_text = Text(root, height=10)
      self.debug_text.grid(row=1, column=0, columnspan=5, sticky=EW)
      self.debug_text.tag_configure('errorline', background='yellow', font='TkFixedFont', relief='raised')
      self.debug_text['state'] = 'disabled'

  def DrawPatternFrame(self, root, row, col):
    pattern_frame = Frame(root, width=400, height=400)
    pattern_frame.grid(row=0, column=0, padx=10, pady=5)
    Label(pattern_frame, text=pattern["Name"], font='Helvetica 18 bold').grid(row=0, column=0, padx=5, pady=5)
  
    pattern_options_frame = Frame(pattern_frame, width=400, height=400)
    pattern_options_frame.grid(row=1, column=0, padx=10, pady=5)

    Label (pattern_options_frame, text="Soft button").grid(row=0, column=0, padx=5, pady=5, sticky=W)
    soft_button = Button(pattern_options_frame, text=pattern["ButtonA"])
    soft_button.grid(row=0, column=1, padx=5, pady=5)    
    soft_button.bind("<ButtonPress>", self.SoftButtonPressed)
    soft_button.bind("<ButtonRelease>", self.SoftButtonReleased)

    row = 1
    self.var_radio_buttons = {}
    self.progress = {}
    self.min_max_menus = {}
    for menu_item in pattern["MenuItems"]:
      spacer_frame = Frame(pattern_options_frame, width=400, height=4, highlightbackground="blue", highlightthickness=2)
      spacer_frame.grid(row=row, column=0, columnspan=2, padx=10, pady=5, sticky="ew")
      row += 1
      
      title = menu_item["Title"] 
      if "UoM" in menu_item:
        if len(menu_item["UoM"]) > 0:
          title += " (" + menu_item["UoM"] + ")"
      
      Label(pattern_options_frame, text=title).grid(row=row, column=0, padx=5, pady=5, sticky=W)
      self.AddMenuOptions(pattern_options_frame, row, menu_item)
      row += 1
      
  # Stolen from https://tkdocs.com/tutorial/text.html
  def WriteToLog(self, msg, error):
    
    # If we're not in debug mode, there's no debug output panel at the bottom. If we get an error,
    # show it in a popup, as it almost certainly means we were running a lua script that died
    if not self.debug:
      if error:
        messagebox.showerror('Script error', msg)
      return

    numlines = int(self.debug_text.index('end - 1 line').split('.')[0])
    self.debug_text['state'] = 'normal'
    if numlines==9:
        self.debug_text.delete(1.0, 2.0)
    if self.debug_text.index('end-1c')!='1.0':
        self.debug_text.insert('end', '\n')
    
    if error:
      self.debug_text.insert('end', msg, 'errorline')
    else:
      self.debug_text.insert('end', msg)
    
    self.debug_text['state'] = 'disabled'
  
  def SoftButtonPressed(self, event):
    self.zc_patterns.PatternSoftButton(1)
    
  def SoftButtonReleased(self, event):
    self.zc_patterns.PatternSoftButton(0)
    
  def ProcessPowerStatusMessage(self, message):
    for channel in message["Channels"]:
      channel_number = channel["Channel"]
      self.PowerDisplays[channel_number].set_actual_power_level(channel["OutputPower"])
      self.PowerDisplays[channel_number].set_max_power_level(channel["MaxOutputPower"])
      self.PowerDisplays[channel_number].set_power_limit(channel["PowerLimit"])
      
      self.PowerDisplays[channel_number].update_display_if_required()

  def ProcessLuaScriptOutputMessage(self, message):
    if message["TextType"] == "Print":
      self.WriteToLog(message["Text"], False)
    elif message["TextType"] == "Error":  
      self.WriteToLog(message["Text"], True)
    

  # When the power sliders are changed, send a message with the new value
  # Send at most one message every 250ms
  def TaskUpdatePowerLevel(self):
    update_message_required = False
    for channel in range(1, 5):
      if self.PowerDisplays[channel].HasSetPowerLevelChangedSinceLastCheck():
        update_message_required = True

    if update_message_required:
      self.zc_patterns.SendSetPowerMessage(self.PowerDisplays[1].GetSetPowerLevel(), 
                                           self.PowerDisplays[2].GetSetPowerLevel(),
                                           self.PowerDisplays[3].GetSetPowerLevel(),
                                           self.PowerDisplays[4].GetSetPowerLevel())   
    
    self.root.after(250, self.TaskUpdatePowerLevel)

  def TaskProcessWsRecvQueue(self):
    if not self.rcv_queue.empty():
      message = self.rcv_queue.get_nowait()
      
      if "Type" in message:
        if message["Type"] == "PowerStatus":
          self.ProcessPowerStatusMessage(message)
        elif message["Type"] == "LuaScriptOutput":
          self.ProcessLuaScriptOutputMessage(message)

    self.root.after(20, self.TaskProcessWsRecvQueue)
    
    

parser = argparse.ArgumentParser(description='Start and run pattern on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')

connection_group = parser.add_mutually_exclusive_group(required=True)
connection_group.add_argument('--ip', action='store', help='IP address of ZC95')
connection_group.add_argument('--serial', action='store', help='Serial port to use')

parser.add_argument('--index', action='store', required=True, help='Start pattern at specified index')

args = parser.parse_args()

# this is a bit much, even for debug
#if args.debug:
#  websocket.enableTrace(True)

rcv_queue = queue.Queue() # to allow received messages to be sent to the GUI

# Connect either using serial or websocket
if args.serial:
  zc_connection = ZcSerial(args.serial, rcv_queue, args.debug)
else:
  zc_connection = ZcWs(args.ip, rcv_queue, args.debug)

conn_thread = threading.Thread(target=zc_connection.run_forever)
conn_thread.start()

zc_connection.wait_for_connection()

zc_messages = zc.ZcMessages(zc_connection, args.debug)
pattern = zc_messages.GetPatternDetails(args.index)

root = Tk() 
gui = ZcPatternGui(root, pattern, zc_messages, rcv_queue, args.debug)

root.after(250, gui.TaskUpdatePowerLevel)
root.after( 20, gui.TaskProcessWsRecvQueue)
zc_messages.PatternStart(args.index)


root.mainloop()
zc_connection.stop()
