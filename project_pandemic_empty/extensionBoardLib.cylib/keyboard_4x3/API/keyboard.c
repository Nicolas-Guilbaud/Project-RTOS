/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include "CyLib.h"

//#include "`$INSTANCE_NAME`_KeyboardPort.h"
//#include "`$INSTANCE_NAME`_KeyboardCol0.h"
#include "`$INSTANCE_NAME`_KeyboardCol1.h"
#include "`$INSTANCE_NAME`_KeyboardCol2.h"
#include "`$INSTANCE_NAME`_KeyboardCol3.h"

#include "`$INSTANCE_NAME`_KeyboardRow0.h"
#include "`$INSTANCE_NAME`_KeyboardRow1.h"
#include "`$INSTANCE_NAME`_KeyboardRow2.h"
#include "`$INSTANCE_NAME`_KeyboardRow3.h"

//#define LED_Write `$INSTANCE_NAME`_KeyboardPort_Write

//#define COL0_Write `$INSTANCE_NAME`_KeyboardCol0_Write
#define COL1_Write `$INSTANCE_NAME`_KeyboardCol1_Write
#define COL2_Write `$INSTANCE_NAME`_KeyboardCol2_Write
#define COL3_Write `$INSTANCE_NAME`_KeyboardCol3_Write

#define ROW0_Read `$INSTANCE_NAME`_KeyboardRow0_Read
#define ROW1_Read `$INSTANCE_NAME`_KeyboardRow1_Read
#define ROW2_Read `$INSTANCE_NAME`_KeyboardRow2_Read
#define ROW3_Read `$INSTANCE_NAME`_KeyboardRow3_Read

#define KBD_DELAY   5

void `$INSTANCE_NAME`_Test(void){
    //LED_Write(1);
    //Kb_KeaboardPort_Write(1);
}

void `$INSTANCE_NAME`_Start(void){
    COL1_Write(1);
    COL2_Write(1);
    COL3_Write(1);
}

uint8_t `$INSTANCE_NAME`_Scan(void){
    COL1_Write(0);
    CyDelay(KBD_DELAY);
    if (!ROW0_Read()) {
        COL1_Write(1);
        return('#');
    }
    if (!ROW1_Read()) {
        COL1_Write(1);
        return('9');
    }
    if (!ROW2_Read()) {
        COL1_Write(1);
        return('6');
    }
    if (!ROW3_Read()) {
        COL1_Write(1);
        return('3');
    }
    COL1_Write(1);
    
    COL2_Write(0);
    CyDelay(KBD_DELAY);
    if (!ROW0_Read()) {
        COL2_Write(1);
        return('0');
    }
    if (!ROW1_Read()) {
        COL2_Write(1);
        return('8');
    }
    if (!ROW2_Read()) {
        COL2_Write(1);
        return('5');
    }
    if (!ROW3_Read()) {
        COL2_Write(1);
        return('2');
    }
    COL2_Write(1);

    COL3_Write(0);
    CyDelay(KBD_DELAY);
    if (!ROW0_Read()) {
        COL3_Write(1);
        return('*');
    }
    if (!ROW1_Read()) {
        COL3_Write(1);
        return('7');
    }
    if (!ROW2_Read()) {
        COL3_Write(1);
        return('4');
    }
    if (!ROW3_Read()) {
        COL3_Write(1);
        return('1');
    }
    COL3_Write(1);
    
    return('z');
}

/* [] END OF FILE */
