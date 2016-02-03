SOURCE = ../src/clientmain.cpp ../src/receive.cpp ../src/util.cpp ../src/password.cpp
HEADER = ../src/util.h ../src/password.h

all: SGW

install:
	g++ -w $(SOURCE) -o SGW -lpthread -lssl -lcrypto
	cp ./SGW /usr/bin

SGW: $(SOURCE) $(HEADER)
	g++ -w $(SOURCE) -o SGW -lpthread -lssl -lcrypto

clean:
	rm SGW
