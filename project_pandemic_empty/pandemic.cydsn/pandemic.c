/* ========================================
 *
 *              ELECH410 labs
 *          FreeRTOS pandemic project
 * 
 *                   2020
 *
 * ========================================
*/

#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "pandemic.h"

#define GAME_PERIOD              (10u) // in ms
#define CONTAMINATION_DEADLINE   (10u) // in ms
#define CONTAMINATION_PERIOD_MAX (300u) // in 10ms
#define CONTAMINATION_PERIOD_MIN (50u) // in 10ms
#define CLUE_PERIOD              (300u) // in 10ms
#define VIRUS_PERIOD             (500u) // in 10ms
#define VACCINE_DELAY            (2500u) // in ms
#define MEDICINE_DELAY           (500u) // in ms
#define SHIP_MEDICINE_DELAY      (500u) // in ms
#define CONTAMINATION_STRENGTH   (20u) //in %
#define VIRUS_SPREAD_STRENGTH    (5u) //in %
#define VACCINE_STRENGTH         (3u) //in %

enum status_e {
    IN_QUARANTINE,
    NO_QUARANTINE,
};

/* Game variables */
uint8 populationCntr = 100u;
uint8 vaccineCntr = 0u;
uint8 medicineCntr = 0u;

uint8 quarantineStatus = NO_QUARANTINE;
uint16 contaminationPeriod = CONTAMINATION_PERIOD_MIN;
uint16 cluePeriod = CLUE_PERIOD;
uint16 virusPeriod = VIRUS_PERIOD;

Token missionCpy;
Token clue;

/* Mutex */
SemaphoreHandle_t quarantineMutex;

/* Timer */
TimerHandle_t contaminationHandler;
void deadlineContamination();

/* Function prototype */
void startContamination(void);
uint8_t encode(uint8_t);

void gameTask( void *arg ){
    (void) arg;
    
    uint8 medicineUsed;
    quarantineMutex = xSemaphoreCreateMutex();

    while(populationCntr > 0u && vaccineCntr < 100u)
    {
        contaminationPeriod--;
        cluePeriod--;
        virusPeriod--;
        
        if(contaminationPeriod == 0u){
            contaminationPeriod = (rand() % (CONTAMINATION_PERIOD_MAX - CONTAMINATION_PERIOD_MIN)) + CONTAMINATION_PERIOD_MIN;
            startContamination();
        }
        if (cluePeriod == 0u){
            cluePeriod = CLUE_PERIOD; 

            clue = rand() & 0xFF; // Returns a random integer between 0 and 255.
            clue = clue != 0u ? clue : 1u;
            
            releaseClue(clue);
        }if (virusPeriod == 0u){
            virusPeriod = VIRUS_PERIOD;
            
            if (medicineCntr > VIRUS_SPREAD_STRENGTH ){
                medicineUsed = VIRUS_SPREAD_STRENGTH;
            }else{
                medicineUsed = medicineCntr;
            }
            populationCntr -= (VIRUS_SPREAD_STRENGTH - medicineUsed);
            medicineCntr -= medicineUsed;
        }
        
        vTaskDelay(GAME_PERIOD);
    }
    
    for(;;)
    {
        LCD_Position(1u, 0u);
        if (vaccineCntr >= 100u){
            LCD_PrintString("   WIN  ");
        }else{
            LCD_PrintString("  LOSE  ");
        }
        for (uint8 i = 0u; i < 5u; i++){
            LED1_Write(i == 0u ? 0u : 1u);
            LED2_Write(i == 1u ? 0u : 1u);
            LED3_Write(i == 2u ? 0u : 1u);
            LED4_Write(i == 3u ? 0u : 1u);
            vTaskDelay(75u);
        }
        vTaskDelay(500u);        
    }
}

void quarantine( void ){
    if(xQueuePeek((xQueueHandle)quarantineMutex, (void *)NULL, (portTickType)NULL) != pdTRUE){  
        /* You lose !
         * releaseContamination() called quarantine() or
         * one of your task has a higher priority than gameTask */
        populationCntr = 0u;  
    }else{
        quarantineStatus = IN_QUARANTINE;
    }  
}

Token assignMissionToLab( Token mission){
    Token result;
    
    missionCpy = mission;
    uint32 delay;
    
    /* Produce medicine pills */
    if (mission == 0u){
        delay = MEDICINE_DELAY;    
        result = 0u;
    
    /* Work on a vaccine */
    }else{
        delay = VACCINE_DELAY;
        result = encode(mission);
    }
    
    CyDelay(delay); // CyDelay() doesn't suspend the task
    
    if (missionCpy != mission){
        /* You lose !
         * Don't forget to protect shared resources ! */
        populationCntr = 0u;
    }
    
    return result;
}

void shipMedicine( Token medicine){
    (void) medicine;
    
    CyDelay(SHIP_MEDICINE_DELAY); // CyDelay() doesn't suspend the task
    medicineCntr++;
}

void shipVaccine( Token vaccine){
    if (vaccine == encode(clue))
    {
        vaccineCntr += VACCINE_STRENGTH;    
    }
}

void startContamination(){
    contaminationHandler = xTimerCreate(
        "Contamination",
        CONTAMINATION_DEADLINE,
        pdFALSE,
        NULL,
        deadlineContamination
    );
    quarantineStatus = NO_QUARANTINE;
    
    xTimerStart( contaminationHandler, 0u );
    xSemaphoreTake( quarantineMutex, portMAX_DELAY );
    releaseContamination();
    xSemaphoreGive( quarantineMutex);
}

void deadlineContamination(){
    if (quarantineStatus == IN_QUARANTINE)
    {
        quarantineStatus = NO_QUARANTINE; 
    }else{
        if (populationCntr > CONTAMINATION_STRENGTH){
             populationCntr -= CONTAMINATION_STRENGTH;
        }else{
            populationCntr = 0u;
        }
    }
}

uint8 getPopulationCntr(){
    return populationCntr;
}

uint8 getMedicineCntr(){
    return medicineCntr;
}

uint8 getVaccineCntr(){
    return vaccineCntr;
}

uint8_t encode(uint8_t n){
    int mask = 0b01001101001110011 >> 3u;
    n = (n & ~mask) | (~n & mask);
    return n;
}

/* [] END OF FILE */
