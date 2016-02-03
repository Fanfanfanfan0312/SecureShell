SOURCE = ../src/mainserver.cpp ../src/util.cpp ../src/terminal.cpp
HEADER = ../src/util.h

all: SGWD

install:
	g++ -w $(SOURCE) -o SGWD -lpthread -lssl -lcrypto
	cp ./SGWD /usr/bin
	mkdir /var/SGWD
	chmod 777 /var/SGWD

SGWD: $(SOURCE) $(HEADER)
	g++ -w $(SOURCE) -o SGWD -lpthread -lssl -lcrypto

clean:
	rm SGWD
