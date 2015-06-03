all:
	make router
	make client

router:
	g++ -o udprouter Router.cpp

client:
	g++ -o client Client.cpp

clean:
	rm client udprouter
