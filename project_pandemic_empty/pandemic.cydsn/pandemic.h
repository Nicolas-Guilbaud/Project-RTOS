/* ========================================
 *
 *              ELECH410 labs
 *          FreeRTOS pandemic project
 * 
 *                   2020
 *
 * ========================================
*/

#ifndef PANDEMIC_H
#define PANDEMIC_H

#include "project.h"
    
typedef uint8 Token;
    
/* Task prototypes */
void gameTask( void * );

/** 
 *  When gameTask releases a vaccine clue it calls this function.
 *  You should implement this in main.c.
 *
 *  @param clue
 */
void releaseClue( Token clue );

/** 
 *  When a contamination occurs gameTask calls this function.
 *  You should implement this in main.c.
 */
void releaseContamination( void );

/** 
 *  Prevent contaminations by quarantine the population.
 */
void quarantine( void );

/** 
 *  Make the lab work on a mission.
 *  If mission is 0 then the lab is producing medicine pills.
 *  If mission is a clue then the lab is working on a vaccine.
 *
 *  @param mission
 *  @return Token The result of the research.
 */
Token assignMissionToLab( Token mission );

/** 
 *  Send medicine pills to gameTask.
 *
 *  @param medicine
 */
void shipMedicine( Token medicine );

/** 
 *  Send the result of the research on a vaccine to gameTask.
 *
 *  @param vaccine
 */
void shipVaccine( Token vaccine );

/** 
 *  @return uint8 The percentage of healthy population.
 */
uint8 getPopulationCntr( void );

/** 
 *  @return uint8 The vaccine counter.
 */
uint8 getVaccineCntr( void );

/** 
 *  @return uint8 The amount of medicine pills available.
 */
uint8 getMedicineCntr( void );

#endif //PANDEMIC_H

/* [] END OF FILE */
