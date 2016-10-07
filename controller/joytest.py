import curses, time
from sixaxis import Joystick
from dummysixaxis import DummyJoystick

dry_run = True

js = DummyJoystick() if dry_run else Joystick()

def main(stdscr):
    begin_x = 20; begin_y = 7
    height = 15; width = 40
    win = curses.newwin(height, width, begin_y, begin_x)

    button_x = 8
    button_y = 0
    x_axis_x = 8
    x_axis_y = 2
    y_axis_x = 8
    y_axis_y = 3
    stat_y = 5

    win.addstr(button_y, 0, "BUTTON")
    win.addstr(x_axis_y, 0, "X")
    win.addstr(y_axis_y, 0, "Y")

    start_time = time.time()
    total_events = 0
    
    while True:
        win.refresh()
        event = js.get_event()
        if event and event.is_valid():
            now = time.time()
            total_events = total_events + 1
            win.addstr(stat_y, 0, "%d events, avg %f" % (total_events, (now-start_time)/total_events))
            
            is_button, button_name, pressed = event.get_button()
            if is_button:
                win.addstr(button_y, button_x, "%s %s" % (button_name, "PRESSED " if pressed else "RELEASED"))

            is_axis, axis_name, value = event.get_axis()
            if is_axis:
                if axis_name == 'x':
                    win.addstr(x_axis_y, x_axis_x, "%5d " % value)
                else:
                    win.addstr(y_axis_y, y_axis_x, "%5d " % value)


    
curses.wrapper(main)
