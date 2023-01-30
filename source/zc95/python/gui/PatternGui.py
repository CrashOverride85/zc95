from tkinter.ttk import *
from tkinter import *
import argparse
import time
import threading
import websocket
import patterns as zc
import ZcWs
import queue

class MinMaxMenu:
  def __init__(self, min_val, max_val, step_size, current_val, progress_bar, current_val_var):
    self.min_val = min_val
    self.max_val = max_val
    self.current_val = current_val
    self.step_size = step_size
    self.progress_bar = progress_bar
    self.current_val_var = current_val_var
    self.current_val_var.set(self.current_val)
    self.update_display()

  def increment(self):
    self.current_val += self.step_size
    
    if self.current_val > self.max_val:
      self.current_val = self.max_val
      
    self.update_display()

  def decrement(self):
    self.current_val -= self.step_size
    
    if self.current_val < self.min_val:
      self.current_val = self.min_val
      
    self.update_display()
      
  def update_display(self):
    # progress bars always go from 0..max, but pattern needs min..max
    self.current_val_var.set(self.current_val)
    self.progress_bar['value'] = self.current_val - self.min_val

class PowerDisplay:
  def __init__(self, root, channel):
    self._root = root
    self._channel = channel
    
    self.bar_width  = 25
    self.bar_height = 200    
    
    self._power_level = 0
    self._actual_power_level = 0
    self._power_limit = 1000
    
    self._update_required = False
    self._label_var = StringVar()
    self._scale_var = IntVar()
    
    self._set_power_level_at_last_check = -1
    self.update_percent_label(0)
    
  def set_actual_power_level(self, level):
    if self._actual_power_level != level:
      self._actual_power_level = level
      self._update_required = True

  def set_max_power_level(self, level):
    if self._power_level != level:
      self._power_level = level
      self._update_required = True
      
      percent = 100-(((1000-self._power_level) / 1000) * 100)
      self.update_percent_label(percent)
      
  def set_power_limit(self, level):
    if self._power_limit != level:
      self._power_limit = level
      self._update_required = True
      
  def update_percent_label(self, percent):
    self._label_var.set(f'{percent:.2f}' + "%")
      
  def update_display_if_required(self):
    if self._update_required:
      self.CanvasDraw()
      self._update_required = False
    
  def draw(self, row, col):      
    power_frame = Frame(self._root, width=400, highlightbackground="blue", highlightthickness=2)
    power_frame.grid(row=row, column=col, padx=10, pady=5, sticky="ew")

    Label(power_frame, text="Channel " + str(self._channel), font='Helvetica 12 bold').grid(row=0, column=0, padx=5, pady=5)
    
    self.canvas = Canvas(power_frame, background='black', width=self.bar_width, height=self.bar_height, highlightthickness=1, highlightbackground="red")
    self.canvas.grid(row=1, column=0)
    self.CanvasDraw();   
    
    label = Label(power_frame, font='Helvetica 12 bold')
    label.grid(row=2, column=0, padx=5, pady=5)
    label['textvariable'] = self._label_var   
    
    scale = Scale(power_frame, orient='vertical', length=200, from_=1000, to=0, showvalue=0, variable=self._scale_var)
    scale.grid(row=3, column=0, padx=5, pady=5)    
    
  def CanvasDraw(self):
    self.canvas.delete("all")

    # blue bar that shows current max power setting
    bar_current_power_height = (self.bar_height / 1000) * self._power_level
    self.canvas.create_rectangle(0, self.bar_height, 25, self.bar_height - bar_current_power_height, fill='blue')
    
    # yellow bar that shows actual power (usually the same as blue, but some patterns may reduce it)
    bar_actual_power_height = (self.bar_height / 1000) * self._actual_power_level
    yellow_bar_width = 11
    yellow_bar_x0 = self.bar_width/2
    self.canvas.create_rectangle((self.bar_width/2) - (yellow_bar_width/2) , self.bar_height, (self.bar_width/2) + (yellow_bar_width/2), self.bar_height - bar_actual_power_height, fill='yellow')    
    
    # Green bar at top that marks power limit (if set)
    if self._power_limit < 1000:
      bar_power_limit_height = (self.bar_height / 1000) * (1000-self._power_limit)
      self.canvas.create_rectangle(0, 0, self.bar_width+1, bar_power_limit_height, fill='green')     

  def HasSetPowerLevelChangedSinceLastCheck(self):
    has_changed = False
    if self._set_power_level_at_last_check != self._scale_var.get():
      has_changed = True
      self._set_power_level_at_last_check = self._scale_var.get()
    
    return has_changed

  def GetSetPowerLevel(self):
    return self._scale_var.get()

class ZcPatternGui:
  def __init__(self, root, pattern_config, zc_patterns, rcv_queue):
    self.root = root
    self.pattern_config = pattern_config
    self.zc_patterns = zc_patterns
    self.rcv_queue = rcv_queue
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
    
    pattern_frame = Frame(root, width=400, height=400)
    pattern_frame.grid(row=0, column=0, padx=10, pady=5)
    Label(pattern_frame, text=pattern["Name"], font='Helvetica 18 bold').grid(row=0, column=0, padx=5, pady=5)
    
    for channel in range(1, 5):
      self.PowerDisplays[channel] = PowerDisplay(self.root, channel)
      self.PowerDisplays[channel].draw(0, channel)

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
      
      if "Type" in message and message["Type"] == "PowerStatus":
        self.ProcessPowerStatusMessage(message)

    self.root.after(20, self.TaskProcessWsRecvQueue)
    
    

parser = argparse.ArgumentParser(description='Start and run pattern on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')
parser.add_argument('--index', action='store', required=True, help='Start pattern at specified index')

args = parser.parse_args()

# this is a bit much, even for debug
#if args.debug:
#  websocket.enableTrace(True)

rcv_queue = queue.Queue() # to allow received web socket messages to be sent to the GUI

zcws = ZcWs.ZcWs("ws://" + args.ip + "/stream", rcv_queue)

ws_thread = threading.Thread(target=zcws.run_forever)
ws_thread.start()
zcws.wait_for_connection()

zc_patterns = zc.ZcPatterns(zcws, args.debug)
pattern = zc_patterns.GetPatternDetails(args.index)

root = Tk() 
gui = ZcPatternGui(root, pattern, zc_patterns, rcv_queue)

root.after(250, gui.TaskUpdatePowerLevel)
root.after( 20, gui.TaskProcessWsRecvQueue)
zc_patterns.PatternStart(args.index)


root.mainloop()
zcws.stop()

