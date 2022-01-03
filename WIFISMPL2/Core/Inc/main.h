/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uart_drv.h"
#include "lps25hb.h"
#include "apl.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOF
#define VCP_TX_Pin GPIO_PIN_2
#define VCP_TX_GPIO_Port GPIOA
#define WIFI_EN_Pin GPIO_PIN_4
#define WIFI_EN_GPIO_Port GPIOA
#define WIFI_MODE_Pin GPIO_PIN_7
#define WIFI_MODE_GPIO_Port GPIOA
#define WIFI_RST_Pin GPIO_PIN_11
#define WIFI_RST_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_15
#define VCP_RX_GPIO_Port GPIOA
#define BDLED_Pin GPIO_PIN_3
#define BDLED_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#define WIFI_EN_LO HAL_GPIO_WritePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin, GPIO_PIN_RESET);
#define WIFI_EN_HI HAL_GPIO_WritePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin, GPIO_PIN_SET);
#define WIFI_EN_TGL HAL_GPIO_TogglePin(WIFI_EN_GPIO_Port, WIFI_EN_Pin);

#define WIFI_MODE_LO HAL_GPIO_WritePin(WIFI_MODE_GPIO_Port, WIFI_MODE_Pin, GPIO_PIN_RESET);
#define WIFI_MODE_HI HAL_GPIO_WritePin(WIFI_MODE_GPIO_Port, WIFI_MODE_Pin, GPIO_PIN_SET);
#define WIFI_MODE_TGL HAL_GPIO_TogglePin(WIFI_MODE_GPIO_Port, WIFI_MODE_Pin);

#define WIFI_RST_LO HAL_GPIO_WritePin(WIFI_RST_GPIO_Port, WIFI_RST_Pin, GPIO_PIN_RESET);
#define WIFI_RST_HI HAL_GPIO_WritePin(WIFI_RST_GPIO_Port, WIFI_RST_Pin, GPIO_PIN_SET);
#define WIFI_RST_TGL HAL_GPIO_TogglePin(WIFI_RST_GPIO_Port, WIFI_RST_Pin);

#define BDLED_OFF HAL_GPIO_WritePin(BDLED_GPIO_Port, BDLED_Pin, GPIO_PIN_RESET);
#define BDLED_ON HAL_GPIO_WritePin(BDLED_GPIO_Port, BDLED_Pin, GPIO_PIN_SET);
#define BDLED_TGL HAL_GPIO_TogglePin(BDLED_GPIO_Port, BDLED_Pin);


extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
