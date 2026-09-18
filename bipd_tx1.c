// For TCP server
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
// For json parsing
#include <stdio.h>
#include <json-c/json.h>
// For logging
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#define PARAM_FILE_NAME  "./param.json"
#define SERVER_INTF_IP_ADDR  "123.123.123.123"

// MSISDN is combination of alpha id, bcd length, ton, npi, and dial number
// Length of MSISDN is 34 byte per sysmocom USIM
#define MSISDN_BYTE_SZ 34
#define BUF_SIZE       1024
#define MSG_HEADER_SZ  4
#define MAX_STR_LEN    32

#define TEST1_DATA_SZ  250
#define TEST2_DATA_SZ  500
#define TEST3_DATA_SZ  750
#define TEST4_DATA_SZ  1000

#define MODE_ACCEPT_ANY_ADDR 1
#define MODE_ACCEPT_LS_ADDR  2

#define FINISH_AFTER_IMSI    1
#define FINISH_AFTER_MSISDN  2

// #define SEND_TEST_HEADER_AND_DATA 0 // 0:header only
#define SEND_TEST_HEADER_AND_DATA 1 // 1:header and payload,
// #define SEND_TEST_DATA_LEN 237

#define REQ_HELLO        "U001"
#define RSP_WELCOME      "D001"

#define REQ_TEST1        "U011"
#define RSP_TEST1        "D011" // send DL response data 250 bytes
#define REQ_TEST2        "U012"
#define RSP_TEST2        "D012" // send DL response data 500 bytes
#define REQ_TEST3        "U013"
#define RSP_TEST3        "D013" // send DL response data 750 bytes
#define REQ_TEST4        "U014"
#define RSP_TEST4        "D014" // send DL response data 1000 bytes

#define REQ_CHG_IMSI     "U002"
#define RSP_SEND_IMSI    "D002"
#define REQ_CMPLT_IMSI   "U003"
#define RSP_GO_AHEAD     "D003"

#define REQ_CHG_MSISDN   "U004"
#define RSP_SEND_MSISDN  "D004"
#define REQ_CMPLT_MSISDN "U005"
#define RSP_FINISH       "D005"

#define REQ_UNKNOWN      "U999"
#define RSP_UNKNOWN      "D999"

char rxdata[BUF_SIZE];
char txdata[BUF_SIZE];

int main(int argc, char *argv[])
{
  struct json_object *parsed_json;
  struct json_object *js_acceptMode;
  // struct json_object *js_ipAddress;
  struct json_object *js_port;
  struct json_object *js_finishMode;
  struct json_object *js_imsi;
  struct json_object *js_msisdn_alphaId;
  struct json_object *js_msisdn_bcdLen;
  struct json_object *js_msisdn_dialNum;
  char jsonStr[1024];

  int acceptMode;
  char ipAddr[64];
  int  listenPort;
  int finishMode;

  unsigned char imsi_str[MAX_STR_LEN];
  unsigned char alpha_id_str[MAX_STR_LEN];
  unsigned char bcd_len_str[MAX_STR_LEN];
  unsigned char dial_num_str[MAX_STR_LEN];
  unsigned char msisdn_byte[MSISDN_BYTE_SZ];

  int serv_sock, clnt_sock;
  int recvLen;
  int sendLen;
  size_t inx = 0;
  size_t length;
  size_t offset;
  size_t bcd_len;
  FILE *fparam;
  size_t readLen;

  struct sockaddr_in serv_adr, clnt_adr;
  socklen_t clnt_adr_sz;

  int rc = chdir("./");
  if (rc < 0)
  {
    printf("error in chdir():%d\n", rc);
    return -4;
    // exit(EXIT_FAILURE);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  openlog("bipLog", LOG_PID, LOG_DAEMON);
  // openlog("bipLog", LOG_CONS | LOG_PID, LOG_USER);

  syslog(LOG_INFO, "------ bip interface down\n");
  system("sudo ip link set bip down");
  sleep(1);
  system("ip tuntap del dev bip mode tap");
  sleep(1);

  syslog(LOG_INFO, "------ up bip interface\n");
  system("ip tuntap add mode tap bip");
  sleep(1);
  system("ip link set bip up");
  sleep(1);
  system("ip addr add 123.123.123.123 dev bip");
  sleep(1);

  syslog(LOG_INFO, "------ start BIP TX1 server\n");
#if (SEND_TEST_HEADER_AND_DATA)
  syslog(LOG_INFO, "send test header and data\n");
#else
  syslog(LOG_INFO, "send test header only\n");
#endif

  // open json formatted parameter file
  fparam = fopen(PARAM_FILE_NAME, "r");
  fread(jsonStr, 1024, 1, fparam);
  fclose(fparam);

  // parse json file
  parsed_json = json_tokener_parse(jsonStr);

  // extract json object
  json_object_object_get_ex(parsed_json, "imsi", &js_imsi);
  json_object_object_get_ex(parsed_json, "msisdn_alphaId", &js_msisdn_alphaId);
  json_object_object_get_ex(parsed_json, "msisdn_bcdLen", &js_msisdn_bcdLen);
  json_object_object_get_ex(parsed_json, "msisdn_dialNum", &js_msisdn_dialNum);
  json_object_object_get_ex(parsed_json, "accpt_mode", &js_acceptMode);
  json_object_object_get_ex(parsed_json, "finish_mode", &js_finishMode);
  // json_object_object_get_ex(parsed_json, "ip_address", &js_ipAddress);
  json_object_object_get_ex(parsed_json, "port", &js_port);

  // parse IMSI
  strcpy(imsi_str, json_object_get_string(js_imsi));
  length = strlen(imsi_str);
  if( (9<(length/2)) || 0==(length%2) )
  {
    syslog(LOG_PERROR, "ERROR: imsi len :%ld\n", length);
    exit(1);
  }
  syslog(LOG_INFO, "imsi str len:%ld string:%s\n", strlen(imsi_str), imsi_str);

  // parse alpha ID
  strcpy(alpha_id_str, json_object_get_string(js_msisdn_alphaId));
  length = strlen(alpha_id_str)-1;
  if(20<length)
  {
    syslog(LOG_PERROR, "ERROR: alpha id len:%ld\n", length);
    exit(2);
  }
  syslog(LOG_INFO, "alpha id str len:%ld string:%s\n", strlen(alpha_id_str), alpha_id_str);

  // parse BCD length
  bcd_len = (size_t)json_object_get_int(js_msisdn_bcdLen);
  if(10<bcd_len)
  {
    syslog(LOG_PERROR, "ERROR: bcd len:%ld\n", length);
    exit(3);
  }
  syslog(LOG_INFO, "bcd len:%ld\n", bcd_len);

  // parse dial number
  strcpy(dial_num_str, json_object_get_string(js_msisdn_dialNum));
  length = strlen(dial_num_str);
  if( (0!=(length%2)) || (bcd_len<(length/2)) )
  {
    syslog(LOG_PERROR, "ERROR: dial num len:%ld\n", length);
    exit(4);
  }
  syslog(LOG_INFO, "<dial num> len:%ld string:%s\n", strlen(dial_num_str), dial_num_str);

  // ---- build MSISDN string
  memset(&msisdn_byte[0], 0xff, MSISDN_BYTE_SZ);
  memcpy(&msisdn_byte[0], alpha_id_str, strlen(alpha_id_str));
  msisdn_byte[20] = (unsigned char)bcd_len;
  msisdn_byte[21] = 0x91; // Type of Number and Numbering Plan ID
  for(inx=0; inx<(strlen(dial_num_str))/2; inx++)
  {
    // syslog(LOG_INFO, "dial %ld %02X %02X\n", inx, dial_num_str[inx*2], dial_num_str[inx*2+1]);
    msisdn_byte[22+inx]  = (dial_num_str[inx*2]-0x30);
    msisdn_byte[22+inx] |= (dial_num_str[inx*2+1]-0x30)<<4;
  }
  syslog(LOG_INFO, "MSISDM string\n");
  unsigned char strBuf[128];
  memset(strBuf, 0x00, 128);
  for(inx=0; inx<MSISDN_BYTE_SZ; inx++)
  {
    sprintf(&strBuf[inx*2], "%02X", msisdn_byte[inx]);
  }
  syslog(LOG_INFO, "%s\n", strBuf);

  // parse server accept mode
  acceptMode = json_object_get_int(js_acceptMode);
  syslog(LOG_INFO, "Server Config\n");
  if(MODE_ACCEPT_ANY_ADDR == acceptMode)
    syslog(LOG_INFO, "Accept Any IP Address \n");
  else if(MODE_ACCEPT_LS_ADDR == acceptMode)
    syslog(LOG_INFO, "Accept Listed IP Address\n");
  else
  {
    syslog(LOG_PERROR, "Not supported accept mode\n");
  }

  // parse finish mode
  finishMode = json_object_get_int(js_finishMode);
  if(FINISH_AFTER_IMSI == finishMode)
    syslog(LOG_INFO, "finish after IMSI\n");
  if(FINISH_AFTER_MSISDN == finishMode)
    syslog(LOG_INFO, "finish after MSISDN\n");
  else
    syslog(LOG_INFO, "Not supported act mode\n");

  // strcpy(ipAddr, json_object_get_string(js_ipAddress));
  strcpy(ipAddr, SERVER_INTF_IP_ADDR);
  listenPort = json_object_get_int(js_port);
  // listenPort = 22001;
  // listenPort = 8080;
  syslog(LOG_INFO, "IP addr: %s\n", ipAddr);
  syslog(LOG_INFO, "Port: %d\n", listenPort);

  // ---- create socket
  serv_sock=socket(PF_INET,SOCK_STREAM,0);
  if(serv_sock==-1)
  {
    syslog(LOG_PERROR, "ERROR: socket() error\n");
  }

  int option = 1; // allow reuse address
  setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  // ---- set address & port
  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family=AF_INET;
  if(MODE_ACCEPT_ANY_ADDR == acceptMode)
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
  else if(MODE_ACCEPT_LS_ADDR == acceptMode)
    serv_adr.sin_addr.s_addr = inet_addr(ipAddr);
  serv_adr.sin_port=htons(listenPort);
  clnt_adr_sz = sizeof(clnt_adr);

  // decrement reference count and free object
  json_object_put(parsed_json);

  // ---- binding
  if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr))==-1)
    syslog(LOG_PERROR, "ERROR: bind() error");

  // ---- handle error
  if(listen(serv_sock,5)==-1)
    syslog(LOG_PERROR, "ERROR: listen() error");

  // ---- handle received packet
  syslog(LOG_INFO, "BIP server is listening\n");
  inx = 0;
  while(1)
  {
    // handle conn request from USIM
    clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
    if(clnt_sock==-1)
    {
      syslog(LOG_PERROR, "ERROR: accept() error");
      continue;
    }
    else
    {
      inx++;
      syslog(LOG_INFO, "------ Connect client %ld %d\n", inx, clnt_sock);
      memset(rxdata, 0x00, BUF_SIZE);
      memset(txdata, 0xFF, BUF_SIZE);
      while((recvLen = read(clnt_sock, rxdata, BUF_SIZE))!=0)
      {
        // clear TX data
        memset(txdata, 0x00, BUF_SIZE);

        // handle packet hello
        if(NULL != strstr(rxdata, REQ_HELLO))
        {
          syslog(LOG_INFO, "recv hello %d\n", recvLen);
          sendLen = write(clnt_sock, RSP_WELCOME, MSG_HEADER_SZ);
          syslog(LOG_INFO, "send welcome %d\n", sendLen);
        }

        // handle packet test1
        else if(NULL != strstr(rxdata, REQ_TEST1))
        {
          syslog(LOG_INFO, "recv test1 %d\n", recvLen);
          memcpy(&txdata[0], RSP_TEST1, MSG_HEADER_SZ);
#if (SEND_TEST_HEADER_AND_DATA)
          memset(&txdata[MSG_HEADER_SZ], 0x31, TEST1_DATA_SZ);
          sendLen = write(clnt_sock, txdata, TEST1_DATA_SZ+MSG_HEADER_SZ); // org
          // sendLen = write(clnt_sock, txdata, SEND_TEST_DATA_LEN); // org
#else
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ); // young test
#endif
          syslog(LOG_INFO, "send test1 %d\n", sendLen);
        }

        // handle packet test2
        else if(NULL != strstr(rxdata, REQ_TEST2))
        {
          syslog(LOG_INFO, "recv test2 %d\n", recvLen);
          memcpy(&txdata[0], RSP_TEST2, MSG_HEADER_SZ);
#if (SEND_TEST_HEADER_AND_DATA)
          memset(&txdata[MSG_HEADER_SZ], 0x32, TEST2_DATA_SZ);
          sendLen = write(clnt_sock, txdata, TEST2_DATA_SZ+MSG_HEADER_SZ); // org
          // sendLen = write(clnt_sock, txdata, SEND_TEST_DATA_LEN); // org
#else
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ); // young test
#endif
          syslog(LOG_INFO, "send test2 %d\n", sendLen);
        }

        // handle packet test3
        else if(NULL != strstr(rxdata, REQ_TEST3))
        {
          syslog(LOG_INFO, "recv test3 %d\n", recvLen);
          memcpy(&txdata[0], RSP_TEST3, MSG_HEADER_SZ);
#if (SEND_TEST_HEADER_AND_DATA)
          memset(&txdata[MSG_HEADER_SZ], 0x33, TEST3_DATA_SZ);
          sendLen = write(clnt_sock, txdata, TEST3_DATA_SZ+MSG_HEADER_SZ); // org
          // sendLen = write(clnt_sock, txdata, SEND_TEST_DATA_LEN); // org
#else
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ); // young test
#endif
          syslog(LOG_INFO, "send test3 %d\n", sendLen);
        }

        // handle packet test4
        else if(NULL != strstr(rxdata, REQ_TEST4))
        {
          syslog(LOG_INFO, "recv test4 %d\n", recvLen);
          memcpy(&txdata[0], RSP_TEST4, MSG_HEADER_SZ);
#if (SEND_TEST_HEADER_AND_DATA)
          memset(&txdata[MSG_HEADER_SZ], 0x34, TEST4_DATA_SZ);
          sendLen = write(clnt_sock, txdata, TEST4_DATA_SZ+MSG_HEADER_SZ); // org
          // sendLen = write(clnt_sock, txdata, SEND_TEST_DATA_LEN); // org
#else
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ); // young test
#endif
          syslog(LOG_INFO, "send test4 %d\n", sendLen);
        }

        // handle packet req change imsi
        else if(NULL != strstr(rxdata, REQ_CHG_IMSI))
        {
          syslog(LOG_INFO, "recv req change imsi %d\n", recvLen);
          memcpy(&txdata[0], RSP_SEND_IMSI, MSG_HEADER_SZ);
          memcpy(&txdata[MSG_HEADER_SZ], imsi_str, strlen(imsi_str));
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ+strlen(imsi_str));
          syslog(LOG_INFO, "send imsi %d\n", sendLen);
        }

        // handle packet complete imsi
        else if(NULL != strstr(rxdata, REQ_CMPLT_IMSI))
        {
          syslog(LOG_INFO, "recv complete imsi %d\n", recvLen);
          if(FINISH_AFTER_IMSI == finishMode)
          {
            sendLen = write(clnt_sock, RSP_FINISH, MSG_HEADER_SZ);
            syslog(LOG_INFO, "send finish %d\n", sendLen);
            close(clnt_sock);
            syslog(LOG_INFO, "------ Disconnect client %ld %d\n", inx, clnt_sock);
            break;
          }
          else
          {
            sendLen = write(clnt_sock, RSP_GO_AHEAD, MSG_HEADER_SZ);
            syslog(LOG_INFO, "send go ahead %d\n", sendLen);
          }
        }

        // handle packet req change msisdn
        else if(NULL != strstr(rxdata, REQ_CHG_MSISDN))
        {
          syslog(LOG_INFO, "recv req change msisdn %d\n", recvLen);
          memcpy(&txdata[0], RSP_SEND_MSISDN, MSG_HEADER_SZ);
          memcpy(&txdata[MSG_HEADER_SZ], msisdn_byte, MSISDN_BYTE_SZ);
          sendLen = write(clnt_sock, txdata, MSG_HEADER_SZ+MSISDN_BYTE_SZ);
          syslog(LOG_INFO, "send msisdn %d\n", sendLen);
        }

        // handle packet complete msisdn
        else if(NULL != strstr(rxdata, REQ_CMPLT_MSISDN))
        {
          syslog(LOG_INFO, "recv complete msisdn %d\n", recvLen);
          if(FINISH_AFTER_MSISDN == finishMode)
          {
            sendLen = write(clnt_sock, RSP_FINISH, MSG_HEADER_SZ);
            syslog(LOG_INFO, "send finish %d\n", sendLen);
            syslog(LOG_INFO, "sleep 1 s\n");
            sleep(1);
            close(clnt_sock);
            syslog(LOG_INFO, "------ Disconnect client %ld %d\n", inx, clnt_sock);
            break;
          }
        }

        else
        {
          syslog(LOG_INFO, "recv unknown cmd %d\n", recvLen);
          sendLen = write(clnt_sock, RSP_UNKNOWN, MSG_HEADER_SZ);
          syslog(LOG_INFO, "send rsp unkown %d\n", sendLen);
        } // end of handling recv packet

        // clear recv buffer
        memset(rxdata, 0, BUF_SIZE);

      } // end of read recv buffer
                  close(clnt_sock);
    } // end of handle conn request

  } // end of while(1)
  close(serv_sock);

  return 0;
} // end of main()

