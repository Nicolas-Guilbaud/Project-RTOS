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

/* Priorities */
#define GAME_PRIORITY           (20)
#define CONTAMINATION_PRIORITY  (19)
#define CLUES_PRIORITY          (15)
#define MEDICINE_PRIORITY       (10)
#define DISPLAY_PRIORITY        (18)

/* TASK PERIODS */
#define DISPLAY_PERIOD          (500)         // in ms  

/* GPIOJ macro */
#define READ(name) GPIOJ1##name##_##Read()
#define WRITE(name,v) GPIOJ1##name##_##Write(v)

/* Period correction to take into account potential tardiness */
#define CORRECT_PERIOD(p) p - (xTaskGetTickCount()%p)

/* Condition used to check if the game is still running */
#define IS_GAME_RUNNING (getPopulationCntr() > 0 && getVaccineCntr() < 100)

/* Mutex for lab, which is a shared resource*/
SemaphoreHandle_t labMutex;
/* Semaphore to trigger contamination task action */
SemaphoreHandle_t contaminationFlag;

/* Queue used to pass the clue token */
#define QUEUE_SIZE 1
#define TOKEN_SIZE sizeof(Token)
QueueHandle_t queue;

/* Task functions */
void cluesTask(void* args);
void medicineTask(void* args);
void displayTask(void* args);
void contaminationTask(void* args);

/* Task handlers */
TaskHandle_t gameHandler;
TaskHandle_t cluesHandler;
TaskHandle_t medicineHandler;
TaskHandle_t displayHandler;
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
    contaminationFlag = xSemaphoreCreateBinary();
    
    // Create tasks
    xTaskCreate( gameTask, GAME_TASK_NAME, TASK_STACK_SIZE, NULL, GAME_PRIORITY, &gameHandler );
    xTaskCreate( cluesTask, CLUES_TASK_NAME, TASK_STACK_SIZE, NULL, CLUES_PRIORITY, &cluesHandler );
    xTaskCreate( medicineTask, MEDICINE_TASK_NAME, TASK_STACK_SIZE, NULL, MEDICINE_PRIORITY, &medicineHandler );
    xTaskCreate( displayTask, DISPLAY_TASK_NAME, TASK_STACK_SIZE, NULL, DISPLAY_PRIORITY, &displayHandler );
    xTaskCreate( contaminationTask, CONTAMINATION_TASK_NAME, TASK_STACK_SIZE, NULL, CONTAMINATION_PRIORITY, &contaminationHandler );
    
    queue = xQueueCreate(QUEUE_SIZE,TOKEN_SIZE);  
    
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
    //Send clue to queue
    xQueueSendToBack(queue,&clue,portMAX_DELAY);
}


/*
 * Used to schedule vaccine production.
 */
void cluesTask(void *args){
    
    (void) args;

    while(IS_GAME_RUNNING){
        
        WRITE(3,1);
        
        Token clue;
        
        //Wait for a new clue
        xQueueReceive(queue,&clue,portMAX_DELAY);
        
        //Use lab to produce vaccine
        xSemaphoreTake(labMutex, portMAX_DELAY);
        Token res = assignMissionToLab(clue);
        xSemaphoreGive(labMutex);
        
        shipVaccine(res);
        
        WRITE(3,0);
    }
    vTaskSuspend(cluesHandler);

}

/*
 * Used to schedule medicine production.
 */
void medicineTask(void *args){
    
    (void) args;
    
    // Assign the lab to the medicine production
    
    while(IS_GAME_RUNNING){
        
        WRITE(2,1);
        
        //Use lab to produce medicine pills
        xSemaphoreTake(labMutex, portMAX_DELAY);
        Token res = assignMissionToLab(0);
        xSemaphoreGive(labMutex);
        
        shipMedicine(res);
        
        WRITE(2,0);
    }
    vTaskSuspend(medicineHandler);
}

/*
*   Every 50ms, it displays one information of the left side of
*   the LCD screen.
*/
void displayTask(void* args){
    
    (void) args;
    while (IS_GAME_RUNNING){
        
        WRITE(1,1);
        
        //Getting values to print
        uint8 pop = getPopulationCntr(),
            vac = getVaccineCntr(),
            med = getMedicineCntr();
        
        LCD_ClearDisplay();
        LCD_Position(0u, 0u);
        
        // Format: "pop vac med"
        LCD_PrintDecUint16(pop);
        LCD_PrintString(" ");
        LCD_PrintDecUint16(vac);
        LCD_PrintString(" ");
        LCD_PrintDecUint16(med);
        
        
        WRITE(1,0);
        // Logic to ensure periodicity
        vTaskDelay(CORRECT_PERIOD(DISPLAY_PERIOD));
    }
    vTaskSuspend(displayHandler);
}

/*
*   Whenever the semaphore is realesed, put the population into quarantine.
*/
void contaminationTask(void* args){
    
    (void) args;
    while(IS_GAME_RUNNING){
        WRITE(4,0);
        xSemaphoreTake(contaminationFlag, portMAX_DELAY);
        WRITE(4,1);
        quarantine();
        
    }
    vTaskSuspend(contaminationHandler);
}
/* [] END OF FILE */
