#include "main.h"


// LPS25HBのアドレス　SA0 は　GNDに接続
#define LPS25HB_ADR	((0x5C<<1)| 0x00)


#define REGADR_WHOAMI 0x0f
#define REGADR_CTRL_REG1 0x20
#define REGADR_PRESS_XL (0x28 | 0x80)


uint8_t who;
HAL_StatusTypeDef LPS25HB_init( void )
{
	uint8_t regs[1];
	HAL_StatusTypeDef ret;


	ret = HAL_I2C_Mem_Read(&hi2c1 , LPS25HB_ADR, REGADR_WHOAMI, 1, regs, 1,1000);
	who = regs[0];
	// who ... 0xbd

	if(ret == HAL_OK ){
		regs[0] = 0x90;
		ret = HAL_I2C_Mem_Write(&hi2c1,LPS25HB_ADR,REGADR_CTRL_REG1,1,regs,1,1000);
	}
	return ret;
}



HAL_StatusTypeDef LPS25HB_get_val( float * press, float *temp )
{
	float f;
	int errcnt=0;
	uint32_t prs;
	int16_t itmp;
	uint8_t regs1[5];
	HAL_StatusTypeDef ret;
	do{
		ret = HAL_I2C_Mem_Read(&hi2c1  , LPS25HB_ADR, REGADR_PRESS_XL, 1, regs1, 5,1000);
		HAL_Delay(1);
		errcnt++;
	}while( (ret != HAL_OK ) || (errcnt < 2 ));


	if(ret == HAL_OK ){
		// pressure
		prs = regs1[2];
		prs <<= 8;
		prs |= regs1[1];
		prs <<= 8;
		prs |= regs1[0];
		f = prs;
		*press = f/4096.0;

		//temp
		itmp  = regs1[4];
		itmp <<= 8;
		itmp |= regs1[3];
		f = itmp;
		*temp = 42.5 + f/480;
	}
	return ret;

}
