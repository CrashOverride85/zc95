_delay_ms = 500

Config = {
    name = "AccIoFlash",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Delay",
            id = 1,
            min = 100,
            max = 2000,
            increment_step = 100,
            uom = "ms",
            default = _delay_ms
         }
    }
}

_active_io_line = 1
_wait_until_ms = 0

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _delay_ms = min_max_val
    end
end

function Loop(time_ms)
    if (time_ms > _wait_until_ms)
    then
        NextIoLine();
        _wait_until_ms = time_ms + _delay_ms
    end
end

function NextIoLine()
  if (_active_io_line == 1)
  then
    zc.AccIoWrite(1, false)
    zc.AccIoWrite(2, true)
    _active_io_line = 2
  elseif (_active_io_line == 2)
  then
    zc.AccIoWrite(2, false)
    zc.AccIoWrite(3, true)
    _active_io_line = 3
  else
    zc.AccIoWrite(3, false)
    zc.AccIoWrite(1, true)
    _active_io_line = 1
  end
end
