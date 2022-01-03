#ifndef __LPS25HB_H
#define __LPS25HB_H


HAL_StatusTypeDef LPS25HB_init( void );
HAL_StatusTypeDef LPS25HB_get_val( float * press, float *temp );




#endif /* __LPS25HB_H */
