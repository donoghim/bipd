// For TCP server
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

// MSISDN is combination of alpha id, bcd length, ton, npi, and dial number
// Length of MSISDN is 34 byte per sysmocom USIM
#define MSISDN_BYTE_SZ 34
#define BUF_SIZE 1024
#define MSG_HEADER_SZ 4

#define MODE_ACCEPT_ANY_ADDR 1 
#define MODE_ACCEPT_LS_ADDR 2

#define FINISH_AFTER_IMSI 1
#define FINISH_AFTER_MSISDN 2

#define REQ_HELLO        "U001"
#define RSP_WELCOME      "D001"

#define REQ_CHG_IMSI     "U002"
#define RSP_SEND_IMSI    "D002"
#define REQ_CMPLT_IMSI   "U003"
#define RSP_GO_AHEAD     "D003"

#define REQ_CHG_MSISDN   "U004"
#define RSP_SEND_MSISDN  "D004"
#define REQ_CMPLT_MSISDN "U005"
// #define RSP_FINISH       "D005"

#define REQ_UNKNOWN      "U011"
#define RSP_UNKNOWN      "D011"

// void error_handling(char *message);
void print_err(char *desc);

char rxdata[BUF_SIZE];
char txdata[BUF_SIZE];
FILE *flog;

int main(int argc, char *argv[])
{
  int serv_sock, clnt_sock;
  int recvLen;
  int sendLen;
  int finishAct;
  size_t inx = 0;
  size_t length;
  size_t offset;
  size_t bcd_len;
  FILE *fparam;
  unsigned char temp[32];
  unsigned char imsi_str[32];
  unsigned char msisdn_byte[MSISDN_BYTE_SZ];
  unsigned char alpha_id_str[32];
  unsigned char bcd_len_str[32];
  unsigned char dial_num_str[32];

  struct sockaddr_in serv_adr, clnt_adr;
  socklen_t clnt_adr_sz;

  printf("strat BIP server --------------- \n");

  // ---- get IMSI
  fparam = fopen("./imsi.txt", "r");
  memset(imsi_str, 0x00, 32);
  if(NULL == fparam)
    print_err("null imsi parameter file");
  if(NULL == fgets(imsi_str, 32, fparam))
    print_err("null imsi");
  length = strlen(imsi_str);
  if( (9<(length/2)) || 0!=(length%2) )
    print_err("length error in imsi");
  fclose(fparam);
  printf("<imsi> len:%ld string:%s\n", strlen(imsi_str), imsi_str);

  // ---- get alpha id from MSISDN
  fparam = fopen("./msisdn.txt", "r");
  if(NULL == fparam)
    print_err("null msisdn parameter file");
  if(NULL == fgets(alpha_id_str, 32, fparam))
    print_err("null msisdn");
  length = strlen(alpha_id_str)-1;
  if(20<length)
    print_err("null msisdn length is too long");
  printf("<alpha id> len:%ld string:%s\n", strlen(alpha_id_str), alpha_id_str);

  // ---- get bcd length from MSISDN
  if(NULL == fgets(bcd_len_str, 32, fparam))
    print_err("null bcd length");
  length = strlen(bcd_len_str);
  if(3<length)
    print_err("null bcd length string is too long");
  bcd_len = (size_t)atoi(bcd_len_str);
  if(10<bcd_len)
    print_err("null bcd length value is too large");
  printf("<bcd len> len:%ld string:%s\n", strlen(bcd_len_str), bcd_len_str);

  // ---- get dial number from MSISDN
  if(NULL == fgets(dial_num_str, 32, fparam))
    print_err("null dial number");
  length = strlen(dial_num_str)-1;
  if( (0!=(length%2)) || (bcd_len<(length/2)) )
    print_err("length error in dial number");
  printf("<dial num> len:%ld string:%s\n", strlen(dial_num_str)-1, dial_num_str);
  fclose(fparam);

  // ---- build MSISDN string
  memset(&msisdn_byte[0], 0xff, MSISDN_BYTE_SZ);
  memcpy(&msisdn_byte[0], alpha_id_str, strlen(alpha_id_str));
  msisdn_byte[20] = (unsigned char)bcd_len;
  msisdn_byte[21] = 0x91; // Type of Number and Numbering Plan ID
  for(inx=0; inx<(strlen(dial_num_str)-1)/2; inx++)
  {
    // printf(ld dial %02X %02X\n", inx, dial_num_str[inx*2], dial_num_str[inx*2+1]);
    msisdn_byte[22+inx]  = (dial_num_str[inx*2]-0x30);
    msisdn_byte[22+inx] |= (dial_num_str[inx*2+1]-0x30)<<4;
  }
  printf("MSISDM string\n");
  for(inx=0; inx<MSISDN_BYTE_SZ; inx++)
  {
    printf("%02X", msisdn_byte[inx]);
  }
  printf("\n");

  // ---- get port number of TCP server
  if(argc!=5)
  {
    printf("Num of arg mismatch !!!\n");
    printf("Usage: %s <mode> <IP address> <port> <fin>\n",argv[0]);
    printf("  mode:    1=accpet any address 2=accept only allowed\n");
    printf("  IP addr: Interface IP address to be allowed\n");
    printf("  port:    TCP port number\n");
    printf("  fin:     1=finish after IMSI 2:finish after MSISDN\n");
    print_err("number of arg mismatch");
  }
  printf("Server Config\n");
  int mode = atoi(argv[1]);
  if(MODE_ACCEPT_ANY_ADDR == mode)
    printf("Accept Any IP Address \n");
  else if(MODE_ACCEPT_LS_ADDR == mode)
    printf("Accept Listed IP Address\n");
  else
  {
    print_err("Not supported accept mode");
  }
  printf("IP addr: %s\n",argv[2]);
  printf("Port: %s\n",argv[3]);
  printf("Act: %s\n",argv[4]);
  finishAct = atoi(argv[4]);
  if( (FINISH_AFTER_IMSI != finishAct) && (FINISH_AFTER_MSISDN != finishAct) )
    print_err("Not supported act mode");

  // ---- create socket
  serv_sock=socket(PF_INET,SOCK_STREAM,0);
  if(serv_sock==-1)
  {
    print_err("socket() error");
  }
  int option = 1;
  setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  // ---- set address & port
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family=AF_INET;
  if(MODE_ACCEPT_ANY_ADDR == mode)
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
  else if(MODE_ACCEPT_LS_ADDR == mode)
    serv_adr.sin_addr.s_addr = inet_addr(argv[2]);
  serv_adr.sin_port=htons(atoi(argv[3]));
  clnt_adr_sz = sizeof(clnt_adr);

  // ---- binding
  if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr))==-1)
    print_err("bind() error");

  // ---- handle error
  if(listen(serv_sock,5)==-1)
    print_err("listen() error");

  // ---- handle received packet
  printf("start TCP echo server on port\n");
  inx = 0;
  while(1)
  {
    // handle conn request from USIM
    clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
    if(clnt_sock==-1)
    {
      print_err("accept() error");
      continue;
    }
    else
    {
      inx++;
      printf("Connect client %ld %d\n", inx, clnt_sock);
      memset(rxdata, 0x00, BUF_SIZE);
      memset(txdata, 0xFF, BUF_SIZE);
      while((recvLen = read(clnt_sock, rxdata, BUF_SIZE))!=0)
      {
        // clear TX data
        memset(txdata, 0x00, BUF_SIZE);

        // handle packet hello
        if(NULL != strstr(rxdata, REQ_HELLO))
        {
          printf("recv hello %d\n", recvLen);
          sendLen = write(clnt_sock, RSP_WELCOME, MSG_HEADER_SZ);
          printf("send welcome %d\n", sendLen);
        }

        // handle packet req change imsi
        else if(NULL != strstr(rxdata, REQ_CHG_IMSI))
        {
          printf("recv req change imsi %d\n", recvLen);
          memcpy(&txdata[0], RSP_SEND_IMSI, MSG_HEADER_SZ);
          memcpy(&txdata[MSG_HEADER_SZ], imsi_str, strlen(imsi_str)-1);
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ+strlen(imsi_str)-1);
          printf("send imsi %d\n", sendLen);
        }

        // handle packet complete imsi
        else if(NULL != strstr(rxdata, REQ_CMPLT_IMSI))
        {
          printf("recv complete imsi %d\n", recvLen);
          if(FINISH_AFTER_IMSI == finishAct)
          {
            // sendLen = write(clnt_sock, RSP_FINISH, MSG_HEADER_SZ);
            // printf("send finish %d\n", sendLen);
            close(clnt_sock);
            printf("Disconnect client %ld %d\n", inx, clnt_sock);
          }
          else
          {
            sendLen = write(clnt_sock, RSP_GO_AHEAD, MSG_HEADER_SZ);
            printf("send go ahead %d\n", sendLen);
          }
        }

        // handle packet req change msisdn
        else if(NULL != strstr(rxdata, REQ_CHG_MSISDN))
        {
          printf("recv req change msisdn %d\n", recvLen);
          memcpy(&txdata[0], RSP_SEND_MSISDN, MSG_HEADER_SZ);
          memcpy(&txdata[MSG_HEADER_SZ], msisdn_byte, MSISDN_BYTE_SZ);
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ+MSISDN_BYTE_SZ);
          printf("send msisdn %d\n", sendLen);
        }

        // handle packet complete msisdn
        else if(NULL != strstr(rxdata, REQ_CMPLT_MSISDN))
        {
          printf("recv complete msisdn %d\n", recvLen);
          if(FINISH_AFTER_MSISDN == finishAct)
          {
            // sendLen = write(clnt_sock, RSP_FINISH, MSG_HEADER_SZ);
            // printf("send finish %d\n", sendLen);
            close(clnt_sock);
            printf("Disconnect client %ld %d\n", inx, clnt_sock);
          }
        }

        else
        {
          printf("recv unknown cmd %d\n", recvLen);
          sendLen = write(clnt_sock, RSP_UNKNOWN, MSG_HEADER_SZ);
          printf("send rsp unkown %d\n", sendLen);
        } // end of handling recv packet

        // clear recv buffer
        memset(rxdata, 0, BUF_SIZE);

      } // end of read recv buffer
                  close(clnt_sock);
    } // end of handle conn request

  } // end of while(1)
  close(serv_sock);

  fclose(flog);
  return 0;
} // end of main()

#if 0
void print_err(char *rxdata)
{
  fputs(rxdata, stderr);
  fputc('\n',stderr);
  exit(1);
}
#endif

void print_err(char *desc)
{
  printf("desciption:%s\n", desc);
  fclose(flog);
  exit(1);
}

