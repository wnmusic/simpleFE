default: t_top

VFLAGS = -I. -D_IVERILOG_SIM
all:  t_frontend t_spi t_top t_loopback

t_max5863: t_max5863.v max5863_if.v
	iverilog $(VFLAGS) -o $@ $^

t_frontend: t_frontend.v frontend.v
	iverilog $(VFLAGS) -o $@ $^

t_spi: t_spi.v spi_if.v
	iverilog $(VFLAGS) -o $@ $^

t_ezusb: t_ezusb.v ezusb_if.v
	iverilog $(VFLAGS) -o $@ $^

t_top: t_top.v top.v ezusb_if.v spi_if.v fifo.v frontend.v
	iverilog $(VFLAGS) -o $@ $^

clean:
	rm t_ezusb t_max5863 t_spi t_top
