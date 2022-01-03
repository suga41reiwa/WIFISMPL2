#include "main.h"

// Wi-Fi設定
#define WIFI_SSID	"GP01-84A8E4A86E86"
#define WIFI_PASS	"69338"


// NTP設定
#define TIMEZONE	(+9)	//　日本のタイムゾーン。
#define NTP_SERVER		"ntp.nict.jp"	//　接続するNTPサーバー

//　FTP設定
/* LAN 内
#define FTP_HOSTNAME	"192.168.58.21"
#define FTP_USER		"ysugawara"
#define	FTP_PORT		21
#define FTP_PASS		"reiwa"
#define	FTP_CWD			"/IOT"

*/

#define FTP_HOSTNAME	"iot2020.starfree.jp"
#define FTP_USER		"iot2020.starfree.jp"
#define FTP_PASS		"7d82mbrk"
#define	FTP_CWD			"/iot"


#define	FILE_NAME		"PRESSLOG.CSV"

// printfで表示するときにはここの#define のコメントを外す
//#define COMM_MONITOR


//
#define	FTP_UPDATE_INTEVAL	3600	//FTP　UPDATE interval (Sec ) *** 1hour interval
//#define	FTP_UPDATE_INTEVAL	60	//FTP　UPDATE interval (Sec ) *** 60Sec interval for debug

static char tmpstr[256];


T_SYS tSys;

typedef struct{
	char sndbuf[128];
	char rcvbuf[128];
	uint32_t rcvp;

	uint16_t pasv_port;
	char pasv_ipadd[64];

	char ftp_host_message[128];

	char sndfile[128];	//1ファイルの大きさ。　NULL含めて128バイトまで
	uint16_t  sfilelen;


	//　時間情報
	int update_flg;
	char filename[20];
	char datatime_str[30];
	uint32_t time_sec;
	uint16_t year;
	
}T_WIFI;

T_WIFI t_wifi;

// timeout
#define WIFI_TMO 1000
#define WIFI_TMO_NET_RESP	(WIFI_TMO*10)


T_UART_MAN tUart1;
#define RX1BUF_SZ 128
static char rx1buf[RX1BUF_SZ];
#define TX1BUF_SZ 128
static char tx1buf[TX1BUF_SZ];

T_UART_MAN tUart2;
#define RX2BUF_SZ 32
static char rx2buf[RX2BUF_SZ];
#define TX2BUF_SZ 128
static char tx2buf[TX2BUF_SZ];


// printf
int _write(int file, char *ptr, int len){
	UART_nputs(&tUart2,ptr, (uint32_t)len, 1000);
	return len;
}



const char *s_OK = "OK";
const char *s_WIFI_CONNECTED = "WIFI CONNECTED";
const char *s_WIFI_GOT_IP = "WIFI GOT IP";
const char *s_0CONNECT = "0,CONNECT";
const char *s_1CONNECT = "1,CONNECT";
const char *s_1CLOSED = "1,CLOSED";
const char *s_SEND_OK= "SEND OK";
const char *s_pIPD = "+IPD,";
const char *s_cipsntptime = "+CIPSNTPTIME:";


static char *cnv_float(char *s , float f  );
static int put_line( char *str ,uint32_t timout);
static int rcv_line( uint32_t timeout );
static void rcv_clr( void );
static int wait_response( const char *normal_ans ,uint32_t tmo);
static int wroom_wifi_connect_seq( void );
static int wroom_wifi_update(void);

static int wroom_powon_seq( void );
static int wroom_wifi_get_time( void );
static int wroom_ftp_connect_seq( void );
static int wroom_ftp_upload(char *top,uint32_t fsize);
static int wroom_ftp_quit( void );




/*
 *
 *
 */
static void apl_init( void )
{
	HAL_StatusTypeDef halstat;
	UART_init();
	memset(&tSys,0,sizeof(tSys));
	tUart1.phuart = &huart1;
	tUart1.rxbuftop = rx1buf;
	tUart1.rxbuf_sz = sizeof(rx1buf);
	tUart1.txbuftop = tx1buf;
	tUart1.txbuf_sz = sizeof(tx1buf);
	UART_create(&tUart1);

	tUart2.phuart = &huart2;
	tUart2.rxbuftop = rx2buf;
	tUart2.rxbuf_sz = sizeof(rx2buf);
	tUart2.txbuftop = tx2buf;
	tUart2.txbuf_sz = sizeof(tx2buf);

	UART_create(&tUart2);
	halstat = LPS25HB_init();
	if(halstat != HAL_OK){
		tSys.errflg = 1;
	}else{
		tSys.errflg = 0;
	}
	t_wifi.update_flg= 0;
}


/*
 * 前回のタイミングより
 *
 */
int is_upload_timming(uint32_t now,uint32_t bak)
{
	int flg = 0;
	if(t_wifi.update_flg == 0 ){
		if( (now == 0) && (now != bak ) ){
			flg = 1;
		}else if(now > bak ) {
			flg = 1;
		}
	}
	t_wifi.update_flg = flg;
	return flg;
}

/*
 *  送信ファイルの内容を作成する。
 */
int make_file_str( char *ftpstr)
{
	int len;
	char spress[10];
	char stempa[10];

	cnv_float(spress,tSys.press);
	cnv_float(stempa,tSys.tempa);

	strcpy(ftpstr,t_wifi.datatime_str);
	strcat(ftpstr,",");
	strcat(ftpstr,spress);
	strcat(ftpstr,",");
	strcat(ftpstr,stempa);
	strcat(ftpstr,"\r\n");
	len = strlen(ftpstr);
	return len;
}


/*
 * アプリメイン
 *
 */
void apl_main( void )
{
	int ret;
	int timout;
	HAL_StatusTypeDef hstat;


	apl_init();

	printf("AIR PRESSURE FTP DEMO V1.00\r\n");

// first set
	for(;;){

		WIFI_EN_HI;
		WIFI_MODE_HI;
		WIFI_RST_LO;
		HAL_Delay(1);
		WIFI_RST_HI;

		ret = wroom_powon_seq();
		if( ret!= WIFI_OK){
			printf("error powon seq\r\n");
		}

		if( ret == WIFI_OK ){
			ret = wroom_wifi_connect_seq();
			if( ret!= WIFI_OK){
				printf("error connect_seq\r\n");
			}
		}

	// ◆◆◆◆　ESP-WROOM2 のファームウェアアップデートが必要な場合はコメントを外してアップデートを実行
	// ret = wroom_wifi_update();

		timout = 0;
		if( ret == WIFI_OK ){

			for(;;){
				ret = wroom_wifi_get_time();
				if(ret == WIFI_OK){
					timout++;
					if(t_wifi.year>1970){
						tSys.ftpupdate_cnt = (FTP_UPDATE_INTEVAL -(t_wifi.time_sec %  FTP_UPDATE_INTEVAL)) % FTP_UPDATE_INTEVAL;
						tSys.ftpupdate_cnt_bak = tSys.ftpupdate_cnt;
						break;
					}
					if(timout>120){
						break;
					}
				}
				HAL_Delay(500);
			}

			while( ret == WIFI_OK ){
				ret = wroom_wifi_get_time();
				if(ret == WIFI_OK){
					if(t_wifi.year>1970){
						tSys.ftpupdate_cnt =  (FTP_UPDATE_INTEVAL -(t_wifi.time_sec %  FTP_UPDATE_INTEVAL)) % FTP_UPDATE_INTEVAL;

			// デバッグ表示 タイミング確認
			// printf("%s %6ld:%6ld\r\n",t_wifi.datatime_str, tSys.ftpupdate_cnt,tSys.ftpupdate_cnt_bak);

						if( is_upload_timming(tSys.ftpupdate_cnt,tSys.ftpupdate_cnt_bak) ){
							hstat = LPS25HB_get_val(&tSys.press,&tSys.tempa);
							if(hstat!=HAL_OK){

								tSys.press = 0;		// 取得エラーの場合はこの値固定
								tSys.tempa = -100.0;// 取得エラーの場合はこの値固定
							}

							t_wifi.sfilelen = make_file_str(t_wifi.sndfile);
							ret = wroom_ftp_connect_seq();
							if(ret == WIFI_OK){
								ret = wroom_ftp_upload(t_wifi.sndfile,t_wifi.sfilelen);
							}
							if(ret == WIFI_OK){
								ret = wroom_ftp_quit();
							}
						}
						tSys.ftpupdate_cnt_bak = tSys.ftpupdate_cnt;
					}
				}else{
					printf("get time error\r\n");
				}
				HAL_Delay(500);
			}

		}
	}
}

/*
 * 1行出力
 */
static int put_line( char *str ,uint32_t timout)
{
	int ret;

	BDLED_ON;
	strcpy(t_wifi.sndbuf,str);
	strcat(t_wifi.sndbuf,"\r\n");
	ret = UART_puts(&tUart1,t_wifi.sndbuf, 1000);

#ifdef COMM_MONITOR
	printf(t_wifi.sndbuf);
#endif
	return ret;
}


/*
 *　FTP ファイル出力
 */
static int put_file( char *str ,uint16_t len,uint32_t timout)
{
	int ret;
	BDLED_ON;
	memcpy(t_wifi.sndbuf,str,len);
	ret = UART_nputs(&tUart1,t_wifi.sndbuf,len, timout);

#ifdef COMM_MONITOR
	printf(t_wifi.sndbuf);
#endif

	if(ret==UART_OK ) ret = WIFI_OK;
	else		ret = WIFI_ERR;
	//	puts_fifo(t_wifi.sndbuf);	//モニター出力はしない
	return ret;
}


/*
 * 1行受信
 */
static int rcv_line( uint32_t timeout )
{
	char ch;
	int ret=WIFI_ERR; // ret..1 OK 1line recieved ,0..timeout
	uint32_t crlfcnt = 0;
	t_wifi.rcvp = 0;
	t_wifi.rcvbuf[t_wifi.rcvp] = 0;
	do{
		HAL_Delay(1);

		while(  UART_rcv(&tUart1,&ch)==HAL_OK   ){
			BDLED_TGL;
			if( t_wifi.rcvp < (sizeof(t_wifi.rcvbuf)-1) ){
				t_wifi.rcvbuf[t_wifi.rcvp++] = ch;
			}
			switch(ch){
			case 0x0d:
					crlfcnt= 1;
					break;
			case 0x0a:
					if(crlfcnt==1){
						crlfcnt = 2;
						t_wifi.rcvbuf[t_wifi.rcvp] = 0;//null
#ifdef COMM_MONITOR
			printf(t_wifi.rcvbuf);
#endif

						ret = WIFI_OK;
					}
				break;
			default:
				crlfcnt = 0;
			}

			if( ret == WIFI_OK) break;	// crlfを受信した。
		}
		timeout--;
	}while(timeout && (crlfcnt!= 2) );
	if( (timeout == 0  )&& t_wifi.rcvp ){
		t_wifi.rcvbuf[t_wifi.rcvp] = 0;//null

	}
	BDLED_OFF;
	return ret;
}

/*
 * 受信バッファをクリアする。
 */
static void rcv_clr( void )
{
	UART_rcv_clr(&tUart1);
}


/*
 * レスポンス待ち
 * 	 const char *normal_ans :期待受信データ
 * 	 uint32_t tmo :
 */
static int wait_response( const char *normal_ans ,uint32_t tmo)
{
	int ret = WIFI_ERR;
	int done =0;
	while( done == 0 ){
		if( rcv_line( tmo ) == WIFI_OK ){
			if( strncmp(t_wifi.rcvbuf,normal_ans,strlen(normal_ans))==0){
				ret = WIFI_OK;
				done = 1;
			}else{
			}
		}else{
			ret = WIFI_ERR;
			done = 1;
		}
		HAL_Delay(1);
	}
	return ret;
}


/*
 * wroom 電源ONシーケンス
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int wroom_powon_seq( void )
{
	int ret = WIFI_ERR;
	HAL_Delay(2000);
	rcv_clr();
	put_line("ATE0",1000);	// ECHO OFF
	ret = wait_response( s_OK ,WIFI_TMO );

	if( ret == WIFI_OK){	// get Version
		rcv_clr();
		put_line("AT+GMR",1000);
		ret = wait_response( s_OK ,WIFI_TMO );
	}

	if( ret == WIFI_OK){	//
		HAL_Delay(300);
		rcv_clr();
		put_line("AT+CWMODE_CUR=1",1000);
		ret = wait_response( s_OK ,WIFI_TMO );
	}

	if( ret == WIFI_OK){
		HAL_Delay(300);
		rcv_clr();
		rcv_clr();
		put_line("AT+CIPMUX=1",1000);
		ret = wait_response( s_OK ,WIFI_TMO );
	}
	BDLED_OFF;

	return ret;

}


/*
 * wroom のファームウェアアップデート
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int wroom_wifi_update(void)
{
	int ret = WIFI_OK;
	rcv_clr();
	put_line("AT+CIUPDATE",WIFI_TMO);
	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	for(;;){
		HAL_Delay(1000);
	}
	return ret;
}



/*
 * wroom wifi に接続する。
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int wroom_wifi_connect_seq( void )
{
	int ret = WIFI_OK;
	rcv_clr();
	sprintf(tmpstr,"AT+CWJAP=\"%s\",\"%s\"",WIFI_SSID ,WIFI_PASS);
	put_line(tmpstr,WIFI_TMO);

	ret = wait_response( s_WIFI_CONNECTED ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	ret = wait_response( s_WIFI_GOT_IP ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );

	if(ret != WIFI_OK){
		goto endoffunc;
	}

/*
 * STPサーバーの設定
 */
 	sprintf(tmpstr,"AT+CIPSNTPCFG=1,%d,\"%s\"",TIMEZONE,NTP_SERVER);
	put_line(tmpstr,WIFI_TMO);
	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
endoffunc :
	return ret;

}



/*
 * cipsend
 * 	uint32_t  cipno	:
 * 	char *str   	:
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int cipsend(uint32_t  cipno,char *str )
{
	int len;
	int ret;
	rcv_clr();
	len = strlen(str);
	sprintf(tmpstr,"AT+CIPSEND=%ld,%d",cipno,len+2 ); // +2 .. crlf
	put_line(tmpstr,WIFI_TMO);
	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	if( ret != WIFI_OK ){
		goto err;
	}
	if(ret==WIFI_OK){
		HAL_Delay(100);
		strcpy(tmpstr,str);
		rcv_clr();
		put_line(str,WIFI_TMO);

		do{
			ret = rcv_line( WIFI_TMO_NET_RESP );
			if( ret == WIFI_OK ){
				if( strncmp(t_wifi.rcvbuf,s_SEND_OK,strlen(s_SEND_OK))==0){
					break;
				}
			}else{

			}
		}while(ret == WIFI_OK );
	}
	err:
	return ret;
}


/*
 * FTPサーバーに接続処理
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int wroom_ftp_connect_seq( void )
{
	int ret = WIFI_OK;
	char str[80];
	rcv_clr();
	sprintf(tmpstr,"AT+CIPSTART=0,\"TCP\",\"%s\",21",FTP_HOSTNAME );
	put_line(tmpstr,WIFI_TMO);

	ret = wait_response( s_0CONNECT ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	 ret = rcv_line( WIFI_TMO );
	if( ret == WIFI_OK ){
		strcpy(t_wifi.ftp_host_message,t_wifi.rcvbuf);
	}else{
		strcpy(t_wifi.ftp_host_message,"");
	}

	HAL_Delay(100);
	sprintf( str,"USER %s",FTP_USER);
	ret = cipsend(0,str);
	if(ret != WIFI_OK ){
			goto endoffunc;
	}
	HAL_Delay(200);
	sprintf( str,"PASS %s",FTP_PASS);
	ret = cipsend(0,str);
	if(ret != WIFI_OK ){
			goto endoffunc;
	}

	HAL_Delay(200);


	sprintf( str,"CWD %s",FTP_CWD);
	ret = cipsend(0,str);
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	HAL_Delay(200);


endoffunc :
	return ret;

}

/*
 *　　PASVモードで　ポート番号を受け取る
 *　　　 uint16_t *portno
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
#define RESPCODE_227_ENTERRING_PASSIVE_MODE	227
static int rcv_ftp_portno( char *ipadd,uint16_t *portno )
{
	int ret;
	int resp;
	int done = 0;
	uint16_t respcode;
	int num[6];
	char *p;
	do{
		ret = rcv_line( WIFI_TMO_NET_RESP );
		if(ret==WIFI_OK){
			p = strchr(t_wifi.rcvbuf,':');

			if( p ){
				p++;
				respcode = atoi( p);
				if(respcode  == RESPCODE_227_ENTERRING_PASSIVE_MODE ){
					p = strchr(t_wifi.rcvbuf,'(');
					p++;
					resp = sscanf(p,"%d,%d,%d,%d,%d,%d",&num[0],&num[1],&num[2],&num[3],&num[4],&num[5] );
					if(resp ==6 ){
						*portno = num[4]*256+num[5];
						sprintf(ipadd,"%d.%d.%d.%d",num[0],num[1],num[2],num[3]);
						ret = WIFI_OK;
						done = 1;
					}else{
						ret = WIFI_ERR;
					}
				}else{
					ret = WIFI_ERR;
				}
			}else{
				ret = WIFI_ERR;
			}
		}else{
			done = 1;
		}
	}while(done==0);
	return ret;
}


/*
 *　uint32_t  cipno:
 *　char *str : 送信file
 * uint16_t len:
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int cipsendfile(uint32_t  cipno,char *str ,uint16_t len )
{
	int sndlen;
	int ret;
	int tmout;
	rcv_clr();
	sprintf(tmpstr,"AT+CIPSEND=%ld,%d",cipno,len ); // +2 .. crlf
	put_line(tmpstr,WIFI_TMO);

	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	if( ret != WIFI_OK ){
		goto err;
	}

	if(ret==WIFI_OK){
		while(len){
			if( len > sizeof(t_wifi.sndbuf)){
				sndlen = sizeof(t_wifi.sndbuf);
			}else{
				sndlen = len;
			}
			rcv_clr();
			put_file(str,sndlen,WIFI_TMO);
			HAL_Delay(sndlen/10+10);

			len -= sndlen;
			str += sndlen;
			tmout = 100;
			while(UART_isSending(&tUart1)){// 信するのでぱっふぁつまりに注意
				HAL_Delay(10);
				tmout--;
				if(tmout==0){
					goto err;
				}
			}
		}
		do{
			ret = rcv_line( WIFI_TMO_NET_RESP );
			if( ret == WIFI_OK ){
				if( strncmp(t_wifi.rcvbuf,s_SEND_OK,strlen(s_SEND_OK))==0){
					break;
				}
			}
		}while(ret == WIFI_OK );
	}
	err:
	return ret;
}

/*
 *　ftpサーバーにファイルをアップロードする。
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int  wroom_ftp_upload( char *top,uint32_t fsize)
{
	int ret = WIFI_OK;
	char str[40];
	HAL_Delay(100);
	rcv_clr();
	ret = cipsend(0,"PASV");
	if(ret != WIFI_OK ){
		goto endoffunc;
	}
	ret = rcv_ftp_portno(t_wifi.pasv_ipadd,&t_wifi.pasv_port);
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	rcv_clr();
	sprintf(tmpstr,"AT+CIPSTART=1,\"TCP\",\"%s\",%d",t_wifi.pasv_ipadd,t_wifi.pasv_port);
	put_line(tmpstr,WIFI_TMO);
	ret = wait_response( s_1CONNECT ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	HAL_Delay(100);

	sprintf(str,"APPE %s",FILE_NAME);
	ret = cipsend(0,str);
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	HAL_Delay(100);

	while(fsize){
		uint32_t locallen;
		HAL_Delay(1);
		if(fsize >= 200){
			locallen = 200;
		}else{
			locallen = fsize;
		}

		ret = cipsendfile(1,top,locallen);
		if(ret != WIFI_OK ){
			goto endoffunc;
		}
		top+=locallen;
		fsize-= locallen;
	}

	rcv_clr();
	put_line("AT+CIPCLOSE=1",WIFI_TMO);
	ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	if(ret != WIFI_OK ){
		goto endoffunc;
	}

	endoffunc :
	return ret;
}



/*
 *　ftpサーバー切断
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int wroom_ftp_quit( void )
{
	int ret = WIFI_OK;
	ret = cipsend(0,"QUIT");

	return ret;
}



/*
以下のような応答が来る。
+CIPSNTPTIME:Thu Aug 04 14:48:05 2016

このなかからsntp_strには　日時情報文字列　"Aug 04 14:48:05 2016"
time_sec には時刻情報を0:0:0からの経過秒を代入する。0 - 86399
	例 14:48:05 であれば
		14*60*60 + 48*60 + 05 = 50400 + 2880 + 05 = 53285

*/
static int rcv_sntptime( char *sntp_str, uint32_t *time_sec ,uint16_t *year)
{
	int ret;
	int resp;
	int done = 0;
	int hh,mm,ss,yy;
	hh = mm = ss = yy = 0;
	do{
		ret = rcv_line( WIFI_TMO_NET_RESP );
		if(ret == WIFI_OK){
			if( memcmp(t_wifi.rcvbuf,s_cipsntptime,strlen(s_cipsntptime)) == 0 ){
				memcpy(sntp_str,&t_wifi.rcvbuf[17],20);
				sntp_str[20] = 0;
/*
				hh = atoi(&t_wifi.rcvbuf[24]);
				mm = atoi(&t_wifi.rcvbuf[27]);
				ss = atoi(&t_wifi.rcvbuf[30]);
				yy = atoi(&t_wifi.rcvbuf[33]);
*/
				resp = sscanf(&t_wifi.rcvbuf[23],"%d:%d:%d %d",&hh,&mm,&ss,&yy);
				if(resp == 4){
					*time_sec = hh*60*60 + mm*60 + ss;
					*year = yy;
					done = 1;
 				}
			}
		}else{
			done = 1;// time out
		}
	}while(done==0);

	return ret;
}


/*
 *　wroom に時刻データを取得する。
 *
 * 戻り値:WIFI_ERR/WIFI_OK
 */
static int wroom_wifi_get_time( void )
{
	int ret = WIFI_OK;
	rcv_clr();
	put_line("AT+CIPSNTPTIME?",WIFI_TMO);
	ret = rcv_sntptime(t_wifi.datatime_str, &t_wifi.time_sec ,&t_wifi.year);

	if(ret == WIFI_OK){
		ret = wait_response( s_OK ,WIFI_TMO_NET_RESP );
	}
	return ret;
}






/*
 *　float を文字列に変換　表示範囲は -9999.0～9999.9まで。
 *　char *s: 変換した文字列を代入する文字列のポインタ
 *　float f: 変換する数値
 *
 * 戻り値:　char s
 */
static char *cnv_float(char *s , float f  )
{
	int icnv;
	int sign = 1;
	if( f<0 ){
		sign= -1;
		f = -f;
	}
	icnv = f*10;

	char *p = &s[7];

	*p--= 0;
	*p-- = '0' + icnv %10; icnv/=10;
	*p-- = '.';
	for(int j=0;j<4;j++){
		*p-- = '0' + icnv %10; icnv/=10;
	}
	if(sign==-1) *p = '-';
	else		*p = ' ';

	return s;
}




