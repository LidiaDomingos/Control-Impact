/************************************************************************
* 5 semestre - Eng. da Computao - Insper
*
* 2021 - Exemplo com HC05 com RTOS
*
*/

#include <asf.h>
#include "conf_board.h"
#include <string.h>
#include <math.h>

/************************************************************************/
/* defines                                                              */
/************************************************************************/

// LEDs
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)

#define LED_PIO_HAND      PIOD
#define LED_PIO_ID_HAND   ID_PIOD
#define LED_IDX_HAND      22
#define LED_IDX_MASK_HAND (1 << LED_IDX_HAND)

#define LED_PIO_BLUE      PIOD
#define LED_PIO_ID_BLUE   ID_PIOD
#define LED_IDX_BLUE      26
#define LED_IDX_MASK_BLUE (1 << LED_IDX_BLUE)

#define LED_PIO_RED      PIOA
#define LED_PIO_ID_RED   ID_PIOA
#define LED_IDX_RED      24
#define LED_IDX_MASK_RED (1 << LED_IDX_RED)

// Botões azuis da esquerda
#define BUT_PIO_UP            	PIOD
#define BUT_PIO_ID_UP         	ID_PIOD
#define BUT_IDX_UP            	30
#define BUT_IDX_MASK_UP       	(1 << BUT_IDX_UP)

#define BUT_PIO_DOWN          	PIOA
#define BUT_PIO_ID_DOWN       	ID_PIOA
#define BUT_IDX_DOWN          	6
#define BUT_IDX_MASK_DOWN     	(1 << BUT_IDX_DOWN)

#define BUT_PIO_LEFT          	PIOC
#define BUT_PIO_ID_LEFT       	ID_PIOC
#define BUT_IDX_LEFT          	19
#define BUT_IDX_MASK_LEFT     	(1 << BUT_IDX_LEFT)

#define BUT_PIO_RIGHT	        PIOA
#define BUT_PIO_ID_RIGHT        ID_PIOA
#define BUT_IDX_RIGHT           2
#define BUT_IDX_MASK_RIGHT      (1 << BUT_IDX_RIGHT)

// Botôes coloridos da direita

#define BUT_PIO_RED            	PIOA
#define BUT_PIO_ID_RED         	ID_PIOA
#define BUT_IDX_RED            	3
#define BUT_IDX_MASK_RED       	(1 << BUT_IDX_RED)

#define BUT_PIO_YELLOW         	PIOA
#define BUT_PIO_ID_YELLOW      	ID_PIOA
#define BUT_IDX_YELLOW         	21
#define BUT_IDX_MASK_YELLOW     (1 << BUT_IDX_YELLOW)

#define BUT_PIO_GREEN          	PIOC
#define BUT_PIO_ID_GREEN       	ID_PIOC
#define BUT_IDX_GREEN          	13
#define BUT_IDX_MASK_GREEN     	(1 << BUT_IDX_GREEN)

#define BUT_PIO_BLUE          	PIOD
#define BUT_PIO_ID_BLUE       	ID_PIOD
#define BUT_IDX_BLUE          	11
#define BUT_IDX_MASK_BLUE     	(1 << BUT_IDX_BLUE)

// DEFINICÃO ANALÓGICO ESQUERDO
#define AFEC_VRX_LEFT			AFEC1
#define AFEC_VRX_ID_LEFT		ID_AFEC1
#define AFEC_VRX_CHANNEL_LEFT	5 // Canal do pino PC30

#define AFEC_VRY_LEFT			AFEC1
#define AFEC_VRY_ID_LEFT		ID_AFEC1
#define AFEC_VRY_CHANNEL_LEFT	6 // Canal do pino PC31

// DEFINICÃO ANALÓGICO DIREITA
#define AFEC_VRX_RIGHT			AFEC0
#define AFEC_VRX_ID_RIGHT		ID_AFEC0
#define AFEC_VRX_CHANNEL_RIGHT	5 // Canal do pino PB2

#define AFEC_VRY_RIGHT			AFEC0
#define AFEC_VRY_ID_RIGHT		ID_AFEC0
#define AFEC_VRY_CHANNEL_RIGHT	8 // Canal do pino PA19


// usart (bluetooth ou serial)
// Descomente para enviar dados
// pela serial debug

// #define DEBUG_SERIAL

#ifdef  DEBUG_SERIAL
#define USART_COM USART1
#define USART_COM_ID ID_USART1
#else
#define USART_COM USART0
#define USART_COM_ID ID_USART0
#endif

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_BLUETOOTH_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_BLUETOOTH_STACK_PRIORITY        (tskIDLE_PRIORITY)

QueueHandle_t xQueue_UNICA;

TaskHandle_t xHandshake = NULL;
TaskHandle_t xBluetooth = NULL;

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

void task_bluetooth(void);
void task_handshake(void);

/************************************************************************/
/* STRUCT                                                            */
/************************************************************************/
typedef struct {
	volatile char id;
	volatile char button;
	volatile int value;
} Data;
/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************

/************************************************************************/
/* RTOS application HOOK                                                */
/************************************************************************/

/* Called if stack overflow during execution */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	* identify which task has overflowed its stack.
	*/
	for (;;) {
	}
}

/* This function is called by FreeRTOS idle task */
extern void vApplicationIdleHook(void) {
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

/* This function is called by FreeRTOS each tick */
extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/
void but_callback_UP(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	volatile Data data;
	data.id = '1';
	data.value = 0;
	if(pio_get(BUT_PIO_UP, PIO_INPUT, BUT_IDX_MASK_UP) == 0) {
		printf("entrei no high");
		data.button = '1';
 	} 
	else {
		printf("entrei no low");
		data.button = 'A';
	}
	printf("mande pra fila");
 	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
	printf("mande pra fila2");
}

void but_callback_DOWN(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '2';
	data.value = 0;
	if(pio_get(BUT_PIO_DOWN, PIO_INPUT, BUT_IDX_MASK_DOWN) == 0) {
		data.button = '2';
	}
	else {
		data.button = 'B';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);

}

void but_callback_LEFT(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '3';
	data.value = 0;
	if(pio_get(BUT_PIO_LEFT, PIO_INPUT, BUT_IDX_MASK_LEFT) == 0) {
		data.button = '3';
	}
	else {
		data.button = 'C';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
}

void but_callback_RIGHT(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '4';
	data.value = 0;
	if(pio_get(BUT_PIO_RIGHT, PIO_INPUT, BUT_IDX_MASK_RIGHT) == 0) {
		data.button = '4';
	}
	else {
		data.button = 'D';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
}

void but_callback_RED(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '5';
	data.value = 0;
	if(pio_get(BUT_PIO_RED, PIO_INPUT, BUT_IDX_MASK_RED) == 0) {
		data.button = '5';
	}
	else {
		data.button = 'E';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
}

void but_callback_YELLOW(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '6';
	data.value = 0;
	if(pio_get(BUT_PIO_YELLOW, PIO_INPUT, BUT_IDX_MASK_YELLOW) == 0) {
		data.button = '6';
	}
	else {
		data.button = 'F';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
}

void but_callback_GREEN(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '7';
	data.value = 0;
	if(pio_get(BUT_PIO_GREEN, PIO_INPUT, BUT_IDX_MASK_GREEN) == 0) {
		data.button = '7';
	}
	else {
		data.button = 'G';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
}

void but_callback_BLUE(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	Data data;
	data.id = '8';
	data.value = 0;
	if(pio_get(BUT_PIO_BLUE, PIO_INPUT, BUT_IDX_MASK_BLUE) == 0) {
		data.button = '8';
	}
	else {
		data.button = 'H';
	}
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
}

void AFEC_VRX_callback_LEFT(void){
	volatile Data data1;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	data1.id = 'I';
	data1.button = 'Y';
	data1.value = afec_channel_get_value(AFEC_VRX_LEFT, AFEC_VRX_CHANNEL_LEFT);
	xQueueSendFromISR(xQueue_UNICA, &data1, &xHigherPriorityTaskWoken);
	
	afec_channel_enable(AFEC_VRY_LEFT, AFEC_VRY_CHANNEL_LEFT);
	afec_start_software_conversion(AFEC_VRY_LEFT);
}

void AFEC_VRY_callback_LEFT(void){
	volatile Data data2;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	data2.id = 'J';
	data2.button = 'Y';
	data2.value = afec_channel_get_value(AFEC_VRY_LEFT, AFEC_VRY_CHANNEL_LEFT);
	xQueueSendFromISR(xQueue_UNICA, &data2, &xHigherPriorityTaskWoken);
	
}

void AFEC_VRX_callback_RIGHT(void){
	Data data;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	data.id = 'K';
	data.button = 'Z';
	data.value = afec_channel_get_value(AFEC_VRX_RIGHT, AFEC_VRX_CHANNEL_RIGHT);
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);
	
	afec_channel_enable(AFEC_VRY_RIGHT, AFEC_VRY_CHANNEL_RIGHT);
	afec_start_software_conversion(AFEC_VRY_RIGHT);
	
	//float x = round(((data.value * (-65.25) - 255*65.25) /255)+65.25,2);
	//printf("x %d \n", (int) x);
}

void AFEC_VRY_callback_RIGHT(void){
	Data data;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	data.id = 'L';
	data.button = 'Z';
	data.value = afec_channel_get_value(AFEC_VRY_RIGHT, AFEC_VRY_CHANNEL_RIGHT);
	xQueueSendFromISR(xQueue_UNICA, &data, &xHigherPriorityTaskWoken);

}

/**
*  Interrupt handler for TC1 interrupt.
*/
void TC0_Handler(void){
	volatile uint32_t ul_dummy;

	/****************************************************************
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	******************************************************************/
	ul_dummy = tc_get_status(TC0, 0);
	printf("[Debug] TC0 IRQ \n");

	/* Avoid compiler warning */
	UNUSED(ul_dummy);
	
	afec_channel_enable(AFEC_VRX_LEFT, AFEC_VRX_CHANNEL_LEFT);
	afec_start_software_conversion(AFEC_VRX_LEFT);
	
	afec_channel_enable(AFEC_VRX_RIGHT, AFEC_VRX_CHANNEL_RIGHT);
	afec_start_software_conversion(AFEC_VRX_RIGHT);

}
/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void io_init(void){
	pmc_enable_periph_clk(LED_PIO_ID);
	pmc_enable_periph_clk(LED_PIO_ID_HAND);
	pmc_enable_periph_clk(LED_PIO_ID_BLUE);
	pmc_enable_periph_clk(LED_PIO_ID_RED);

	pmc_enable_periph_clk(BUT_PIO_ID_UP);
 	pmc_enable_periph_clk(BUT_PIO_ID_DOWN);
	pmc_enable_periph_clk(BUT_PIO_ID_LEFT);
	pmc_enable_periph_clk(BUT_PIO_ID_RIGHT);
	 
	pmc_enable_periph_clk(BUT_PIO_ID_RED);
	pmc_enable_periph_clk(BUT_PIO_ID_YELLOW);
	pmc_enable_periph_clk(BUT_PIO_ID_GREEN);
	pmc_enable_periph_clk(BUT_PIO_ID_BLUE);

	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT | PIO_DEBOUNCE);
	pio_set_output(LED_PIO_HAND, LED_IDX_MASK_HAND, 1, 0, 0);
	pio_set_output(LED_PIO_BLUE, LED_IDX_MASK_BLUE, 0, 0, 0);
	pio_set_output(LED_PIO_RED, LED_IDX_MASK_RED, 0, 0, 0);

	pio_configure(BUT_PIO_UP, PIO_INPUT, BUT_IDX_MASK_UP, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_UP, BUT_IDX_MASK_UP, 60);
 	pio_configure(BUT_PIO_DOWN, PIO_INPUT, BUT_IDX_MASK_DOWN, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_DOWN, BUT_IDX_MASK_DOWN, 60);
	pio_configure(BUT_PIO_LEFT, PIO_INPUT, BUT_IDX_MASK_LEFT, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_LEFT, BUT_IDX_MASK_LEFT, 60);	
	pio_configure(BUT_PIO_RIGHT, PIO_INPUT, BUT_IDX_MASK_RIGHT, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_RIGHT, BUT_IDX_MASK_RIGHT, 60);	

	pio_configure(BUT_PIO_RED, PIO_INPUT, BUT_IDX_MASK_RED, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_RED, BUT_IDX_MASK_RED, 60);	
	pio_configure(BUT_PIO_YELLOW, PIO_INPUT, BUT_IDX_MASK_YELLOW, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_YELLOW, BUT_IDX_MASK_YELLOW, 60);	
	pio_configure(BUT_PIO_GREEN, PIO_INPUT, BUT_IDX_MASK_GREEN, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_GREEN, BUT_IDX_MASK_GREEN, 60);
	pio_configure(BUT_PIO_BLUE, PIO_INPUT, BUT_IDX_MASK_BLUE, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_BLUE, BUT_IDX_MASK_BLUE, 60);
	
	pio_handler_set(BUT_PIO_UP,
					BUT_PIO_ID_UP,
					BUT_IDX_MASK_UP,
					PIO_IT_EDGE,
					but_callback_UP);
	
	pio_handler_set(BUT_PIO_DOWN,
					BUT_PIO_ID_DOWN,
					BUT_IDX_MASK_DOWN,
					PIO_IT_EDGE,
					but_callback_DOWN);
	
	pio_handler_set(BUT_PIO_LEFT,
					BUT_PIO_ID_LEFT,
					BUT_IDX_MASK_LEFT,
					PIO_IT_EDGE,
					but_callback_LEFT);
	
	pio_handler_set(BUT_PIO_RIGHT,
					BUT_PIO_ID_RIGHT,
					BUT_IDX_MASK_RIGHT,
					PIO_IT_EDGE,
					but_callback_RIGHT);

	pio_handler_set(BUT_PIO_RED,
					BUT_PIO_ID_RED,
					BUT_IDX_MASK_RED,
					PIO_IT_EDGE,
					but_callback_RED);

	pio_handler_set(BUT_PIO_YELLOW,
					BUT_PIO_ID_YELLOW,
					BUT_IDX_MASK_YELLOW,
					PIO_IT_EDGE,
					but_callback_YELLOW);

	pio_handler_set(BUT_PIO_GREEN,
					BUT_PIO_ID_GREEN,
					BUT_IDX_MASK_GREEN,
					PIO_IT_EDGE,
					but_callback_GREEN);
	
	pio_handler_set(BUT_PIO_BLUE,
					BUT_PIO_ID_BLUE,
					BUT_IDX_MASK_BLUE,
					PIO_IT_EDGE,
					but_callback_BLUE);

	pio_enable_interrupt(BUT_PIO_UP, BUT_IDX_MASK_UP);
 	pio_enable_interrupt(BUT_PIO_DOWN, BUT_IDX_MASK_DOWN);
	pio_enable_interrupt(BUT_PIO_LEFT, BUT_IDX_MASK_LEFT);
	pio_enable_interrupt(BUT_PIO_RIGHT, BUT_IDX_MASK_RIGHT);

	pio_enable_interrupt(BUT_PIO_RED, BUT_IDX_MASK_RED);
	pio_enable_interrupt(BUT_PIO_YELLOW, BUT_IDX_MASK_YELLOW);
	pio_enable_interrupt(BUT_PIO_GREEN, BUT_IDX_MASK_GREEN);
	pio_enable_interrupt(BUT_PIO_BLUE, BUT_IDX_MASK_BLUE);
	
	pio_get_interrupt_status(BUT_PIO_UP);
 	pio_get_interrupt_status(BUT_PIO_DOWN);
	pio_get_interrupt_status(BUT_PIO_LEFT);
	pio_get_interrupt_status(BUT_PIO_RIGHT);

	pio_get_interrupt_status(BUT_PIO_RED);
	pio_get_interrupt_status(BUT_PIO_YELLOW);
	pio_get_interrupt_status(BUT_PIO_GREEN);
	pio_get_interrupt_status(BUT_PIO_BLUE);

	NVIC_EnableIRQ(BUT_PIO_ID_UP);
 	NVIC_EnableIRQ(BUT_PIO_ID_DOWN);
	NVIC_EnableIRQ(BUT_PIO_ID_LEFT);
	NVIC_EnableIRQ(BUT_PIO_ID_RIGHT);

	NVIC_EnableIRQ(BUT_PIO_ID_RED);
	NVIC_EnableIRQ(BUT_PIO_ID_YELLOW);
	NVIC_EnableIRQ(BUT_PIO_ID_GREEN);
	NVIC_EnableIRQ(BUT_PIO_ID_BLUE);
	
	NVIC_SetPriority(BUT_PIO_ID_UP, 4);
 	NVIC_SetPriority(BUT_PIO_ID_DOWN, 4);
	NVIC_SetPriority(BUT_PIO_ID_LEFT, 4);
	NVIC_SetPriority(BUT_PIO_ID_RIGHT, 4);

	NVIC_SetPriority(BUT_PIO_ID_RED, 4);
	NVIC_SetPriority(BUT_PIO_ID_YELLOW, 4);
	NVIC_SetPriority(BUT_PIO_ID_GREEN, 4);
	NVIC_SetPriority(BUT_PIO_ID_BLUE, 4);
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		#if (defined CONF_UART_CHAR_LENGTH)
		.charlength = CONF_UART_CHAR_LENGTH,
		#endif
		.paritytype = CONF_UART_PARITY,
		#if (defined CONF_UART_STOP_BITS)
		.stopbits = CONF_UART_STOP_BITS,
		#endif
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
	setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	* emits one character at a time.
	*/
	#endif
}

uint32_t usart_puts(uint8_t *pstring) {
	uint32_t i ;

	while(*(pstring + i))
	if(uart_is_tx_empty(USART_COM))
	usart_serial_putchar(USART_COM, *(pstring+i++));
}

void usart_put_string(Usart *usart, char str[]) {
	usart_serial_write_packet(usart, str, strlen(str));
}

int usart_get_string(Usart *usart, char buffer[], int bufferlen, uint timeout_ms) {
	uint timecounter = timeout_ms;
	uint32_t rx;
	uint32_t counter = 0;

	while( (timecounter > 0) && (counter < bufferlen - 1)) {
		if(usart_read(usart, &rx) == 0) {
			buffer[counter++] = rx;
		}
		else{
			timecounter--;
			vTaskDelay(1);
		}
	}
	buffer[counter] = 0x00;
	return counter;
}

void usart_send_command(Usart *usart, char buffer_rx[], int bufferlen,
char buffer_tx[], int timeout) {
	usart_put_string(usart, buffer_tx);
	usart_get_string(usart, buffer_rx, bufferlen, timeout);
}

void config_usart0(void) {
	sysclk_enable_peripheral_clock(ID_USART0);
	usart_serial_options_t config;
	config.baudrate = 9600;
	config.charlength = US_MR_CHRL_8_BIT;
	config.paritytype = US_MR_PAR_NO;
	config.stopbits = false;
	usart_serial_init(USART0, &config);
	usart_enable_tx(USART0);
	usart_enable_rx(USART0);

	// RX - PB0  TX - PB1
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 0), PIO_DEFAULT);
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 1), PIO_DEFAULT);
}

int hc05_init(void) {
	char buffer_rx[128];
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+NAMEControlImpact", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+PIN0000", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "X", 100);
}

static void config_AFEC(void){
/*************************************
   * Ativa e configura AFEC
   *************************************/
  /* Ativa AFEC - 1 */
	afec_enable(AFEC1);
	afec_enable(AFEC0);

	/* struct de configuracao do AFEC */
	struct afec_config afec_cfg;

	/* Carrega parametros padrao */
	afec_get_config_defaults(&afec_cfg);

	/* Configura AFEC */
	afec_init(AFEC_VRX_LEFT, &afec_cfg);
	afec_init(AFEC_VRY_LEFT, &afec_cfg);
	afec_init(AFEC_VRX_RIGHT, &afec_cfg);
	afec_init(AFEC_VRY_RIGHT, &afec_cfg);

	/*** Configuracao específica do canal AFEC ***/
	struct afec_ch_config afec_ch_cfg;
	afec_ch_get_config_defaults(&afec_ch_cfg);
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;
	afec_ch_set_config(AFEC_VRX_LEFT, AFEC_VRX_CHANNEL_LEFT, &afec_ch_cfg);
	afec_ch_set_config(AFEC_VRY_LEFT, AFEC_VRY_CHANNEL_LEFT, &afec_ch_cfg);
	afec_ch_set_config(AFEC_VRX_RIGHT, AFEC_VRX_CHANNEL_RIGHT, &afec_ch_cfg);
	afec_ch_set_config(AFEC_VRY_RIGHT, AFEC_VRY_CHANNEL_RIGHT, &afec_ch_cfg);

	/*
	* Calibracao:
	* Because the internal ADC offset is 0x200, it should cancel it and shift
	 down to 0.
	 */
	afec_channel_set_analog_offset(AFEC_VRX_LEFT, AFEC_VRX_CHANNEL_LEFT, 0x200);
	afec_channel_set_analog_offset(AFEC_VRY_LEFT, AFEC_VRY_CHANNEL_LEFT, 0x200);
	afec_channel_set_analog_offset(AFEC_VRX_RIGHT, AFEC_VRX_CHANNEL_RIGHT, 0x200);
	afec_channel_set_analog_offset(AFEC_VRY_RIGHT, AFEC_VRY_CHANNEL_RIGHT, 0x200);

	/* configura IRQ */
	afec_set_callback(AFEC_VRX_LEFT, AFEC_VRX_CHANNEL_LEFT, AFEC_VRX_callback_LEFT, 1);
	afec_set_callback(AFEC_VRY_LEFT, AFEC_VRY_CHANNEL_LEFT, AFEC_VRY_callback_LEFT, 1);
	afec_set_callback(AFEC_VRX_RIGHT, AFEC_VRX_CHANNEL_RIGHT, AFEC_VRX_callback_RIGHT, 1);
	afec_set_callback(AFEC_VRY_RIGHT, AFEC_VRY_CHANNEL_RIGHT, AFEC_VRY_callback_RIGHT, 1);
	
	NVIC_SetPriority(AFEC_VRX_ID_LEFT, 4);
	NVIC_EnableIRQ(AFEC_VRX_ID_LEFT);
	NVIC_SetPriority(AFEC_VRY_ID_LEFT, 4);
	NVIC_EnableIRQ(AFEC_VRY_ID_LEFT);
	
	NVIC_SetPriority(AFEC_VRX_ID_RIGHT, 4);
	NVIC_EnableIRQ(AFEC_VRX_ID_RIGHT);
	NVIC_SetPriority(AFEC_VRY_ID_RIGHT, 4);
	NVIC_EnableIRQ(AFEC_VRY_ID_RIGHT);
	
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	uint32_t channel = 1;

	/* Configura o PMC */
	/* O TimerCounter é meio confuso
	o uC possui 3 TCs, cada TC possui 3 canais
	TC0 : ID_TC0, ID_TC1, ID_TC2
	TC1 : ID_TC3, ID_TC4, ID_TC5
	TC2 : ID_TC6, ID_TC7, ID_TC8
	*/
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  4Mhz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);

	tc_init(TC, TC_CHANNEL, ul_tcclks
							| TC_CMR_WAVE /* Waveform mode is enabled */
							| TC_CMR_ACPA_SET /* RA Compare Effect: set */
							| TC_CMR_ACPC_CLEAR /* RC Compare Effect: clear */
							| TC_CMR_CPCTRG /* UP mode with automatic trigger on RC Compare */
	);

	tc_write_ra(TC, TC_CHANNEL, 2*65532/3);
	tc_write_rc(TC, TC_CHANNEL, 3*65532/3);

	//tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura e ativa interrupçcão no TC canal 0 */
	/* Interrupção no C */
#ifdef DEBUG
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
#endif
	/* Inicializa o canal 0 do TC */
	tc_start(TC, TC_CHANNEL);
}

void send_package(Data data){
	char eof = 'X';
	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	usart_write(USART_COM, data.id);

	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	usart_write(USART_COM, data.button);

	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	usart_write(USART_COM, data.value);

	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	
	usart_write(USART_COM, data.value >> 8);

	while(!usart_is_tx_ready(USART_COM)) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	usart_write(USART_COM, eof);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/
void task_handshake(void) {
	printf("Task HandShake started \n");
	
	printf("Inicializando HC05 \n");
	config_usart0();
	hc05_init();
	
	// configura LEDs e Botões
	io_init();
	
	Data data;
	char eof = 'X';

	data.id = 'W';
	data.button = 'W';
	data.value = 0;
	send_package(data);
	char resposta = 0;
	
	while(1) {
		if (usart_read(USART_COM, &resposta) == 0) {
			printf("rx = %c \n", resposta);
			if (resposta == 'w') {
				printf("resposta == %c \n", resposta);
								/* Create task to send data from bluetooth */
				xTaskCreate(task_bluetooth, "BLT", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, &xBluetooth);
			}
		}
		else {
			printf("entrei no else");
			send_package(data);
		}
		
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void task_bluetooth(void) {
	printf("Task Bluetooth started \n");
	vTaskDelete(xHandshake);
	/* iniciliza botao */
	config_AFEC();
	TC_init(TC0, ID_TC0, 0, 3);
	tc_start(TC0, 0);
	// configura LEDs e Botões
	pio_clear(LED_PIO_HAND, LED_IDX_MASK_HAND);

	Data data;

	char eof = 'X';
	int valor_anterior_x = 0;
	int valor_anterior_y = 0;
	int valor_anterior_rx = 0;
	int valor_anterior_ry = 0;
	
	// Task não deve retornar.
	while(1) {
		if (xQueueReceive(xQueue_UNICA, &data, 0)) {
			if (data.value == 0){	
				printf("data id (  %c   )", data.id);
				if (data.button == '1' || data.button == '2' || data.button == '3' || data.button == '4'){
					pio_set(LED_PIO_BLUE, LED_IDX_MASK_BLUE);
					printf("liga");	
				}
				else if (data.button == 'A' || data.button == 'B' || data.button == 'C' || data.button == 'D') {
					pio_clear(LED_PIO_BLUE, LED_IDX_MASK_BLUE);	
					printf("desliga");
				}
				if (data.button == '5' || data.button == '6' || data.button == '7' || data.button == '8'){
					pio_set(LED_PIO_RED, LED_IDX_MASK_RED);
					printf("liga");
				}
				else if (data.button == 'E' || data.button == 'F' || data.button == 'G' || data.button == 'H') {
					pio_clear(LED_PIO_RED, LED_IDX_MASK_RED);
					printf("desliga");
				}
				send_package(data);
			}
			
			if (data.button == 'Y'){
				if (data.id == 'I'){
					if((data.value > valor_anterior_x + 300) || (data.value < valor_anterior_x - 300)){
						send_package(data);
						valor_anterior_x = data.value;
					}
				}
				if (data.id == 'J'){
					if ((data.value > valor_anterior_y + 300) || (data.value < valor_anterior_y - 300)){
						send_package(data);
						valor_anterior_y = data.value;
					}
				}
			}
			
			if (data.button == 'Z'){
				if (data.id == 'K'){
					if((data.value > valor_anterior_rx + 300) || (data.value < valor_anterior_rx - 300)){
						send_package(data);
						valor_anterior_rx = data.value;
					}
				}
				if (data.id == 'L'){
					if ((data.value > valor_anterior_ry + 300) || (data.value < valor_anterior_ry - 300)){
						send_package(data);
						valor_anterior_ry = data.value;
					}
				}
			}
		}
		}
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/

int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();
	configure_console();
	
	xQueue_UNICA = xQueueCreate(32, sizeof(Data));
	
	if (xQueue_UNICA == NULL){
		printf("falha em criar a queue \n");
	}
	/* Create task to make led blink */
	if (xTaskCreate(task_handshake, "HSK", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, &xHandshake) != pdPASS) {
		printf("Erro ao criar task handshake \n");
	}

	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}