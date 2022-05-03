/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "arm_math.h"
#include "ring_buffer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define FFT_SAMPLES 1024
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t AdcValue[FFT_SAMPLES];

float FFTInBuffer[FFT_SAMPLES];
float FFTOutBuffer[FFT_SAMPLES];

arm_rfft_fast_instance_f32 FFTHandler;

volatile uint8_t SamplesReady;

uint8_t OutFreqArray[10];

static uint16_t i = 0;
/* transfer variables - comment when not used */
/*
RingBuffer_t buf_t;
static uint8_t size;
static uint8_t chartmp[8];
static uint16_t tmp = 0;
*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
float complexABS(float real, float compl);
void CalculateFFT(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  if(HAL_OK != HAL_TIM_Base_Start(&htim2)) Error_Handler();
  if(HAL_OK != HAL_ADC_Start_DMA(&hadc1,(uint32_t*) AdcValue, FFT_SAMPLES) ) Error_Handler();
//  if(HAL_OK != HAL_UART_Transmit_DMA(&huart2,(uint8_t*) AdcValue, FFT_SAMPLES)) Error_Handler();

  arm_rfft_fast_init_f32(&FFTHandler, FFT_SAMPLES);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /* comment here to turn off data transmit */
	  /*
	  if(0 != RB_Read(&buf_t,&tmp)) Error_Handler();
	  size = sprintf((char*)chartmp, "%i\n", tmp);
	  HAL_UART_Transmit_DMA(&huart2,chartmp,size);
	  */

	  if(SamplesReady == 1)
	  {
		  SamplesReady = 0;

		  for(uint32_t i = 0; i < FFT_SAMPLES; i++)
		  {
			  FFTInBuffer[i] =  (float)AdcValue[i];
		  }

		  CalculateFFT();

	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	static uint16_t i = 0;

	if(hadc->Instance == ADC1)
	{
		if(HAL_OK != HAL_ADC_Stop_DMA(&hadc1)) Error_Handler();

		for (uint32_t i = 0; i < FFT_SAMPLES; i++)
		{
			FFTInBuffer[i] = (float) AdcValue[i];
		}

		CalculateFFT();
	}
}
*/

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	if(hadc->Instance == ADC1)
	{
		if(HAL_OK != HAL_UART_Transmit_DMA(&huart2,(uint8_t*) AdcValue, 512)) Error_Handler();
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if(hadc->Instance == ADC1)
	{
		if(HAL_OK != HAL_UART_Transmit_DMA(&huart2,(uint8_t*) AdcValue, 512)) Error_Handler();
//		AdcValue[i] = HAL_ADC_GetValue(&hadc1);

		/* ring buffer used to transmit data - comment when not used */
		/*
		if(0 != RB_Write(&buf_t,AdcValue[i])) Error_Handler();
		*/
//		i++;
//		HAL_GPIO_TogglePin(A2_GPIO_Port,A2_Pin); //output for oscilloscope

		if(HAL_OK != HAL_ADC_Start_DMA(&hadc1,(uint32_t*)AdcValue, FFT_SAMPLES) ) Error_Handler();

		if(i == FFT_SAMPLES)
		{
			i = 0;
			SamplesReady = 1;
		}
	}
}

float complexABS(float real,float compl){
	return sqrtf(real*real+compl*compl);
}

void CalculateFFT(void)
{
	arm_rfft_fast_f32(&FFTHandler, FFTInBuffer, FFTOutBuffer, 0);

	int Freqs[FFT_SAMPLES] = {0};
	int FreqPoint = 0;
	int Offset = 45; //variable noise floor offset

	//calculate abs values and linear-to-dB
	for (int i = 0; i < FFT_SAMPLES; i = i + 2)
	{

		Freqs[FreqPoint] = (int)(20*log10f(complexABS(FFTOutBuffer[i], FFTOutBuffer[i+1]))) - Offset;

		if(Freqs[FreqPoint] < 0)
		{
			Freqs[FreqPoint] = 0;
		}
		FreqPoint++;

	}
//  for 1024 samples

	OutFreqArray[0] = (uint8_t)Freqs[1]; // 22 Hz
	OutFreqArray[1] = (uint8_t)Freqs[2]; // 63 Hz
	OutFreqArray[2] = (uint8_t)Freqs[3]; // 125 Hz
	OutFreqArray[3] = (uint8_t)Freqs[6]; // 250 Hz
	OutFreqArray[4] = (uint8_t)Freqs[12]; // 500 Hz
	OutFreqArray[5] = (uint8_t)Freqs[23]; // 1000 Hz
	OutFreqArray[6] = (uint8_t)Freqs[51]; // 2200 Hz
	OutFreqArray[7] = (uint8_t)Freqs[104]; // 4500 Hz
	OutFreqArray[8] = (uint8_t)Freqs[207]; // 9000 Hz
	OutFreqArray[9] = (uint8_t)Freqs[344]; // 15000 Hz


//  for 2048 samples
/*
	OutFreqArray[0] = (uint8_t)Freqs[2]; // 22 Hz
	OutFreqArray[1] = (uint8_t)Freqs[3]; // 63 Hz
	OutFreqArray[2] = (uint8_t)Freqs[6]; // 125 Hz
	OutFreqArray[3] = (uint8_t)Freqs[12]; // 250 Hz
	OutFreqArray[4] = (uint8_t)Freqs[23]; // 500 Hz
	OutFreqArray[5] = (uint8_t)Freqs[46]; // 1000 Hz
	OutFreqArray[6] = (uint8_t)Freqs[101]; // 2200 Hz
	OutFreqArray[7] = (uint8_t)Freqs[207]; // 4500 Hz
	OutFreqArray[8] = (uint8_t)Freqs[413]; // 9000 Hz
	OutFreqArray[9] = (uint8_t)Freqs[688]; // 15000 Hz
*/

}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
