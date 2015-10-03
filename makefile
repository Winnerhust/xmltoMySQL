CC=g++ -Wall -g

APP=xmltoMySQL

all:${APP}

tinyxml2.o: tinyxml2/tinyxml2.cpp
	${CC} tinyxml2/tinyxml2.cpp -c
inifile.o: inifile2/inifile.cpp
	${CC} inifile2/inifile.cpp -c
mysqlhelper.o:mysqlhelper.cpp
	${CC} mysqlhelper.cpp -c
${APP}:inifile.o tinyxml2.o mysqlhelper.o
	${CC} main.cpp -o ${APP} inifile.o tinyxml2.o mysqlhelper.o -lmysqlclient 

clean:
	rm -rf *.o ${APP}
