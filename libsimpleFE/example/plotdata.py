
import matplotlib.pyplot as plot
import sys
import struct

data = None
chunk = 0;

def on_key_press(event):
    global chunk
    event.canvas.figure.clear()
    y = struct.unpack("@8192B", data[chunk*8192:(chunk+1)*8192]);
    yi = y[0::2]
    yq = y[1::2]
    event.canvas.figure.gca().plot(yi)
    event.canvas.draw()
    chunk = chunk + 1;


file = open(sys.argv[1], mode="rb")
    
data = file.read()
fig = plot.figure()
fig.canvas.mpl_connect("key_press_event", on_key_press)

plot.show()

file.close()
    
    
    
