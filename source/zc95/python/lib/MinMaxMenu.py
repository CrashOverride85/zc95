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
