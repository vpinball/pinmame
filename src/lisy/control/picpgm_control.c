/*
 socketserver for picpgm webinterface
 bontango 11.2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <wiringPi.h>

char update_path[255]=" "; //' ' indicates that there is no update file

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//short form of write command
void sendit( int sockfd, char *buffer)
{
   int n;

//   printf("%s",buffer);

   n = write(sockfd,buffer,strlen(buffer));
   if (n < 0) error("ERROR writing to socket");
}

//do an update of the pic
void do_flash( int sockfd, char *what)
{
 FILE *fstream;
 char *orgname, *source;
 char *line;
 char buffer[512];
 int ret_val,line_nu;
 //we trust ASCII values
 //the format here is 'F'

 line = &what[1];

 orgname = strtok(line, ";");
 source = strtok(NULL, ";");

 sprintf(buffer,"<a href=\"./index.php\">Back to PICPGM Homepage</a><br><br>");
 sendit( sockfd, buffer);

 sprintf(buffer,"WE WILL NOW do the flashing<br><br>\n");
 sendit( sockfd, buffer);
 sprintf(buffer,"by using file: %s<br><br>\n",orgname);
 sendit( sockfd, buffer);

 sprintf(buffer,"excute: /usr/bin/picpgm %s<br><br>\n",update_path);
 sendit( sockfd, buffer);
 sprintf(buffer,"/usr/bin/picpgm -p %s >/tmp/picpgm_output",source);
 ret_val = system(buffer);

 //print out what picpgm is giving us
  line_nu = 0;
  fstream = fopen("/tmp/picpgm_output","r");
  while( ( line=fgets(buffer,sizeof(buffer),fstream) ) != NULL)
   {
     sprintf(buffer,"%s<br><br>\n",line);
     sendit( sockfd, buffer);
     if ( ++line_nu > 15 ) break; //maximum 15 lines
   }

 if (ret_val != 0)
    {
     sprintf(buffer,"return value of picpgm was %d<br><br>",ret_val);
     sendit( sockfd, buffer);
     sprintf(buffer,"<h2>flashing  failed</h2><br><br>");
     sendit( sockfd, buffer);
    }
  else
    {
     sprintf(buffer,"flashing done<br><br>");
    }

 sprintf(buffer,"<h2>OK</h2><br><br>\n");
 sendit( sockfd, buffer);
 sprintf(buffer,"you may now go back and flash another file<br><br>\n");
 sendit( sockfd, buffer);
}




//send infos for the homepage
void send_home_infos( int sockfd )
{
     char buffer[256];

   sprintf(buffer,"<h2>LISY35 Flash Utility Home Page</h2> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./flashpic.html\">flash a PIC</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_exit.php\">Exit</a><br><br> \n");
   sendit( sockfd, buffer);
}

/*
void send_flashPIC_infos( int sockfd )
{
  char buffer[256];
  int ret_val;

  if ( update_path[0] != ' ')  //there was a setting of the update path
  {
    sprintf(buffer,"WE WILL NOW flash the PIC<br><br>\n");
    sendit( sockfd, buffer);


    //try to get the update file with wget
    sprintf(buffer,"excute: /usr/local/bin/picpgm %s<br><br>\n",update_path);
    sendit( sockfd, buffer);
    sprintf(buffer,"/usr/local/bin/picpgm %s",update_path);
    ret_val = system(buffer);

    if (ret_val != 0)
    {
     sprintf(buffer,"return value of wget was %d<br><br>",ret_val);
     sendit( sockfd, buffer);
     sprintf(buffer,"update failed<br><br>");
     sendit( sockfd, buffer);
    }
    else  //just unpack the lisy_update.tgz and execute install.sh from within
    {

      sprintf(buffer,"update done, you may want to reboot now<br><br>\n");
      sendit( sockfd, buffer);

    }

    //reset update path to prevent endless loop
    update_path[0] = ' ';


   }
  else  //normal header
  {

  //start with some header
  sprintf(buffer,"do an update, with the URL specified<br><br>\n");
  sendit( sockfd, buffer);
  sprintf(buffer,"NOTE: do only use URLs form lisy80.com<br><br>\n");
  sendit( sockfd, buffer);

  sprintf(buffer,"<p>  Update path: <input type=\"text\" name=\"U\" size=\"100\" maxlength=\"250\" value=\"\" /></p>");
  sendit( sockfd, buffer);

  sprintf(buffer,"<p><input type=\"submit\" /></p> ");
  sendit( sockfd, buffer);
  }

}
*/

//****** MAIN  ******


int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[512];
     char ip_interface[10];
     struct sockaddr_in serv_addr, cli_addr, *myip;
     int n;
     int do_exit = 0;
     struct ifreq ifa;
     char *line;
     int tries = 0;


     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     //try to find out our IP on eth0
     strcpy (ifa.ifr_name, "eth0");
     strcpy (ip_interface, "ETH0"); //upercase for message
     if((n=ioctl(sockfd, SIOCGIFADDR, &ifa)) != 0)
      {
        //no IP on eth0, we try wlan0 now, 20 times
        strcpy (ifa.ifr_name, "wlan0");
        strcpy (ip_interface, "WLAN0"); //upercase for message
        do
        {
          sleep(1);
          n=ioctl(sockfd, SIOCGIFADDR, &ifa);
          tries++;
        } while ( (n!=0) &( tries<20));
      }



     if(n) //no IP found
     {
        printf("NO IP\n");
	exit(1);
     }
     else //we found an IP
     {
      myip = (struct sockaddr_in*)&ifa.ifr_addr;
	//get teh pouinter to teh Ip address
        line = inet_ntoa(myip->sin_addr);
        printf("IP is %s\n",line);
     }


     //now set up the socketserver
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 5963;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     //wait and listen
    do {
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");

     //the home screen woth all available commands
     if ( strcmp( buffer, "home") == 0) { send_home_infos(newsockfd); close(newsockfd); }
     //provide simple input field for initiating flash of PIC
     //else if ( strcmp( buffer, "flashPIC") == 0) { send_flashPIC_infos(newsockfd); close(newsockfd); }
     //should we exit?
     else if ( strcmp( buffer, "exit") == 0) do_exit = 1;
     //with an uppercase 'F' we do try to initiate flash of PIC
     else if (buffer[0] == 'F') { do_flash(newsockfd,buffer);close(newsockfd); }
     //as default we print out what we got
     else fprintf(stderr,"Message: %s\n",buffer);


  } while (do_exit == 0);

     close(newsockfd);
     close(sockfd);
     return 0; 
}

