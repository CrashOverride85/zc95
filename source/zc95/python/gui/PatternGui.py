from tkinter.ttk import *
from tkinter import *

import websocket
import patterns as zc

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


class ZcPatternGui:
  def __init__(self, root, pattern_config):
    self.root = root
    self.pattern_config = pattern_config
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
        Radiobutton(radio_button_frame, text=choice["Name"], variable=self.var_radio_buttons[menu_item["Id"]], value=int(choice["Id"])).grid(row=choice_count, column=0, padx=5, pady=5)
        choice_count += 1
      
  def MinMaxButtonPushed(self, menu_id, direction):
    # If the +/- buttons are clicked, increment the progress bar in step_size steps 
    # in the appropriate direction, up to a limit of min_val/max_val
    
    if direction == "+":
      self.min_max_menus[menu_id].increment()
    else:
      self.min_max_menus[menu_id].decrement()
      

  def InitDisplay(self, root):
    pattern_frame = Frame(root, width=400, height=400)
    pattern_frame.grid(row=0, column=0, padx=10, pady=5)
    Label(pattern_frame, text=pattern["Name"], font='Helvetica 18 bold').grid(row=0, column=0, padx=5, pady=5)

    pattern_options_frame = Frame(pattern_frame, width=400, height=400)
    pattern_options_frame.grid(row=1, column=0, padx=10, pady=5)

    Label (pattern_options_frame, text="Soft button").grid(row=0, column=0, padx=5, pady=5, sticky=W)
    Button(pattern_options_frame, text=pattern["ButtonA"]).grid(row=0, column=1, padx=5, pady=5)

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


ws = websocket.WebSocket()
print("Connecting")
ws.connect("ws://192.168.1.136/stream")
patterns = zc.ZcPatterns(ws)

pattern = patterns.GetPatternDetails(14)

root = Tk() 
gui = ZcPatternGui(root, pattern)

root.mainloop()


