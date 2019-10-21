%.o:%.cxx
	$(CXX) -I../../libdsp -I../../libsimpleFE/ -c $? -O3
bpsk: bpsk.o
	$(CXX) -o $@ $< -L../../libdsp/build -lLibdsp -L../../libsimpleFE/build -lsimpleFE -lpthread -lm -ludev -lusb-1.0 -lfftw3f

clean:
	rm *.o
	rm bpsk
