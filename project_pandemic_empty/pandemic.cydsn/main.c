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
#define DISPLAY_TASK_NAME ("display_task")
#define DISPLAY_STATE_TASK_NAME ("display_state_task")
#define CONTAMINATION_TASK_NAME ("contamination_task")

#define GAME_PRIORITY (20)
#define CONTAMINATION_PRIORITY (19)
#define CLUES_PRIORITY (10)
#define MEDICINE_PRIORITY (9)
#define DISPLAY_STATE_PRIORITY (2)
#define DISPLAY_PRIORITY (1)

#define DISPLAY_PERIOD (50)             // in ms  
#define DISPLAY_STATE_PERIOD (3000)       // in periods of display (3s per state)

/* Number of pills required in stock before allocating lab to vaccine */
#define PILLS_REQUIRED (0)

/* Mutex for lab, which is a shared resource*/
SemaphoreHandle_t labMutex;
/* Semaphore to synchronize the clue processing task with the release of the clue
 * Avoids the situation where a new clue is released before the previous one is processed
 * by the lab, invaliding the medicine research.
 */
SemaphoreHandle_t clueSync;
/* Semaphore to trigger contamination task action */
SemaphoreHandle_t contaminationFlag;

/* Represents the current active clue
 * It is a shared ressource without the need of a mutex (no critical section)
 */
Token currentClue = 0;

/*
* Represents the current information being display in the LCD.
*/
typedef enum {
    DISPLAY_POP,
    DISPLAY_VAC,
    DISPLAY_MED
} DISPLAY_STATE;

DISPLAY_STATE display_state = DISPLAY_POP;

/* Task functions */
void cluesTask(void* args);
void medicineTask(void* args);
void displayTask(void* args);
void displayStateTask(void* args);
void contaminationTask(void* args);

/* Task handlers */
TaskHandle_t gameHandler;
TaskHandle_t cluesHandler;
TaskHandle_t medicineHandler;
TaskHandle_t displayHandler;
TaskHandle_t displayStateHandler;
TaskHandle_t contaminationHandler;

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
    contaminationFlag = xSemaphoreCreateBinary();

    // Create tasks
    xTaskCreate( gameTask, GAME_TASK_NAME, TASK_STACK_SIZE, NULL, GAME_PRIORITY, &gameHandler );
    xTaskCreate( cluesTask, CLUES_TASK_NAME, TASK_STACK_SIZE, NULL, CLUES_PRIORITY, &cluesHandler );
    xTaskCreate( medicineTask, MEDICINE_TASK_NAME, TASK_STACK_SIZE, NULL, MEDICINE_PRIORITY, &medicineHandler );
    xTaskCreate( displayTask, DISPLAY_TASK_NAME, TASK_STACK_SIZE, NULL, DISPLAY_PRIORITY, &displayHandler );
    xTaskCreate( displayStateTask, DISPLAY_STATE_TASK_NAME, TASK_STACK_SIZE, NULL, DISPLAY_STATE_PRIORITY, &displayStateHandler );
    xTaskCreate( contaminationTask, CONTAMINATION_TASK_NAME, TASK_STACK_SIZE, NULL, CONTAMINATION_PRIORITY, &contaminationHandler );
    
    // Launch freeRTOS
    vTaskStartScheduler();     
    
    for(;;){}
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
    // release the flag so contamination task can execute.
    xSemaphoreGive(contaminationFlag);
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
void cluesTask(void *args){

    for(;;){
        xSemaphoreTake(clueSync, portMAX_DELAY); // Wait for a new clue
        
        //Check if there are enough pills to start vaccine production
        if(getMedicineCntr() < PILLS_REQUIRED) continue;

        // Assign the lab to the vaccine research
        xSemaphoreTake(labMutex, portMAX_DELAY);
        shipVaccine(assignMissionToLab(currentClue));
        xSemaphoreGive(labMutex);
    }

}

/*
 * Used to schedule medicine production.
 */
void medicineTask(void *args){
    for(;;){
        // Assign the lab to the medicine production
        xSemaphoreTake(labMutex, portMAX_DELAY);
        shipMedicine(assignMissionToLab(0));
        xSemaphoreGive(labMutex);
    }
}

/*
*   Manages the display states
*/
void displayStateTask(void* args){
    // STATE TRANSITION: POP -> VAC -> MED
    // Every 3s the state is changed
   
    for (;;){ 
        if (display_state == DISPLAY_POP){
            display_state = DISPLAY_VAC;
        }
        else if (display_state == DISPLAY_VAC){
            display_state = DISPLAY_MED;   
        }
        else{
            display_state = DISPLAY_POP;   
        }
        // Logic to ensure periodicity
        vTaskDelay(DISPLAY_STATE_PERIOD - (xTaskGetTickCount() % DISPLAY_STATE_PERIOD));
    }
}

/*
*   Every 50ms, it displays one information of the left side of
*   the LCD screen for 3s each.
*/
void displayTask(void* args){
    for (;;){
        LCD_Position(0u, 0u);
        LCD_ClearDisplay();
        if (display_state == DISPLAY_POP){
            LCD_PrintString("POP: ");
            LCD_PrintInt8(getPopulationCntr());
        }
        else if (display_state == DISPLAY_VAC){
            LCD_PrintString("VAC: ");
            LCD_PrintInt8(getVaccineCntr());
        }
        else{
            LCD_PrintString("MED: ");
            LCD_PrintInt8(getMedicineCntr());
        }
        // Logic to ensure periodicity
        vTaskDelay(DISPLAY_PERIOD - (xTaskGetTickCount()%DISPLAY_PERIOD));
    }
}

/*
*   Whenever the semaphore is realesed, put the population into quarantine.
*/
void contaminationTask(void* args){
    for (;;){
        xSemaphoreTake(contaminationFlag, portMAX_DELAY);
        quarantine();
    }   
}
/* [] END OF FILE */
