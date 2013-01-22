#include "WinSock2.h"
#include "stdio.h"
#include "Defination.h"
#include  <fstream>
#include <string>
#include <time.h> 
#include <iostream>
#include<fstream>
#include <windows.h>
#include <direct.h>


using namespace std;


#define SERVER_PORT		15025
#define SERVER_IP		"172.20.14.204"
//#define SERVER_IP		"172.16.0.10"
//172.20.15.204

#pragma comment(lib,"ws2_32.lib")//这句关键;
int GetOneMessage(SOCKET s, char* output, short* output_len);
int DissectOneMessage(char* message);
int DissectPositionInfo(char* Info, short len);

FILE* createNewFile();

 FILE* pid, *plog;
 char* getCurrentTime();

void main()
{

	time_t lt1;
    time( &lt1 );
    struct tm *newtime=localtime(&lt1);
	char folder[30],logFile[50];

	 strftime(logFile, 128, "%Y-%m-%d.log", newtime);
	 plog=fopen(logFile,"w+");
    				
	WORD version = MAKEWORD(2,1);
	WSAData wsaData;
	int err;
	err = WSAStartup(version,&wsaData);
	if(err!=0)
	{  
		printf("WSA start failed !\n");
		fprintf(plog,"%s : WSA start failed !\n",getCurrentTime());
		Sleep(1000);
		return ;
	}
	else printf("WSA start succesffull !\n");
	protoent *ppe;
	ppe = getprotobyname("tcp");
	SOCKET clientsock = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
	if(clientsock==INVALID_SOCKET)
	{
		printf("INVALID SOCKET !\n");
		fprintf(plog,"%s : INVALID SOCKET !\n",getCurrentTime());
		Sleep(1000);
		return ;
	}
	else printf("soclet start succesffull !\n");
	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr=inet_addr(SERVER_IP);


	u_long iMode = 0;
	err = ioctlsocket(clientsock,FIONBIO,&iMode);
	if (err != NO_ERROR)
	{
	   printf("failed to ioctl socket !\n");
	   fprintf(plog,"%s : failed to ioctl socket !\n",getCurrentTime());
		Sleep(1000);
		
		return;
	}
	else printf("ioctl initial succesffull !\n");
	err = connect(clientsock, (sockaddr*)&addr, sizeof(addr));
	if(err==SOCKET_ERROR)
	{
		printf("---------------连接失败-------------------\n");
		fprintf(plog,"%s : ---------------连接失败-------------------\n",getCurrentTime());
		Sleep(1000);
		return ;
	}
	else printf("#------------连接成功---------------- !\n");

	char tempbuf[4096]={0};
	short buflen = 0;
	int count=0;


	 strftime(folder, 128, "%Y-%m-%d", newtime);
     mkdir(folder);
	 chdir(folder);




	printf("#---------连接成功，数据接收中.........\n");
	printf("#    .为一条位置信息\n");
	printf("#    #为一条警报信息\n");
	printf("#    *为一条营运数据\n");


	Sleep(1000);

	while(1)
	{
		memset(tempbuf,0,sizeof(tempbuf));
		buflen = sizeof(tempbuf);
		err = GetOneMessage(clientsock,tempbuf,&buflen); // 4096
		if(err != 0)
		{
			err = WSAGetLastError();
			break;
		}

	 //char *file=createNewFile();
	 //char filename[60];
	 //strcpy(filename,file);
	 //char path[127];
	 //sprintf(path,"%s\\%s",folder,filename);
	 //printf("%s",path);
	 //pid=fopen(path,"at+");
		pid= createNewFile();

	
		DissectOneMessage(tempbuf);
		//printf("%d  ",count++);

		
		fclose(pid);
	}
}

int GetOneMessage(SOCKET s, char* output, short* output_len)
{
	short* message_len=NULL;
	int err=0;
	memset(output,0,*output_len);

	err = recv(s,output,3,0);
	if (err==-1)
	{
		err = WSAGetLastError();
		printf("error: WSAGetLastError\n" );
	   fprintf(plog,"%s : error: WSAGetLastError\n",getCurrentTime());
		return -1;
	}
	message_len = (short*)(output+1);
	if( (*message_len+3) > *output_len )
	{	//output buffer dosen't have enough space
		printf("output buffer dosen't have enough space!");
		fprintf(plog,"%s : output buffer dosen't have enough space!\n",getCurrentTime());
		Sleep(2000);
		return -2;
	}
	err = recv(s,output+3,*message_len,0);
	if ( err==-1 )
	{
		err = WSAGetLastError();
		Sleep(1000);
		return -1;
	}

	*output_len = *message_len;
	return 0;
}


int DissectOneMessage(char* message)
{
	char* type=NULL;
	short* len=NULL;
	short offset=0;
	type = message + offset;  //消息类型
	offset = offset+1;


	len = (short*)(message+offset); //消息长度
	offset = offset + 2;



	switch (*type)
	{
	case 0x02:
		//printf("收到一个位置信息 len: %d\r\n",*len);
		printf(".");
     	DissectPositionInfo(message+offset,*len);
		//fprintf(pid,line);
		//fprintf(pid,"\n");

		break;
	case 0x03:	
		//printf("收到一个报警 len: %d\r\n",*len);
		printf("#");
		break;
	case 0x04:
		//printf("收到一个营运数据 len: %d\r\n",*len);
		printf("*");
		break;
	default :
		printf("  \n@@ 解析消息错误 @@ \n ");
		fprintf(plog,"%s :  @@ 解析消息错误 @@ type=%s \n",getCurrentTime(),*type);
		Sleep(1000);
		break;
	}
	return 0;
}

int DissectPositionInfo(char* Info, short len)
{
	short offset=0;
	short* unit_id = NULL;
	short* unit_len = NULL;
	char* unit_value = NULL;

	char plate[20]={0};
	int	 tempint=0;
	short tempshort1=0,tempshort2=0,tempshort3=0,tempshort4=0,tempshort5=0,tempshort6=0;

	char tempchar=0;


	while (offset<len)
	{
			unit_id = (short*)(Info + offset);
			offset = offset + 2;
			unit_len = (short*)(Info + offset);
			offset = offset + 2;
			unit_value = Info + offset;
			offset = offset + *unit_len;

		switch (*unit_id)
		{
		case POSINFO_PLATE_NUMBER:
			memcpy(plate,unit_value,*unit_len);
			//printf("	车牌号：%s\r\n",plate);
	     fprintf(pid,"\n%s,",plate?plate:NULL);
		 //sprintf(line,"%s,",plate);
			memset(plate,0,20);
			break;
		case POSINFO_LONGITUDE:
			tempint = *((int*)unit_value);
			//printf("	经度：%d\r\n",tempint);
		  fprintf(pid,"%d,",tempint?tempint:NULL);
		  //sprintf(line,"%d,",tempint);
			break;
		case POSINFO_LATITUDE:	
			tempint = *((int*)unit_value);
			//printf("	纬度：%d\r\n",tempint);
		fprintf(pid,"%d,",tempint?tempint:NULL);
		//sprintf(line,"%d,",tempint);
			break;
		case POSINFO_REPORT_TIME:	
			tempshort1 = *((short*)unit_value);
			tempchar = *(unit_value+2);
			tempshort2 = tempchar;
			tempchar = *(unit_value+3);
			tempshort3 = tempchar;
			tempchar = *(unit_value+4);
			tempshort4 = tempchar;
			tempchar = *(unit_value+5);
			tempshort5 = tempchar;
			tempchar = *(unit_value+6);
			tempshort6 = tempchar;
			//printf("	报告时间：%d-%d-%d   %d:%d:%d\r\n",tempshort1,tempshort2,tempshort3,tempshort4,tempshort5,tempshort6);
			fprintf(pid,"%d-%02d-%02d   %02d:%02d:%02d,",tempshort1,tempshort2,tempshort3,tempshort4,tempshort5,tempshort6);
			//sprintf(line,"%4d-%02d-%02d   %02d:%02d:%02d,",tempshort1,tempshort2,tempshort3,tempshort4,tempshort5,tempshort6);
			break;
		case POSINFO_DEV_ID:
			tempint = *((int*)unit_value);
			//printf("	设备编号：%d\r\n",tempint);
		fprintf(pid,"%d,",tempint?tempint:NULL);
	    //sprintf(line,"%d,",tempint);
			break;
		case POSINFO_SPEED:	
			tempshort1 = *((short*)unit_value);
			//printf("	速度：%d\r\n",tempshort1);
			fprintf(pid,"%d,",tempshort1?tempshort1:NULL);
			//sprintf(line,"%d,",tempshort1);
			break;
		case POSINFO_DIRECTION:	
			tempshort1 = *((short*)unit_value);
			//printf("	方向：%d\r\n",tempshort1);
		  fprintf(pid,"%d,",tempshort1?tempshort1:NULL);
		  //sprintf(line,"%d,",tempshort1);
			break;
		case POSINFO_LOCATION_STATUS:
			tempchar = *unit_value;
			tempshort1 = tempchar;
			//printf("	定位状态：%d\r\n",tempshort1);
		  fprintf(pid,"%d,",tempshort1?tempshort1:NULL);
		  //sprintf(line,"%d,",tempshort1);
			break;
		case ALARMINFO_SIM_NUMBER:
			memcpy(plate,unit_value,*unit_len);
			//printf("	SIM卡号：%s\r\n",plate);
		  fprintf(pid,"%s,",plate?plate:NULL);
		  //sprintf(line,"%d,",plate);
			memset(plate,0,20);
			break;
		case ALARMINFO_CAR_STATUS:
			tempchar = *unit_value;
			tempshort1 = tempchar;
			//printf("	车辆状态：%d\r\n",tempshort1);
		  fprintf(pid,"%d,",tempshort1?tempshort1:NULL);
		  //sprintf(line,"%d,",tempshort1);
			break;
		case ALARMINFO_CAR_COLOUR:
			memcpy(plate,unit_value,*unit_len);
			//printf("	车牌颜色：%s\r\n",plate);
	       fprintf(pid,"%s,",plate?plate:NULL);
		   //sprintf(line,"%s,%s",plate,"\n");
	    
			 //   pid=fopen(filename,"at+");	   	
				//fprintf(pid,line);
				//fclose(pid);
				//memset(line,0,sizeof(line));
			memset(plate,0,20);
			break;
		case 0:
			break;

		default:
			printf("	\n---解析【位置信息】错误---！！！ unit_id= %d \n",*unit_id);
		   fprintf(plog,"%s : ---解析【位置信息】错误---！！！ unit_id= %d \n",getCurrentTime(),*unit_id);
			Sleep(1000);
			break;

		}

	}
	return 0;
}

FILE* createNewFile()
{      
		FILE *fid1;
        struct tm *newtime;
        char tmpbuf[128];
		int year,month,day,hour,minute,second;
		
        time_t lt1;
        time( &lt1 );
        newtime=localtime(&lt1);

			 strftime(tmpbuf, 128, "%Y", newtime);
		       year=atoi(tmpbuf);
			     //printf("%d\n",year);

			 strftime(tmpbuf, 128, "%m", newtime);
		       month=atoi(tmpbuf);
			     //printf("%d\n",month);

 		     strftime( tmpbuf, 128, "%d", newtime);
		       day=atoi(tmpbuf); 
			   // printf("%d\n",day);

 		     strftime( tmpbuf, 128, "%H", newtime);
		       hour=atoi(tmpbuf);
			    //printf("%d\n",hour);

			 strftime( tmpbuf, 128, "%M", newtime);
		       minute=atoi(tmpbuf);
			   // printf("%d\n",minute);

 		     strftime( tmpbuf, 128, "%S", newtime);
		       second=atoi(tmpbuf);
		        // printf("%d %d %d %d %d %d \n",year,month,day,hour,minute,second);

			 //strftime( tmpbuf, 128, "%Y_%m_%d_%H_%M_%S", newtime);	
			 //fid1=fopen(tmpbuf, "at+");
			 // if( fid1==NULL )
				// {				
				//	  printf("error open %s file to write !\n",tmpbuf);
				//	  std::cin.get();
				//	  exit(1);
				//}



				 if(minute%30==0)
				 {

					  strftime( tmpbuf, 128, "%Y_%m_%d_%H_%M_%S", newtime);							
			 
				 }
				 else{

					 if(minute<30){minute=0;second=0;}
					 else if(minute>=30){minute=30;second=0;}
					 memset(tmpbuf, 0, sizeof(tmpbuf));
					

					 sprintf_s(tmpbuf,"%4d_%02d_%02d_%02d_%02d_%02d",year,month,day,hour,minute,second);
					 //printf("%4d_%02d_%02d_%02d_%02d_%02d",year,month,day,hour,minute,second);
				 
				 }	
				 fid1=fopen(tmpbuf, "at+");
				 //err=fopen_s(fid1,tmpbuf, "at+");
				 if( fid1==NULL )
				 {				
					  printf("error open %s file to write !\n",tmpbuf);
					  std::cin.get();
					  exit(1);
				}


				 return fid1;
				 //return tmpbuf;
}

char* getCurrentTime()
{
	time_t lt1;
    time( &lt1 );
    struct tm *newtime=localtime(&lt1);
	char now[30];

	 strftime(now, 128, "%Y-%m-%d-%H-%M-%S", newtime);

	 return now;

}

