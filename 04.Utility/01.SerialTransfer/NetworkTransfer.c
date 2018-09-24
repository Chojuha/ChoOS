#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DWORD unsigned int
#define BYTE unsigned char
#define MIN(a , b) (((a) < (b)) ? (a):(b))
#define MAX(a , b) (((a) > (b)) ? (a):(b))
#define SERIAL_FIFOMAXSIZE 16

int main(int argc , char **argv) {
	char FileName[256];
	char DataBuffer[SERIAL_FIFOMAXSIZE];
	struct sockaddr_in SocketAddr;
	int Socket;
	BYTE Ack;
	DWORD DataLength;
	DWORD SentSize;
	DWORD Temp;
	FILE *fp;
	if(argc < 2) {
		fprintf(stderr , "Please Input File Name : ");
		gets(FileName);
	}
	else {
		strcpy(FileName , argv[1]);
	}
	fp = fopen(FileName , "rb");
	if(fp == NULL) {
		fprintf(stderr , "File Not Fount : %s\n" , FileName);
		return 0;
	}
	fseek(fp , 0 , SEEK_END);
	DataLength = ftell(fp);
	fseek(fp , 0 , SEEK_SET);
	fprintf(stderr , "<File Read Success>\n\nFile Name : %s\nFile Size : %dB\n" , FileName , DataLength);
	SocketAddr.sin_family = AF_INET;
	SocketAddr.sin_port = htons(4444);
	SocketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	Socket = socket(AF_INET , SOCK_STREAM , 0);
	if(connect(Socket , (struct sockaddr*)&SocketAddr , sizeof(SocketAddr)) == -1) {
		fprintf(stderr , "Socket Connet Error.\nIP Address : 127.0.0.1\nPort : 4444\n");
		return 0;
	}
	else {
		fprintf(stderr , "Socket Connet Success.\nIP Address : 127.0.0.1\nPort : 4444\n");
	}
	if(send(Socket , &DataLength , 4 , 0) != 4) {
		fprintf(stderr , "Data Length Send Fail.\n%dB\n" , DataLength);
		return 0;
	}
	else {
		fprintf(stderr , "Data Length Send Sucess.\n%dB\n" , DataLength);
	}
	if(recv(Socket , &Ack , 1 , 0) != 1) {
		fprintf(stderr , "Ack receive Error.\n");
		return 0;
	}
	fprintf(stderr , "\nData Transfer Start.\nStatus:");
	SentSize = 0;
	while(SentSize < DataLength) {
		Temp = MIN(DataLength-SentSize , SERIAL_FIFOMAXSIZE);
		SentSize += Temp;
		if(fread(DataBuffer , 1 , Temp , fp) != Temp) {
			fprintf(stderr , "File Read Error.\n");
			return 0;
		}
		if(send(Socket , DataBuffer , Temp , 0) != Temp) {
			fprintf(stderr , "Socket Send Error\n");
			return 0;
		}
		if(recv(Socket , &Ack , 1 , 0) != 1) {
			fprintf(stderr , "Ack Receive Error\n");
		}
		fprintf(stderr , "#");
	}
	fclose(fp);
	close(Socket);
	fprintf(stderr , "\nSend Complete.\nTotal Size : %dB\n" , SentSize);
	getchar();
	return 0;
}
