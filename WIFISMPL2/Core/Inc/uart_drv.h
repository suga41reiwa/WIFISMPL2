/*
 * uart2_drv.h
 *
 *  Created on: 2020/07/05
 *      Author: user
 */

#ifndef SRC_UART_DRV_H_
#define SRC_UART_DRV_H_


#define UART_OK	0
#define UART_ERR	1

typedef struct {
	UART_HandleTypeDef *phuart;

	char *rxbuftop;
	char *txbuftop;

	uint32_t rxbuf_sz;
	uint32_t txbuf_sz;

	uint32_t rxp;
	volatile	uint8_t flg_snd;
}T_UART_MAN;


void UART_init( void );
void UART_create( T_UART_MAN *ptuartman );
void UART_rcv_clr( T_UART_MAN *ptuartman );
uint8_t UART_rcv( T_UART_MAN *ptuartman,char *ch );
uint8_t UART_puts( T_UART_MAN *ptuartman,char * str ,uint32_t timeout);
uint8_t UART_nputs( T_UART_MAN *ptuartman,char *data , uint32_t len ,uint32_t timeout);
uint8_t UART_isSending( T_UART_MAN *ptuartman);


#endif /* SRC_UART2_DRV_H_ */
