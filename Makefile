bin=HttpdServer
cc=g++
LDFLAGS=-lpthread

.PHONY:HttpServer
$(bin):HttpdServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) -std=c++11
.PHONY:clead
clean:
	rm -f $(bin)


