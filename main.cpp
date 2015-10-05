#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "inifile2/inifile.h"
#include "tinyxml2/tinyxml2.h"
#include "mysqlhelper.h"

using namespace std;
using namespace inifile;
using namespace tinyxml2;

/* 配置文件路径名，默认为 config.ini */
#define DEFAULT_CFG_FILE "config.ini"
/* 默认的xml的根路径名 */
#define DEFAULT_ROOT_ELEMENT_NAME "results"

/* 默认的url下载的临时文件路径名 */
#define DEFAULT_DOWNLOAD_FILE_NAME "xml_file.xml.tmp"

typedef int (*fnExecuteImportSQL)(void *conn,
                                  void *st,
                                  const vector<string> &columnsType,
                                  const vector<const char *> &valueList);

/* 用于保持要导入xml的结果
 * type = 0 表示本地文件，type = 1 表示url路径
 * path为文件名或者url路径
 * */
typedef struct tagXmlFilePath {
    int  type;/* local file:0,url:1 */
    char path[512];
} XmlFilePath;

/* 用于保存全局配置
 * */
typedef struct tagConfig {
    vector<string> columnsName;
    vector<string> columnsType;
    vector<string> columnsTblName;

    string hostName;
    string userName;
    string passWord;
    string database;
    string tblName;
    string rootElementName;
    unsigned int dbPort;

    string cfgFilePath;
    vector<XmlFilePath> xmlFileList;
} Config;

/* 全局配置 */
Config g_config;

/**
 *  功能描述:打印命令行帮助信息
 *  @param argv 命令行入参
 *
 *  @return 无
 *  */
void print_usage(char *argv[])
{
    printf("Usage : [%s] [-h] [-c cofig_file] { [-f xml_file] | [-u url_file] }\n", argv[0]);
    printf("        -h Show help\n");
    printf("        -c config file path, default is %s\n", g_config.cfgFilePath.c_str());
    printf("        -f the path of local xml file\n");
    printf("        -u the url of remote xml file\n");
    printf("For example:\n");
    printf("    %s -f sample.xml \n", argv[0]);
    printf("    %s -u \"https://sample.com\"\n", argv[0]);
    printf("    %s -c config.ini -f sample1.xml sample2.xml -u \"https://sample.com\"\n", argv[0]);
}

/**
 *  功能描述:解析命令行参数
 *  @param argc 命令行参数个数
 *  @param argv 命令行入参
 *
 *  @return 无
 *  */
void get_option(int argc, char *argv[])
{
    extern char    *optarg;
    extern int      optind;
    int optch;
    int type ;
    const char    optstring[] = "hc:f:u:";
    XmlFilePath xmlFile;

    g_config.cfgFilePath = DEFAULT_CFG_FILE;

    while ((optch = getopt(argc , (char * const *)argv , optstring)) != -1) {
        switch (optch) {
            case 'h':
                print_usage(argv);
                exit(-1);
            case 'c':
                g_config.cfgFilePath = optarg;
                break;

            case 'f':
            case 'u':

                type = (optch == 'f') ? 0 : 1;
                xmlFile.type = type;
                strcpy(xmlFile.path, optarg);
                g_config.xmlFileList.push_back(xmlFile);

                /* -f 和 -u 可带多个参数
                 * 通过引用外部变量extern int optind，通过读取argv[optind++]，
                 * 并判断argv[optind++]的第一个字符是否为"-"为止 */

                while (optind < argc && argv[optind][0] != '-') {
                    xmlFile.type = type;
                    strcpy(xmlFile.path, argv[optind++]);
                    g_config.xmlFileList.push_back(xmlFile);
                }

                break;
            case '?':
            case ':':
            default:
            	print_usage(argv);
            	printf("unknown parameter: -%c\n", optopt);
                exit(-1);
        }
    }
    
	/* 没有带参数 */
    if (argc == 1){
    	print_usage(argv);
    	exit(-1);
    }

	/* 没有解析完，说明有无效的参数 */
    if (optind != argc){
    	printf("[ERROR]There is some invalid options [%s]\n",argv[optind]);
    	print_usage(argv);
    	exit(-1);
    }
}


/**
 *  功能描述:初始化配置
 *  @param cfgFileName 配置文件路径名
 *  @param config 输出参数，配置文件解析后保存到config中
 *
 *  @return 成功返回0，失败返回其他值
 *  */
int initConfig(const string &cfgFileName, Config &config)
{
    IniFile cfgFile;
    vector<string> columns;

    if (cfgFileName == "") {
        fprintf(stderr, "[ERROR]the config file name is null\n");
        return -1;
    }

    cfgFile.load(cfgFileName);
    cfgFile.getValues("TABLE", "column", columns);

    for (size_t i = 0 ; i < columns.size(); ++i) {
        size_t pos1 = columns[i].find_first_of(',');
        size_t pos2 = columns[i].find_last_of(',');

        if ((pos1 == string::npos) || (pos1 == pos2)) {
            fprintf(stderr, "[ERROR]config error,COLUMN=%s\n", columns[i].c_str());
            return -1;
        }

        config.columnsName.push_back(columns[i].substr(0, pos1));
        config.columnsTblName.push_back(columns[i].substr(pos1 + 1, pos2 - pos1 - 1));
        config.columnsType.push_back(columns[i].substr(pos2 + 1));

        printf("[INFO]cloumn:name=[%s],tblname=[%s],type=[%s]\n",
               config.columnsName[i].c_str(),
               config.columnsTblName[i].c_str(),
               config.columnsType[i].c_str());
    }

    /* 根节点默认为results */
    if (cfgFile.getValue("TABLE", "rootelementname", config.rootElementName)) {
        config.rootElementName = DEFAULT_ROOT_ELEMENT_NAME;
    }

    printf("[INFO]rootElementName=%s\n", config.rootElementName.c_str());

    int ret = 0;
    config.dbPort = cfgFile.getIntValue("DB", "dbport", ret);
    ret |= cfgFile.getValue("DB", "hostname", config.hostName);
    ret |= cfgFile.getValue("DB", "username", config.userName);
    ret |= cfgFile.getValue("DB", "password", config.passWord);
    ret |= cfgFile.getValue("DB", "database", config.database);
    ret |= cfgFile.getValue("DB", "tblname", config.tblName);

    if (ret != 0) {
        fprintf(stderr, "[ERROR]DB config error\n");
        return -1;
    }

    return 0;
}

/**
 *  功能描述:利用wget下载远程文件
 *  @param url 远程文件url路径
 *  @param savedFileName 要保持到本地的文件名
 *
 *  @return 成功返回0，失败返回其他值
 *  */
int downLoadFilebyWget(const char *url, const char *savedFileName)
{
    char cmd[512];

    sprintf(cmd, "wget %s -O %s", url, savedFileName);

    if (system(cmd) != 0) {
        fprintf(stderr, "[ERROR]execute command [%s] failed\n", cmd);
        return -1;
    }

    return 0;
}


/**
 *  功能描述:将xml导入数据库函数
 *  @param xmlFileName xml文件的本地路径名
 *  @param pfnExecuteSQL 数据库插入的钩子函数
 *  @param conn 数据库连接
 *  @param st 预编译后的语句句柄
 *
 *  @return 成功返回0,失败返回其他值
 *  */
int importXmltoDB(const char *xmlFileName,
                  const string &rootElementName,
                  const vector<string> &columnsName,
                  fnExecuteImportSQL pfnExecuteSQL,
                  void *conn,
                  void *st)
{
    size_t k = 0;
    int ret;
    vector<const char *> valueList;
    XMLDocument doc;

    printf("[INFO]begin to import %s\n", xmlFileName);

    doc.LoadFile(xmlFileName);

    if (doc.ErrorID() != 0) {
        fprintf(stderr, "[ERROR]Load File %s error,ErrorID=%d\n", xmlFileName, doc.ErrorID());
        return -1;
    }

    const XMLElement *results = doc.FirstChildElement(rootElementName.c_str());

    if (results == NULL) {
        fprintf(stderr, "[ERROR]There has no Results lable\n");
        return -1;
    }


    for (const XMLElement *result = results->FirstChildElement();
         result != NULL;
         result = result->NextSiblingElement()) {

        valueList.clear();

        for (size_t i = 0; i < columnsName.size(); ++i) {
            const char *value = result->FirstChildElement(columnsName[i].c_str())->GetText();
            printf("[INFO][%lu]:%s=%s\n", k, columnsName[i].c_str(), value);

            valueList.push_back(value);
        }

        if (pfnExecuteSQL != NULL) {
            ret = pfnExecuteSQL(conn, st, g_config.columnsType, valueList);

            if (ret != 0) {
                fprintf(stderr, "[ERROR] import data to DB failed\n");
                return ret;
            }
        }

        k++;
    }

    doc.Clear();

    return 0;
}

int main(int argc, char *argv[])
{
    char xmlFileName[256];
    void *conn = NULL;
    void *st = NULL;



    get_option(argc, argv);

    if (initConfig(g_config.cfgFilePath, g_config) != 0) {
        return -1;
    }

    /* 初始化数据库连接 */
    if (initDBConnect(g_config.hostName.c_str(),
                      g_config.userName.c_str(),
                      g_config.passWord.c_str(),
                      g_config.tblName.c_str(),
                      g_config.dbPort,
                      (void **)&conn)) {
        printDBError(conn, "[ERROR]initDBConnect failed");
        return -1;
    }

    /* 预编译导入数据库语句的句柄 */
    if (prepareInsertStatement(conn, g_config.database, g_config.tblName, g_config.columnsTblName , &st)) {
        printDBError(conn, "[ERROR]prepareStatement failed");
        closeDBConnect(conn);
        return -1;
    }

    printf("[INFO]the count of Xml File =%u\n",g_config.xmlFileList.size());
    for (size_t i = 0; i < g_config.xmlFileList.size(); ++i) {

        /* 传入 url 路径的会先下载到本地的临时文件中再处理 */
        if (g_config.xmlFileList[i].type == 1) {
            sprintf(xmlFileName, "%lu_%s", i, DEFAULT_DOWNLOAD_FILE_NAME);

            if (downLoadFilebyWget(g_config.xmlFileList[i].path, xmlFileName) != 0) {
                fprintf(stderr, "[WARNING]downLoad file [%s] failed", g_config.xmlFileList[i].path);
                continue;
            }
        } else {
            strcpy(xmlFileName, g_config.xmlFileList[i].path);
        }

        if (importXmltoDB(xmlFileName, g_config.rootElementName, g_config.columnsName,
                          ExecuteImportSQLHook, conn, st) != 0) {
            fprintf(stderr, "[WARNING]import file [%s] failed", g_config.xmlFileList[i].path);
            continue;
        }

        printf("[INFO]import file [%s] successfully\n", g_config.xmlFileList[i].path);
    }

    closeStmtHandle(st);
    closeDBConnect(conn); // 关闭链接

    return 0;
}
