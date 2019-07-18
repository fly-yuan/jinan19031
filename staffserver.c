#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h> //for sqlite3_open ..
#include <signal.h>
#include <time.h>//--------

//#include "common.h"
#define PATH_DATA "./staff.db" //数据库
#define N 32
#define M 128
#define R 0x1 //注册
#define L 0x2 //登录
#define Q 0x3  //查询单词
#define H 0x4  //历史记录

#define E 0x5 //退出
#define D 0x6  //
#define X 0x7
#define USER_Q 0X8
#define NAMELEN 16
#define DATALEN 128


int flags = 0;

typedef struct staff_info{
	int  no; 			//员工编号
	int  usertype;  	//ADMIN 1	USER 2	 
	char name[NAMELEN];	//姓名
	char pwd[8]; 	//密码
	int  age; 			// 年龄
	
}staff_info_t;

typedef struct{
	int type;//消息类型
	char name[N];//用户名
	char text[M];//单词  或 密码
	 //通信的消息
	int  flags;      //标志位
	staff_info_t info; 
}MSG;
#define LEN_SMG sizeof(MSG)
#define err_log(log)\
	do{\
	 perror(log);\
	 exit(1);\
	}while(0)

typedef struct sockaddr SA;
void get_system_time(char* timedata);
void history_init(MSG *msg,char *buf,sqlite3 *db);
int process_register(int clientfd,MSG *msg,sqlite3 *db);
void process_login(int clientfd,MSG *msg,sqlite3 *db);


int process_admin_query_request(int clientfd,MSG *msg,sqlite3 *db);
int process_admin_deluser_request(int clientfd,MSG *msg,sqlite3 *db);
int process_admin_modify_request(int clientfd,MSG *msg,sqlite3 *db);
void process_history(int clientfd,MSG *msg,sqlite3 *db);

int process_user_query_request(int clientfd,MSG *msg,sqlite3 *db);
void get_time(const char *date);
int history_callback(void *arg, int ncolumn, char **f_value, char **f_name);
int process_admin_history_request(int clientfd,MSG *msg,sqlite3 *db);
void handler(int arg)
{
	wait(NULL);
}
int main(int argc, const char *argv[])
{
	int serverfd,clientfd;
	struct sockaddr_in serveraddr,clientaddr;
	socklen_t len=sizeof(SA);
	int cmd;
	char clean[M]={0};
	MSG msg;
	pid_t pid;
	sqlite3 *db;
	ssize_t bytes;
	char *errmsg;
	if(argc!=3)
	{
		printf("User:%s <IP> <port>\n",argv[0]);
		return -1;
	}

	if(sqlite3_open(PATH_DATA,&db)!=SQLITE_OK)
	{
			printf("%s\n",sqlite3_errmsg(db));
			return -1;
	}
	if(sqlite3_exec(db,"create table user(staffno integer,name text,pwd text,age integer);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("create usrinfo table success.\n");
	}

	if(sqlite3_exec(db,"create table historyinfo(time text,name text,words text);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{ //华清远见创客学院         嵌入式物联网方向讲师
		printf("create historyinfo table success.\n");
	}
	if((serverfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		err_log("fail to socket");
	}
	/*优化： 允许绑定地址快速重用 */
	int b_reuse = 1;
	setsockopt (serverfd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof (int));

	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr=inet_addr(argv[1]);
	if(bind(serverfd,(SA*)&serveraddr,len)<0)
	{
		err_log("fail to bind");
	}
	if(listen(serverfd,10)<0)
	{
		err_log("fail to listen");
	}
 	signal(SIGCHLD,handler);//处理僵尸进程

	while(1)
	{
		if((clientfd=accept(serverfd,(SA*)&clientaddr,&len))<0)
		{
			perror("fail to accept");
			continue;
		}
		pid=fork();
		if(pid<0)
		{
			perror("fail to fork");
			continue;
		}
		else if(pid==0)
		{
			close(serverfd);
			while(1)
			{
				bytes=recv(clientfd,&msg,LEN_SMG,0);
				if(bytes<=0)
					break;
				switch(msg.type)
				{
					case R:
						process_register(clientfd,&msg,db);
						break;
					case L:
						process_login(clientfd,&msg,db);
						break;
					case Q:
						process_admin_query_request(clientfd,&msg,db);
						break;
					case D:
						process_admin_deluser_request(clientfd,&msg,db);
						break;
					case X:
						process_admin_modify_request(clientfd,&msg,db);
						break;
					case H:
						process_admin_history_request(clientfd,&msg,db);
						break;
					case USER_Q:
						process_user_query_request(clientfd,&msg,db);
						break;
					case E:
					    exit(0);
				}
			}
			close(clientfd);
			exit(1);
		}
		else
		{
			close(clientfd);
		}


	}
	
	return 0;
}
void process_login(int clientfd,MSG *msg,sqlite3 *db)
{
	char sql[M]={0};
	char *errmsg;
	char **rep;
	int n_row;
	int n_column;

	sprintf(sql,"select * from user where name='%s' and pwd='%s'",msg->name,msg->text);
	if(sqlite3_get_table(db,sql,&rep,&n_row,&n_column,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",errmsg);
		strcpy(msg->text,"Fail");
		send(clientfd,msg,LEN_SMG,0);
		return;
	}
	else
	{
		if(n_row==0)//查不到
		{

			strcpy(msg->text,"Fail");
			send(clientfd,msg,LEN_SMG,0);
			return;
		}
		else
		{

			strcpy(msg->text,"OK");
			send(clientfd,msg,LEN_SMG,0);
			return;
		}
	}
}

int process_register(int clientfd,MSG *msg,sqlite3 *db)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//封装sql命令--- 插入一个用户信息到数据库－如果插入成功则发送ＯＫ通知客户端--否则发送失败---同时插入历史记录数据
	char sql[DATALEN] = {0};
	char buf[DATALEN] = {0};
	char *errmsg;

	printf("%d\t %s\t %s\t %d\n",msg->info.no,msg->info.name,msg->info.pwd,\
		msg->info.age);
	
	sprintf(sql,"insert into user values(%d,'%s','%s',%d);",msg->info.no,msg->info.name,msg->info.pwd,\
		msg->info.age);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("----------%s.\n",errmsg);
		strcpy(msg->text,"failed");
		send(clientfd,msg,sizeof(MSG),0);
		return -1;             
	}else{
		strcpy(msg->text,"OK");
		send(clientfd,msg,sizeof(msg),0);
		printf("%s register success.\n",msg->info.name);
	}

	sprintf(buf,"管理员%s添加了%s用户",msg->name,msg->info.name);
	history_init(msg,buf,db);

	return 0;
}


int process_admin_query_request(int clientfd,MSG *msg,sqlite3 *db)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//检查msg->flags--->封装sql命令－查找历史记录表－回调函数－发送查询结果－发送结束标志

	
	int i = 0,j = 0;
	char sql[DATALEN] = {0};
	char **resultp;
	int nrow,ncolumn;
	char *errmsg;

	if(msg->flags == 1){
		sprintf(sql,"select * from user where name='%s';",msg->info.name);
	}else{
		sprintf(sql,"select * from user;");
	}
	
	if(sqlite3_get_table(db, sql, &resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("searching.....\n");
		printf("ncolumn :%d\tnrow :%d.\n",ncolumn,nrow);
		
		for(i = 0; i < ncolumn; i ++){
			printf("%-8s ",resultp[i]);
		}
		puts("");
		puts("=============================================================");
		
		int index = ncolumn;
		for(i = 0; i < nrow; i ++){
			//for(j = 0 ; j < ncolumn; j ++){
			//printf("%-8s ",resultp[index++]);
			printf("%s    %s     %s     %s\n",resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);
				
			sprintf(msg->text,"%s,   %s,   %s,   %s;",resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);
			
			send(clientfd,msg,sizeof(MSG),0);
			//}
			usleep(1000);
			puts("=============================================================");
			index += ncolumn;
		}
		
		if(msg->flags != 1){  //全部查询的时候不知道何时结束，需要手动发送结束标志位，但是按人名查找不需要
			//通知对方查询结束了
			strcpy(msg->text,"over*");
			send(clientfd,msg,sizeof(MSG),0);
		}
		
		sqlite3_free_table(resultp);
		printf("sqlite3_get_table successfully.\n");
	}



}
int process_admin_deluser_request(int clientfd,MSG *msg,sqlite3 *db)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//封装sql命令--- 插入一个用户信息到数据库－如果插入成功则发送ＯＫ通知客户端--否则发送失败---同时插入历史记录数据
	char sql[DATALEN] = {0};
	char buf[DATALEN] = {0};
	char *errmsg;

	printf("msg->info.no :%d\t msg->info.name: %s.\n",msg->info.no,msg->info.name);
	
	sprintf(sql,"delete from user where staffno=%d and name='%s';",msg->info.no,msg->info.name);	
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("----------%s.\n",errmsg);
		strcpy(msg->text,"failed");
		send(clientfd,msg,sizeof(MSG),0);
		return -1;
	}else{
		strcpy(msg->text,"OK");
		send(clientfd,msg,sizeof(msg),0);
		printf("%s deluser %s success.\n",msg->name,msg->info.name);
	}

	sprintf(buf,"管理员%s删除了%s用户",msg->name,msg->info.name);
	history_init(msg,buf,db);

	return 0;
}
int process_admin_modify_request(int clientfd,MSG *msg,sqlite3 *db)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	int nrow,ncolumn;
	char *errmsg, **resultp;
	char sql[DATALEN] = {0};	
	char historybuf[DATALEN] = {0};

	switch (msg->text[0])
	{
		case 'N':
			sprintf(sql,"update user set name='%s' where staffno=%d;",msg->info.name, msg->info.no);
			sprintf(historybuf,"%s修改工号为%d的用户名为%s",msg->name,msg->info.no,msg->info.name);
			break;
		case 'A':
			sprintf(sql,"update user set age=%d where staffno=%d;",msg->info.age, msg->info.no);
			sprintf(historybuf,"%s修改工号为%d的年龄为%d",msg->name,msg->info.no,msg->info.age);
			break;
		
		case 'M':
			sprintf(sql,"update user set passwd='%s' where staffno=%d;",msg->info.pwd, msg->info.no);
			sprintf(historybuf,"%s修改工号为%d的密码为%s",msg->name,msg->info.no,msg->info.pwd);
			break;
	}
	
	//调用sqlite3_exec执行sql命令
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
		sprintf(msg->text,"数据库修改失败！%s", errmsg);
	}else{
		printf("the database is updated successfully.\n");
		sprintf(msg->text, "数据库修改成功!");
		history_init(msg,historybuf,db);
	}

	//通知用户信息修改成功
	send(clientfd,msg,sizeof(MSG),0);

	printf("------%s.\n",historybuf);
	return 0;
}
int process_user_query_request(int clientfd,MSG *msg,sqlite3 *db)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//检查msg->flags--->封装sql命令－查找历史记录表－回调函数－发送查询结果－发送结束标志

	int i = 0,j = 0;
	char sql[DATALEN] = {0};
	char **resultp;
	int nrow,ncolumn;
	char *errmsg;

	sprintf(sql,"select * from user where name='%s';",msg->name);
	if(sqlite3_get_table(db, sql, &resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("searching.....\n");	
		for(i = 0; i < ncolumn; i ++){
			printf("%-8s ",resultp[i]);
		}
		puts("");
		puts("======================================================================================");
				
		int index = ncolumn;
		for(i = 0; i < nrow; i ++){
			printf("%s    %s     %s     %s\n",resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);
				
			sprintf(msg->text,"%s,   %s,   %s,   %s;",resultp[index+ncolumn-4],resultp[index+ncolumn-3],resultp[index+ncolumn-2],resultp[index+ncolumn-1]);
			
			send(clientfd,msg,sizeof(MSG),0);
			//}
			usleep(1000);
			puts("======================================================================================");
			index += ncolumn;
		}

		sqlite3_free_table(resultp);
		printf("sqlite3_get_table successfully.\n");
	}

}
void get_system_time(char* timedata)
{
	time_t t;
	struct tm *tp;

	time(&t);
	tp = localtime(&t);
	sprintf(timedata,"%d-%d-%d %d:%d:%d",tp->tm_year+1900,tp->tm_mon+1,\
			tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
	return ;
}

/******************************************************
*函数名：history_init
*参   数：线程处理结构体，用户处理内容buf
*******************************************************/
void history_init(MSG *msg,char *buf,sqlite3 *db)
{
	//获取当前时间--封装sql命令---将buf用户的操作记录插入到历史记录的表当中
	int nrow,ncolumn;
	char *errmsg, **resultp;
	char sqlhistory[DATALEN] = {0};
	char timedata[DATALEN] = {0};

	get_system_time(timedata);

	sprintf(sqlhistory,"insert into historyinfo values ('%s','%s','%s');",timedata,msg->name,buf);
	if(sqlite3_exec(db,sqlhistory,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
		printf("insert historyinfo failed.\n");
	}else{
		printf("insert historyinfo success.\n");
	}
}
int history_callback(void *arg, int ncolumn, char **f_value, char **f_name)
{
	int i = 0;
	MSG *msg= (MSG *)arg;
	int clientfd = msg->flags;

	if(flags == 0){
		for(i = 0; i < ncolumn; i++){
			printf("%-11s", f_name[i]);
		}
		putchar(10);
		flags = 1;
	}

	for(i = 0; i < ncolumn; i+=3)
	{
		printf("%s-%s-%s",f_value[i],f_value[i+1],f_value[i+2]);
		sprintf(msg->text,"%s---%s---%s.\n",f_value[i],f_value[i+1],f_value[i+2]);
		send(clientfd,msg,sizeof(MSG),0);
		usleep(1000);
	}
	puts("");

	return 0;
}


int process_admin_history_request(int clientfd,MSG *msg,sqlite3 *db)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//封装sql命令－查找历史记录表－回调函数－发送查询结果－发送结束标志
	char sql[DATALEN] = {0};
	char *errmsg;
	msg->flags = clientfd; //临时保存通信的文件描述符
	
	sprintf(sql,"select * from historyinfo;");
	if(sqlite3_exec(db,sql,history_callback,(void *)msg,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg); 	
	}else{
		printf("query history record done.\n");
	}

	//通知对方查询结束了
	strcpy(msg->text,"over*");
	send(clientfd,msg,sizeof(MSG),0);

	 flags = 0; //记得将全局变量修改为原来的状态
}
