#include "main.h"

#include "uart_drv.h"
#include "string.h"

//
#define UARTMAN_SZ	2
T_UART_MAN *uartman[UARTMAN_SZ];
uint32_t uartmanp ;


void UART_init( void )
{
	__disable_irq();
	for(int i = 0;i<UARTMAN_SZ;i++ ){
		uartman[i] = NULL;
	}
	uartmanp = 0;
	__enable_irq();
}

void UART_create( T_UART_MAN *ptuartman )
{
	__disable_irq();
	uartman[uartmanp] = ptuartman;
	uartmanp++;
	ptuartman->flg_snd = 0;
	ptuartman->rxp = 0;
	HAL_UART_Receive_IT(ptuartman->phuart,(uint8_t*)ptuartman->rxbuftop,ptuartman->rxbuf_sz);
	__enable_irq();
}


void UART_rcv_clr( T_UART_MAN *ptuartman )
{
	char dummy;
	int cnt=0;
	while(UART_rcv(ptuartman,&dummy)==UART_OK){
		cnt++;
		if(cnt >= ptuartman->rxbuf_sz){
			// error;
			break;
		}
	}

}

uint8_t UART_rcv( T_UART_MAN *ptuartman,char *ch )
{
	uint8_t ret;
	if( (ptuartman->rxbuf_sz-ptuartman->rxp) != ptuartman->phuart->RxXferCount){
		*ch = ptuartman->rxbuftop[ptuartman->rxp];
		ptuartman->rxp++;
		if(ptuartman->rxp >= ptuartman->rxbuf_sz){
			ptuartman->rxp = 0;
		}
		ret = UART_OK;
	}else{
		ret = UART_ERR;
	}
	return ret;
}



/*
 * 文字列の送信
 * char *str 送信文字列
 *
 */
uint8_t UART_puts( T_UART_MAN *ptuartman,char * str ,uint32_t timeout)
{
	uint8_t ret;
	uint32_t  len;
	len = strlen(str);
	if(len){
		ret =  UART_nputs( ptuartman,str , len ,timeout);
	}else{
		ret = UART_OK;
	}
	return ret;
}
/*
 * 指定文字数の送信
 * uint8_t *data  送信文字数 (バイト)
 * uint32_t len
 *
 */
int errcnt = 0;
uint8_t UART_nputs( T_UART_MAN *ptuartman,char *data , uint32_t len ,uint32_t timeout)
{
	HAL_StatusTypeDef halstat;
	uint8_t ret = UART_ERR;
	if(timeout){
		while( ptuartman->flg_snd ){
#ifdef _CMSIS_OS_H
			osDelay(1);
#else
			HAL_Delay(1);
#endif
			timeout--;
			if(timeout==0){
				ret = UART_OK;
				goto endoffunc;
			}
		}
	}else{
		if(ptuartman->flg_snd){
			ret = UART_OK;
			goto endoffunc;
		}
	}
	memcpy( ptuartman->txbuftop,data,len);
	__disable_irq();
	halstat = HAL_UART_Transmit_IT( ptuartman->phuart,(uint8_t*)ptuartman->txbuftop,len);
	if(halstat == HAL_OK){
		ptuartman->flg_snd = 1;
	}else{
		errcnt++;
	}
	__enable_irq();
endoffunc:
	return ret;
}

uint8_t UART_isSending(  T_UART_MAN *ptuartman )
{
	return 		ptuartman->flg_snd ;
}



/**
  * @brief  Rx Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

  /* Prevent unused argument(s) compilation warning */
	for(int i=0;i<uartmanp;i++){
		if(huart == uartman[i]->phuart ){
			HAL_UART_Receive_IT(huart,(uint8_t*)uartman[i]->rxbuftop, uartman[i]->rxbuf_sz );
			break;	//一度ヒットしたらループから抜ける
		}
	}
}

/**
  * @brief  Tx Half Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	for(int i=0;i<uartmanp;i++){
		if(huart == uartman[i]->phuart ){
			uartman[i]->flg_snd = 0;
			break;	//一度ヒットしたらループから抜ける
		}


	}

}
