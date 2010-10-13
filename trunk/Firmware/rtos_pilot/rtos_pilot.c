/*! 
 *  The main file of the Gluonpilot's FreeRTOS autopilot.
 *
 *  The functionality here is pretty limited:
 *   - Initialize primairy hardware
 *   - Load configuration from dataflash
 *   - Start all the tasks
 *
 *  @file     rtos_pilot.c
 *  @author   Tom Pycke
 *  @date     24-dec-2009
 *  @since    0.1
 */
 
#include <stdio.h>

// FreeRTOS includes
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/croutine.h"
#include "FreeRTOS/semphr.h"

// Gluonpilot library includes
#include "microcontroller/microcontroller.h"
#include "uart1_queue/uart1_queue.h"
#include "dataflash/dataflash.h"
#include "pwm_in/pwm_in.h"
#include "ppm_in/ppm_in.h"

// rtos_pilot includes
#include "control.h"
#include "sensors.h"
#include "communication.h"
#include "configuration.h"
#include "datalogger.h"
#include "navigation.h"

extern xSemaphoreHandle xGpsSemaphore;
extern xSemaphoreHandle xSpiSemaphore;


int main()
{
	microcontroller_init();
	
	uart1_queue_init(115200l);  // default baudrate: 115200
	
	printf("Gluonpilot v0.5 [%s %s, config: %d bytes, logline: %d bytes, navigation: %d bytes, double: %d bytes]\r\n", __DATE__, __TIME__, sizeof(struct Configuration), sizeof(struct LogLine), sizeof(navigation_data.navigation_codes), sizeof(double));

	microcontroller_reset_type();  // for debugging
	led_init();
	
	// create semaphores needed for FreeRTOS synchronization (better to do it know, they are changed in interrupts of uart2 and ppm)
	vSemaphoreCreateBinary( xSpiSemaphore );
	vSemaphoreCreateBinary( xGpsSemaphore );
	
	dataflash_open();
	printf("%d MB flash found\r\n", (int)PAGE_SIZE/264);
	uart1_puts("Loading configuration...");
	configuration_load();
	uart1_puts("done\r\n");
	
	// Open RC receiver input: pwm_in/ppm_in task: in ppm_in/pwm_in.c
	// This is too low level to do it in the control task
	if (config.control.use_pwm)
		pwm_in_open(); 
	else
	{
		uart1_puts("Opening ppm...");
		ppm_in_open(); // We need a complete frame (which takes at least 20ms) to start so never can start early enough!
		uart1_puts(" done\r\n");
	}	
	

	// Create our tasks. 
	xTaskCreate( control_task,                 ( signed portCHAR * ) "Control",      ( configMINIMAL_STACK_SIZE * 3 ), NULL, tskIDLE_PRIORITY + 6, NULL );
	//uart1_puts("Control task started\r\n");
	xTaskCreate( sensors_task,                 ( signed portCHAR * ) "Sensors",      ( configMINIMAL_STACK_SIZE * 5 ), NULL, tskIDLE_PRIORITY + 5, NULL );
	//uart1_puts("Sensors task started\r\n");
	xTaskCreate( sensors_gps_task,             ( signed portCHAR * ) "GpsNavi",          ( configMINIMAL_STACK_SIZE * 5 ), NULL, tskIDLE_PRIORITY + 4, NULL );
	//uart1_puts("Sensors_GPS task started\r\n");
	xTaskCreate( communication_input_task,     ( signed portCHAR * ) "ConsoleInput", ( configMINIMAL_STACK_SIZE * 3 ), NULL, tskIDLE_PRIORITY + 3, NULL );
	//uart1_puts("Communications task started\r\n");
	xTaskCreate( datalogger_task,              ( signed portCHAR * ) "Dataflash",    ( configMINIMAL_STACK_SIZE * 3 ), NULL, tskIDLE_PRIORITY + 2, NULL );
	//uart1_puts("communication_telemetry task started\r\n");
	xTaskCreate( communication_telemetry_task, ( signed portCHAR * ) "Telemetry",    ( configMINIMAL_STACK_SIZE * 2 ), NULL, tskIDLE_PRIORITY + 1, NULL );

	//uart1_puts("Starting scheduler\r\n");
	// Order the scheduler to start scheduling our two tasks.
	vTaskStartScheduler();
	
	// We should only get here when the scheduler wasn't able to allocate enough memory.
	uart1_puts("Not enough heap!\r\n");
	
	return 1;
}


void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName )
{
	uart1_puts("Stack overflow! ");
	uart1_puts((char*)pcTaskName);
	uart1_puts("\n\r");
	while(1) ; 
}


void vApplicationIdleHook( void )
{
}
