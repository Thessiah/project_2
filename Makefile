all:
	make router
	make client

router:
	g++ -o udprouter my-router.cpp -lpthread

client:
	g++ -o client Client.cpp

clean:
	rm client udprouter
