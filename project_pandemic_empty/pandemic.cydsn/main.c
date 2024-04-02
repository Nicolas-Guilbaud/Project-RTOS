/* ========================================
 *
 *              ELECH410 labs
 *          FreeRTOS pandemic project
 * 
 *                   2020
 *
 * ========================================
*/
#include "project.h"

/* RTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"

#include "pandemic.h"

#define TASK_STACK_SIZE (1024)

/* Task definitions */
#define GAME_TASK_NAME ("game_task")
#define CLUES_TASK_NAME ("clues_task")
#define MEDICINE_TASK_NAME ("medicine_task")

#define GAME_PRIORITY (20)
#define CLUES_PRIORITY (10)
#define MEDICINE_PRIORITY (9)

/* Number of pills required in stock before allocating lab to vaccine */
#define PILLS_REQUIRED (0)

/* Mutex for lab, which is a shared resource*/
SemaphoreHandle_t labMutex;
/* Semaphore to synchronize the clue processing task with the release of the clue
 * Avoids the situation where a new clue is released before the previous one is processed
 * by the lab, invaliding the medicine research.
 */
SemaphoreHandle_t clueSync;

/* Represents the current active clue
 * It is a shared ressource without the need of a mutex (no critical section)
 */
Token currentClue = 0;

/* Task functions */
void cluesTask( void * args);
void medicineTask( void * args);

/* Task handlers */
TaskHandle_t gameHandler;
TaskHandle_t cluesHandler;
TaskHandle_t medicineHandler;

/*
 * Installs the RTOS interrupt handlers.
 */
static void freeRTOSInit( void );

int main(void)
{
    /* Enable global interrupts. */
    CyGlobalIntEnable; 
    
    freeRTOSInit();
    
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    LCD_Start();
    KB_Start();
    
    /* Create mutexes */
    labMutex = xSemaphoreCreateMutex();
    clueSync = xSemaphoreCreateBinary();

    // Create tasks
    xTaskCreate( gameTask, GAME_TASK_NAME, TASK_STACK_SIZE, NULL, GAME_PRIORITY, &gameHandler );
    xTaskCreate( cluesTask, CLUES_TASK_NAME, TASK_STACK_SIZE, NULL, CLUES_PRIORITY, &cluesHandler );
    xTaskCreate( medicineTask, MEDICINE_TASK_NAME, TASK_STACK_SIZE, NULL, MEDICINE_PRIORITY, &medicineHandler );
    
    // Launch freeRTOS
    vTaskStartScheduler();     
    
    for(;;)
    {
    }
}

void freeRTOSInit( void )
{
    /* Port layer functions that need to be copied into the vector table. */
    extern void xPortPendSVHandler( void );
    extern void xPortSysTickHandler( void );
    extern void vPortSVCHandler( void );
    extern cyisraddress CyRamVectors[];

	/* Install the OS Interrupt Handlers. */
	CyRamVectors[ 11 ] = ( cyisraddress ) vPortSVCHandler;
	CyRamVectors[ 14 ] = ( cyisraddress ) xPortPendSVHandler;
	CyRamVectors[ 15 ] = ( cyisraddress ) xPortSysTickHandler;
}

/*
 * When a contamination occurs gameTask calls this function.
 * 
 */
void releaseContamination( void ){
    /* TODO Complete this */
}

/*
 * When gameTask releases a vaccine clue it calls this function.
 * 
 */
void releaseClue( Token clue ){
    currentClue = clue;
    xSemaphoreGive(clueSync);
}


/*
 * Used to schedule vaccine production.
 */
void cluesTask( void * args){

    for(;;){
        xSemaphoreTake(clueSync, portMAX_DELAY); // Wait for a new clue
        
        //Check if there are enough pills to start vaccine production
        if(getMedicineCntr() < PILLS_REQUIRED) continue;

        // Assign the lab to the vaccine research
        xSemaphoreTake(labSemaphore, portMAX_DELAY);
        shipVaccine( assignMissionToLab(currentClue) );
        xSemaphoreGive(labSemaphore);
    }

}

/*
 * Used to schedule medicine production.
 */
void medicineTask( void * args){
    for(;;){
        // Assign the lab to the medicine production
        xSemaphoreTake(labSemaphore, portMAX_DELAY);
        shipMedicine( assignMissionToLab(0) );
        xSemaphoreGive(labSemaphore);
    }
}


/* [] END OF FILE */
