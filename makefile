CC=g++ -Wall -g

#makefile中的shell调用格式 $(shell 命令) 
UNAME_STR=$(shell  uname)
MINGW=$(findstring MINGW,${UNAME_STR})


ifeq ($(strip $(MINGW)),MINGW)
	MYSQLCPPFLAGS=-I"C:\Program Files (x86)\MySQL\MySQL Connector C 6.1\include"
	MYSQLLDFLAGS=-L"C:\Program Files (x86)\MySQL\MySQL Connector C 6.1\lib" -llibmysql
else
	MYSQLLDFLAGS=-lmysqlclient	
	MYSQLCPPFLAGS=-I/usr/include/mysql
endif 

APP=xmltoMySQL

all:${APP}
	
tinyxml2.o: tinyxml2/tinyxml2.cpp
	${CC} tinyxml2/tinyxml2.cpp -c
inifile.o: inifile2/inifile.cpp
	${CC} inifile2/inifile.cpp -c
mysqlhelper.o:mysqlhelper.cpp
	${CC} ${MYSQLCPPFLAGS} mysqlhelper.cpp -c
${APP}:inifile.o tinyxml2.o mysqlhelper.o
	${CC} main.cpp -o ${APP} inifile.o tinyxml2.o mysqlhelper.o ${MYSQLLDFLAGS} 

clean:
	rm -rf *.o ${APP} ${APP}.exe
