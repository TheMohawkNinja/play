#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string>
#include <fstream>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char ** argv)
{
	int fd, n, RWin_State, F12_State, bytesReceived, port;
	int sock=socket(AF_INET,SOCK_STREAM,0);
	int connectRes=INT16_MAX;
	bool inputIsSwitched=false;
	char buf[256], dataBuffer[1];
	std::string line,usbip,keyFD,keyboard,mouse,IP;
	std::string user=getlogin();
	std::string configPath="/home/"+user+"/.config/play/config";
	std::ifstream configReader;

	//Get configuration information
	try
	{
		configReader.open(configPath);

		while(!configReader.eof())
		{
			getline(configReader,line);

			if(line.substr(0,1)!="#")//Hashtag denotes comments
			{
				if(line.find("usbip")==0)
				{
					usbip=line.substr(line.find("=")+1,line.length()-line.find("="));
					fprintf(stdout,"Setting Windows usbip path to %s\n",usbip.c_str());
				}
				else if(line.find("keyFD")==0)
				{
					keyFD=line.substr(line.find("=")+1,line.length()-line.find("="));
					fprintf(stdout,"Setting keyboard file descriptor path to %s\n",keyFD.c_str());
				}
				else if(line.find("keyboard")==0)
				{
					keyboard=line.substr(line.find("=")+1,line.length()-line.find("="));
					fprintf(stdout,"Setting busid for keyboard to %s\n",keyboard.c_str());
				}
				else if(line.find("mouse")==0)
				{
					mouse=line.substr(line.find("=")+1,line.length()-line.find("="));
					fprintf(stdout,"Setting busid for mouse to %s\n",mouse.c_str());
				}
				else if(line.find("IP")==0)
				{
					IP=line.substr(line.find("=")+1,line.length()-line.find("="));
					fprintf(stdout,"Setting IP to %s\n",mouse.c_str());
				}
				else if(line.find("port")==0)
				{
					port=stoi(line.substr(line.find("=")+1,line.length()-line.find("=")));
					fprintf(stdout,"Setting port to %d\n",port);
				}
			}
		}
		if(usbip=="")
		{
			fprintf(stderr,"usbip path not found\n");
			return -2;
		}
		if(keyFD=="")
		{
			fprintf(stderr,"keyboard file descriptor path not found\n");
			return -3;
		}
		if(keyboard=="")
		{
			fprintf(stderr,"keyboard busid not found\n");
			return -4;
		}
		if(mouse=="")
		{
			fprintf(stderr,"mouse busid not found\n");
			return -5;
		}
		if(IP=="")
		{
			fprintf(stderr,"IP not found\n");
			return -6;
		}
		if(port==0)
		{
			fprintf(stderr,"port not found\n");
			return -7;
		}
	}
	catch(...)
	{
		fprintf(stderr,"Error while attempting to read config file at \"%s\"\n",configPath.c_str());
		return -1;
	}

	//Create a hint structure for the server we're connecting with
	sockaddr_in hint;
	hint.sin_family=AF_INET;
	hint.sin_port=htons(port);
	inet_pton(AF_INET,IP.c_str(),&hint.sin_addr);

	if(sock==-1)
	{
		fprintf(stderr,"Unable to create socket on port %d\n",port);
		return -8;
	}
	else
	{
		fprintf(stdout,"Successfully initialized socket %d on port %d\n",sock,port);
	}

	//Connect to the server on the socket
	connectRes=connect(sock,(sockaddr*)&hint,sizeof(hint));

	if(connectRes==-1)
	{
		fprintf(stderr,"Could not connect to %s:%d\n",IP.c_str(),port);
		return -9;
	}
	else
	{
		fprintf(stdout,"Successfully connected to %s:%d\n",IP.c_str(),port);
	}


	//Open keyboard file descriptor
	fd = open(("/dev/input/by-id/"+keyFD).c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	if (fd == -1)
	{
		fprintf(stderr,"open_port: Unable to open /dev/input/by-id/%s - \n",keyFD.c_str());
		return -10;
	}

	// Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
	fcntl(fd, F_SETFL, 0);

	sleep(1);

	while(1)
	{
		n=read(fd,(void*)buf, 255);
		if (n==0)
		{
			fprintf(stdout,"No data on port\n");
		}
		else if(n>0)
		{
			if(!inputIsSwitched)
			{
				buf[n] = '\0';
				//printf("%i bytes read : %s", n, buf);

				//printf("Key: %d\n",buf[42]);
				//printf("State: %d\n",buf[44]);
				/*for(int i=0; i<n; i++)
				{
					printf("buf[%d]=%d\n",i,buf[i]);
					printf("%d-",buf[i]);
				}*/
				printf("\n");

				if(buf[42]==126)
				{
					RWin_State=buf[44];
				}
				else if(buf[42]==88)
				{
					F12_State=buf[44];
				}

				if(RWin_State==1&&F12_State==1)
				{
					fprintf(stdout,"Switching input stream to Windows\n");
					inputIsSwitched=true;

					system(("usbip bind --busid="+keyboard).c_str());
					system(("winrun \""+usbip+" attach_ude -r 192.168.1.2 --busid="+keyboard+"\"").c_str());
					system(("usbip bind --busid="+mouse).c_str());
					system(("winrun \""+usbip+" attach_ude -r 192.168.1.2 --busid="+mouse+"\"").c_str());

					//Artifically set to 0 to avoid them "sticking" in program
					RWin_State=0;
					F12_State=0;
				}
			}
			else
			{
				//Wait to receive change signal from chin_win
				bytesReceived=recv(sock,dataBuffer,1,0);

				if(bytesReceived>0)
				{
					fprintf(stdout,"Switching input stream to Linux\n");
					inputIsSwitched=false;

					system(("usbip unbind --busid="+keyboard).c_str());
					system(("usbip unbind --busid="+mouse).c_str());
				}
				else
				{
					fprintf(stderr,"Zero-length buffer recieved\n");
					return -11;
				}
			}
		}
	}

	close(fd);
	return 0;
}
