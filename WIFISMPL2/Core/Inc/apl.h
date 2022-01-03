#ifndef __APL_MAIN_H
#define __APL_MAIN_H


/*
 * Ver1.0.1 リセット後の待ち時間を 2秒から4秒に変更
 *
 *
 */



#define WIFI_OK		0
#define WIFI_ERR	-1


typedef struct{
	float press;
	float tempa;
	int errflg;

	uint32_t ftpupdate_cnt;	// 0-
	uint32_t ftpupdate_cnt_bak;	// 0-
}T_SYS;

extern T_SYS tSys;
extern	T_UART_MAN tUart1;
extern T_UART_MAN tUart2;

void apl_main( void );

#endif /* __APL_MAIN_H */
