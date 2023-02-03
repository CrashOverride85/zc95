from tkinter.ttk import *
from tkinter import *

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
