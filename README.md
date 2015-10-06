# xmltoMySQL
一个将xml文件导入MySQL数据库的c++小程序.
* 支持导入本地文件和远程文件
* 支持Windows和Linux环境编译

程序对导入的格式有一定要求，必须是：
```
<列表头>
<行>
<字段></字段>
<字段></字段>
...
<字段></字段>
</行>
</列表头>
```
如：将sample.xml文件导入数据库test的表test中：
```
<?xml version="1.0" encoding="UTF-8"?>
<results>
    <result>
        <id>1024</id>
        <name>max</name>
        <type>Number</type>
        <value>100</value>
    </result>
    <result>
        <id>1025</id>
        <name>desc</name>
        <type>String</type>
        <value>Tom</value>
    </result>
</results>
```

导入数据库后如下：
```
mysql> select * from test;
+------+------+--------+-------+
| ID   | Name | Type   | Value |
+------+------+--------+-------+
| 1024 | max  | Number | 100   |
| 1025 | desc | String | Tom   |
+------+------+--------+-------+
2 rows in set (0.00 sec)
```

##一. 编译环境
1.安装mysqlclient库
	该程序支持Linux环境和MinGW编译环境
	
* Linux下需要安装库mysqlclient
		`sudo apt-get install libmysqlclient-dev`
* Windows 需要安装mysqlclient

2.在makefile中指定mysql头文件和动态库的位置
如：
```makefile
ifeq ($(strip $(MINGW)),MINGW)
	MYSQLCPPFLAGS=-I"C:\Program Files (x86)\MySQL\MySQL Connector C 6.1\include"
	MYSQLLDFLAGS=-L"C:\Program Files (x86)\MySQL\MySQL Connector C 6.1\lib" -llibmysql
else
	MYSQLLDFLAGS=-lmysqlclient	
	MYSQLCPPFLAGS=-I/usr/include/mysql
endif 
```
如果是MINGW环境在第一个分支修改对应的路径，如果是Linux在第二个分支修改路径。Linux下对应的动态库为libmysqlclient.so，而在windows下为libmysql.dll。

3.执行make命令即可生成程序xmltoMySQL或者xmltoMySQL.exe.

##二. 使用
1.首先要配置好配置文件，默认的配置文件名为xmltoMySQL.ini.
以开始的例子来说明，其配置文件如下
```
[DB]
#数据库所在主机名或者IP地址
hostname=localhost
#登陆数据库所用的用户名
username=root
#密码
password=root
#数据库名
database=test
#表名
tblname=test
#数据库服务的端口号
dbport=3306

[TABLE]
#xml的节点和数据库的列映射信息，每个column代表一个字段，
#包括该字段在xml的节点名，该字段在数据库中的字段名，以及字段的类型
#分别用逗号隔开,不主动过滤空格
column=id,ID,long
column=name,Name,string
column=type,Type,string
column=value,Value,string

#xml的根节点，默认为results
#rootelementname=results
```

【DB】段用于配置连接mysql数据库相关的选项，包括要导入的数据库名和数据表的名字。
【TABLE】段要指定要导入的xml和mysql数据库表各个字段之间的映射关系。每个column代表一个字段的映射。格式为
```
column=xml的节点名，数据库中的字段名，数据库中字段的类型
```
**程序只会导入配置文件中指定的字段，没有指定的不会自动导入.**

`rootelementname`为要导入的根节点名，默认为“results”,也可以根据实际的xml格式修改。

2.执行程序。
	配置好配置文件后就可以执行程序了。用法为
```
 ./xmltoMySQL [-h] [-c cofig_file] { [-f xml_file] | [-u url_file] }
```
 各个参数选项含义如下：
 
 * **-h** 显示帮助信息
 -  **-c** 指定配置文件路径
 -  **-f** 指定要导入的本地xml的文件路径，后面可以跟多个本地文件路径
 -  **-u** 指定远程xml的url路径，后面可以跟多个url路径

使用方式如下：
```
./xmltoMySQL -f sample.xml 
./xmltoMySQL -u "https://sample.com"
./xmltoMySQL -c myconfig.ini -f sample1.xml sample2.xml -u "https://sample.com"
```

**注意事项**：

* 目前远程xml文件使用wget进行下载。如果在Windows下使用该程序导入远程文件，需要安装windows版的wget，并添加其路径到系统变量PATH中去。
* Windows版程序运行时如果提示找不到libmysql.dll时，需要将其拷贝到xmltoMySQL所在的同一个目录下。
