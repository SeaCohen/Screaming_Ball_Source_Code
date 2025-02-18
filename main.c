#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"
#include "spi.h"
#include "hw_adc.h"
#include "hw_gprcm.h"
#include "adc.h"

//Common interface includes
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"
#include "i2c_if.h"

// Custom includes
#include "utils/network_utils.h"

#include "glcdfont.h"
#include "oled_test.h"
#include "Adafruit_SSD1351.h"
#include "Adafruit_GFX.h"

//RNG
#include <time.h>
#include <stdlib.h>



//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                1    /* Current Date */
#define MONTH               6     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                10    /* Time - hours */
#define MINUTE              39    /* Time - minutes */
#define SECOND              0     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a1fae5myyg50qw-ats.iot.us-east-1.amazonaws.com" // done
#define GOOGLE_DST_PORT       8443


#define POSTHEADER "POST /things/CC3200_lab4_EEC172_Virginia/shadow HTTP/1.1\r\n" // done
#define GETHEADER "GET /things/CC3200_lab4_EEC172_Virginia/shadow HTTP/1.1\r\n" // done

#define HOSTHEADER "Host: a1fae5myyg50qw-ats.iot.us-east-1.amazonaws.com\r\n"  // done
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"var\" :\""                                           \
                        "Hello phone, "                                     \
                        "message from CC3200 via AWS IoT!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static int set_time();
static void BoardInit(void);
static int http_post(int);






//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

#define DATA1_PART1 "{" \
                    "\"state\": {\r\n" \
                        "\"desired\" : {\r\n" \
                            "\"var\" :\""

#define DATA1_PART2 "\"\r\n" \
                        "}" \
                    "}" \
                "}\r\n\r\n"







//below is the game code

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100



#define APPLICATION_VERSION     "1.4.0"
#define APP_NAME                "I2C Demo"
#define UART_PRINT              Report
#define FOREVER                 1
#define CONSOLE                 UARTA0_BASE
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

#define UART_PRINT         Report
#define FOREVER            1
#define NO_OF_SAMPLES      128

unsigned long pulAdcSamples[NO_OF_SAMPLES];

#define PIN_58   0x00000039
// Note : PIN_MODE_255 is a dummy define for pinmux utility code generation
// PIN_MODE_255 should never be used in any user code.
#define PIN_MODE_255 0x000000FF

static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

void
DisplayPrompt()
{
    UART_PRINT("\n\rcmd#");
}

void
DisplayUsage()
{
    UART_PRINT("Command Usage \n\r");
    UART_PRINT("------------- \n\r");
    UART_PRINT("write <dev_addr> <wrlen> <<byte0> [<byte1> ... ]> <stop>\n\r");
    UART_PRINT("\t - Write data to the specified i2c device\n\r");
    UART_PRINT("read  <dev_addr> <rdlen> \n\r\t - Read data frpm the specified "
                "i2c device\n\r");
    UART_PRINT("writereg <dev_addr> <reg_offset> <wrlen> <<byte0> [<byte1> ... "
                "]> \n\r");
    UART_PRINT("\t - Write data to the specified register of the i2c device\n\r");
    UART_PRINT("readreg <dev_addr> <reg_offset> <rdlen> \n\r");
    UART_PRINT("\t - Read data from the specified register of the i2c device\n\r");
    UART_PRINT("\n\r");
    UART_PRINT("Parameters \n\r");
    UART_PRINT("---------- \n\r");
    UART_PRINT("dev_addr - slave address of the i2c device, a hex value "
                "preceeded by '0x'\n\r");
    UART_PRINT("reg_offset - register address in the i2c device, a hex value "
                "preceeded by '0x'\n\r");
    UART_PRINT("wrlen - number of bytes to be written, a decimal value \n\r");
    UART_PRINT("rdlen - number of bytes to be read, a decimal value \n\r");
    UART_PRINT("bytex - value of the data to be written, a hex value preceeded "
                "by '0x'\n\r");
    UART_PRINT("stop - number of stop bits, 0 or 1\n\r");
    UART_PRINT("--------------------------------------------------------------"
                "--------------- \n\r\n\r");

}


char myfinalbuffer[512];   // buffer to store the final JSON string to send to AWS
char myCharArray[3]={'0','1','8'}; //ACTUAL MESSAGE TO BE SENT


static int http_get(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(DATA1);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, DATA1);
    pcBufHeaders += strlen(DATA1);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        UART_PRINT("Message Gotten! ");
        UART_PRINT("\n\r\n\r");

        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");

        myCharArray[0] = acRecvbuff[223+14];
        myCharArray[1] = acRecvbuff[224+14];
        myCharArray[2] = acRecvbuff[225+14];
        UART_PRINT(myCharArray);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

void
DisplayBuffer(unsigned char *pucDataBuf, unsigned char ucLen)
{
    unsigned char ucBufIndx = 0;
    UART_PRINT("Read contents");
    UART_PRINT("\n\r");
    while(ucBufIndx < ucLen)
    {
        UART_PRINT(" 0x%x, ", pucDataBuf[ucBufIndx]);
        ucBufIndx++;
        if((ucBufIndx % 8) == 0)
        {
            UART_PRINT("\n\r");
        }
    }
    UART_PRINT("\n\r");
}

static void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t      CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

int
ProcessReadCommand(char *pcInpString)
{
    unsigned char ucDevAddr, ucLen;
    unsigned char aucDataBuf[256];
    char *pcErrPtr;
    int iRetVal;

    //
    // Get the device address
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucDevAddr = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    //
    // Get the length of data to be read
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucLen = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);
    //RETERR_IF_TRUE(ucLen > sizeof(aucDataBuf));

    //
    // Read the specified length of data
    //
    iRetVal = I2C_IF_Read(ucDevAddr, aucDataBuf, ucLen);

    if(iRetVal == SUCCESS)
    {
        UART_PRINT("I2C Read complete\n\r");

        //
        // Display the buffer over UART on successful write
        //
        DisplayBuffer(aucDataBuf, ucLen);
    }
    else
    {
        UART_PRINT("I2C Read failed\n\r");
        return FAILURE;
    }

    return SUCCESS;
}

int
ProcessReadRegCommand(char *pcInpString)
{
    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    unsigned char aucRdDataBuf[256];
    char *pcErrPtr;

    //
    // Get the device address
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucDevAddr = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    //
    // Get the register offset address
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucRegOffset = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);

    //
    // Get the length of data to be read
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucRdLen = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);
    //RETERR_IF_TRUE(ucLen > sizeof(aucDataBuf));

    //
    // Write the register address to be read from.
    // Stop bit implicitly assumed to be 0.
    //
    RET_IF_ERR(I2C_IF_Write(ucDevAddr,&ucRegOffset,1,0));

    //
    // Read the specified length of data
    //
    RET_IF_ERR(I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen));

    UART_PRINT("I2C Read From address complete\n\r");

    //
    // Display the buffer over UART on successful readreg
    //
    DisplayBuffer(aucRdDataBuf, ucRdLen);

    return SUCCESS;
}

//MICROPHONE STUFF

float MeasureLoudness(void)
{
    unsigned int uiIndex = 0;
        unsigned long ulSample;
        unsigned long ulMaxSample = 0;
        unsigned long ulMinSample = 0xFFFFFFFF;

        unsigned long pulAdcSamples[NO_OF_SAMPLES]; // Local variable for ADC samples

        //MAP_ADCEnable(ADC_BASE);

        while (uiIndex < NO_OF_SAMPLES + 4)
        {
            if (MAP_ADCFIFOLvlGet(ADC_BASE, ADC_CH_1))
            {
                ulSample = MAP_ADCFIFORead(ADC_BASE, ADC_CH_1);
                pulAdcSamples[uiIndex++] = ulSample;

                unsigned long ulCurrentSample = (ulSample >> 2) & 0x0FFF;
                if (ulCurrentSample > ulMaxSample)
                {
                    ulMaxSample = ulCurrentSample;
                }
                if (ulCurrentSample < ulMinSample)
                {
                    ulMinSample = ulCurrentSample;
                }
            }
        }

        //MAP_ADCChannelDisable(ADC_BASE, ADC_CH_1);

        unsigned long ulPeakToPeak = ulMaxSample - ulMinSample;
        float fVoltage = ((float)ulPeakToPeak * 1.4) / 4096;

        return fVoltage;
}

int
ProcessWriteRegCommand(char *pcInpString)
{
    unsigned char ucDevAddr, ucRegOffset, ucWrLen;
    unsigned char aucDataBuf[256];
    char *pcErrPtr;
    int iLoopCnt = 0;

    //
    // Get the device address
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucDevAddr = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);

    //
    // Get the register offset to be written
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucRegOffset = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    aucDataBuf[iLoopCnt] = ucRegOffset;
    iLoopCnt++;

    //
    // Get the length of data to be written
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucWrLen = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);
    //RETERR_IF_TRUE(ucWrLen > sizeof(aucDataBuf));

    //
    // Get the bytes to be written
    //
    for(; iLoopCnt < ucWrLen + 1; iLoopCnt++)
    {
        //
        // Store the data to be written
        //
        pcInpString = strtok(NULL, " ");
        RETERR_IF_TRUE(pcInpString == NULL);
        aucDataBuf[iLoopCnt] =
                (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    }
    //
    // Write the data values.
    //
    RET_IF_ERR(I2C_IF_Write(ucDevAddr,&aucDataBuf[0],ucWrLen+1,1));

    UART_PRINT("I2C Write To address complete\n\r");

    return SUCCESS;
}

int
ProcessWriteCommand(char *pcInpString)
{
    unsigned char ucDevAddr, ucStopBit, ucLen;
    unsigned char aucDataBuf[256];
    char *pcErrPtr;
    int iRetVal, iLoopCnt;

    //
    // Get the device address
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucDevAddr = (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);

    //
    // Get the length of data to be written
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucLen = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);
    //RETERR_IF_TRUE(ucLen > sizeof(aucDataBuf));

    for(iLoopCnt = 0; iLoopCnt < ucLen; iLoopCnt++)
    {
        //
        // Store the data to be written
        //
        pcInpString = strtok(NULL, " ");
        RETERR_IF_TRUE(pcInpString == NULL);
        aucDataBuf[iLoopCnt] =
                (unsigned char)strtoul(pcInpString+2, &pcErrPtr, 16);
    }

    //
    // Get the stop bit
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    ucStopBit = (unsigned char)strtoul(pcInpString, &pcErrPtr, 10);

    //
    // Write the data to the specified address
    //
    iRetVal = I2C_IF_Write(ucDevAddr, aucDataBuf, ucLen, ucStopBit);
    if(iRetVal == SUCCESS)
    {
        UART_PRINT("I2C Write complete\n\r");
    }
    else
    {
        UART_PRINT("I2C Write failed\n\r");
        return FAILURE;
    }

    return SUCCESS;
}

int
ParseNProcessCmd(char *pcCmdBuffer)
{
    char *pcInpString;
    int iRetVal = FAILURE;

    pcInpString = strtok(pcCmdBuffer, " \n\r");
    if(pcInpString != NULL)

    {

        if(!strcmp(pcInpString, "read"))
        {
            //
            // Invoke the read command handler
            //
            iRetVal = ProcessReadCommand(pcInpString);
        }
        else if(!strcmp(pcInpString, "readreg"))
        {
            //
            // Invoke the readreg command handler
            //
            iRetVal = ProcessReadRegCommand(pcInpString);
        }
        else if(!strcmp(pcInpString, "writereg"))
        {
            //
            // Invoke the writereg command handler
            //
            iRetVal = ProcessWriteRegCommand(pcInpString);
        }
        else if(!strcmp(pcInpString, "write"))
        {
            //
            // Invoke the write command handler
            //
            iRetVal = ProcessWriteCommand(pcInpString);
        }
        else
        {
            UART_PRINT("Unsupported command\n\r");
            return FAILURE;
        }
    }

    return iRetVal;
}

int8_t ReadAcceleration(unsigned char devAddr, unsigned char regOffset) {
    unsigned char dataBuf[1];
    int iRetVal;

    // Write register offset
    iRetVal = I2C_IF_Write(devAddr, &regOffset, 1, 0);
    if (iRetVal != SUCCESS) {
        printf("I2C write failed\n");
        return -1;
    }

    // Read acceleration data
    iRetVal = I2C_IF_Read(devAddr, dataBuf, 1);
    if (iRetVal != SUCCESS) {
        printf("I2C read failed\n");
        return -1;
    }

    // Interpret the data as a signed 8-bit integer for acceleration
    return (int8_t)dataBuf[0];
}




void createBackground(void)
{
  unsigned int i,j;
  goTo(0, 0);

  for(i=0;i<128;i++)
  {
    for(j=0;j<128;j++)
    {
      writeData(BLACK>>8);writeData((unsigned char) BLACK);
    }
  }
}



int playGame(int gamemode){ //Gamemode key
    int speedX =0; //speeds are added to momentum, momentum is added to position
    int speedY =0;

    int mass = 16; //Resistance to the acceleration




    //int zoneX = rand()%(maxDirX-minDirX-sizeOfZone)+minDirX;
    //int zoneY = rand()%(maxDirY-minDirY-sizeOfZone)+minDirY;

    int TicksPerSecond = 10;
    int barrierScore = 10;
    int radius = 5;
    int waitingTime = 8000000/TicksPerSecond;
    int minDirX = radius; //minDir and maxDir are based off of radius so game can work with any size of circle
    int maxDirX = 128-radius;
    int maxDirY = maxDirX;
    int minDirY = radius + barrierScore; //Ball should not be able to cross into the UI

    int prevX;
    int prevY;
    int momentumX =speedX;
    int momentumY =speedY;

    int score =0;
    int scoreIncrement = 4;
    int maxScoreTimer = 5;
    int startingZoneSize = 30;
    int scoreTimer = maxScoreTimer;
    int gameTimer = 300;
    int maxLaunchSpeed = 64;
    int hazardSize = 20;
    float quietTime = 0; //Ignore if less than this


    float wallspring = 1.0;
    if(gamemode == 1){
        Report ("\n\r CLASSIC START \n\r");
        //Game's default values are to represent the classic game so nothing needs to happen in setup
    }
    else if(gamemode ==2){ //Silent
        Report ("\n\r SILENT START \n\r");
        mass = 8; //found via feel
        wallspring = 0.75;
        scoreIncrement = 2;
    }
    else if(gamemode ==3){ //screamer
        Report ("\n\r SCREAMER START \n\r");
        scoreIncrement = 5;
        maxScoreTimer =1;
        //Need to scream to get anywhere, but this is handled when the loudness value is set
    }

    int8_t acceleration_x, acceleration_y;

    // Read accelerometer angle for X-axis
    acceleration_x = ReadAcceleration(0x18, 0x3);

    // Read accelerometer angle for Y-axis
    acceleration_y = -ReadAcceleration(0x18, 0x5);

    //Draw the background for ballin'
    createBackground();
    MAP_UtilsDelay(800000);

    //Setup initial ballin'
    int nextX = rand()%(maxDirX-minDirX)+minDirX;
    int nextY = rand()%(maxDirY-minDirY)+minDirY;


    //draw ball
    fillCircle(nextX, nextY, radius, WHITE);
    MAP_UtilsDelay(2000);

    //generate position of score zone
    int sizeOfZone = startingZoneSize;
    int zoneX = rand()%(maxDirX-minDirX-sizeOfZone)+minDirX;
    int zoneY = rand()%(maxDirY-minDirY-sizeOfZone)+minDirY;

    drawRect(zoneX, zoneY, sizeOfZone, sizeOfZone, YELLOW);

    int hazardX = rand()%(maxDirX-minDirX-hazardSize)+minDirX;
    int hazardY = rand()%(maxDirY-minDirY-hazardSize)+minDirY;
    drawRect(hazardX, hazardY, hazardSize, hazardSize, MAGENTA);

    //Draw in the UI
    int i=0;
    char scoreboard[] = "Time:XX  Score:000      ";
    while(i<=22){
        drawChar(i*6, 1, scoreboard[i], WHITE, 0x000F, 1);
        i++;
    }






    while(gameTimer>=0){ //start the main ball game
        float loudness = 0.0;
        if(gamemode==3){
            loudness = MeasureLoudness()*MeasureLoudness();
        }
        if(gamemode==1){
            loudness = MeasureLoudness()*1.2;
        }
        loudness = loudness*loudness; //square loudness so it feels more responsive.
        if(gamemode ==2){
            loudness = 1.0; //if game is in silent mode loudness is just always set to 1
        }

        //Update top display

        //timer
        if(gameTimer%10==0){
            drawChar(5*6, 1, '0'+gameTimer/100, WHITE, 0x000F, 1);
            drawChar(6*6, 1, '0'+gameTimer/10%10, WHITE, 0x000F, 1);
        }


        prevX = nextX; //next becomes the previous
        prevY = nextY;

        //Get current tilt here
        // Read acceleration for X-axis
        acceleration_x = ReadAcceleration(0x18, 0x3);

        // Read acceleration for Y-axis
        acceleration_y = -ReadAcceleration(0x18, 0x5);



        //Calculate speed ball should be accelerated
        int speedX = acceleration_x/mass;
        int speedY = acceleration_y/mass; //8 felt right for the speed we wanted without mic. Smaller number, faster ball
        momentumX = momentumX+ speedX*loudness*loudness;
        momentumY = momentumY+ speedY*loudness*loudness;
        if(loudness<quietTime){ //user suddenly gets quiet
            momentumX=0;
            momentumY=0;
        }
        //Get next position X
        nextX = prevX +momentumX;
        if(nextX < minDirX){ //Left wall
            nextX = minDirX;
            momentumX = -momentumX*wallspring; //this has the ball bounce FASTER THAN IT COMES IN
        }
        if(nextX > maxDirX){ //Right wall
            nextX = maxDirX;
            momentumX = -momentumX*wallspring; //perfectly elastic collisions would not multiply in a fraction
        }

        //Get next position Y
        nextY = prevY +momentumY;
        if(nextY < minDirY){ //Bottom wall
            nextY = minDirY;
            momentumY = -momentumY*wallspring; // The walls are springs that bounce ball faster than it comes in...
        }
        if(nextY > maxDirY){ //Top wall
            nextY = maxDirY;
            momentumY = -momentumY*wallspring;
        }


        //Scoring Check
        if((nextX>=zoneX && nextX<=zoneX+sizeOfZone) && (nextY>=zoneY && nextY<=zoneY+sizeOfZone)){
            gameTimer++; //player should not game over inside a green area, scoring "pauses" timer
            if(gameTimer%10==1){ //prevents game from constantly freezing to update the timer over and over
                gameTimer++;
            }
            if(scoreTimer==maxScoreTimer){
                drawRect(zoneX, zoneY, sizeOfZone, sizeOfZone, GREEN);
            }
            scoreTimer--;
            if(scoreTimer<=0){
                score+=scoreIncrement;


                //Update Score on display
                drawChar(15*6, 1, '0'+score/100%10, WHITE, 0x000F, 1);
                drawChar(16*6, 1, '0'+score/10%10, WHITE, 0x000F, 1);
                drawChar(17*6, 1, '0'+score%10, WHITE, 0x000F, 1);
                drawRect(zoneX, zoneY, sizeOfZone, sizeOfZone, BLACK);
                sizeOfZone--; //Make the game harder when a point is scored
                scoreTimer = maxScoreTimer; //Reset score timer to make player wait half a second again to score more points
                if(sizeOfZone<10){
                    sizeOfZone = 10;
                }
                zoneX = rand()%(maxDirX-minDirX-sizeOfZone)+minDirX; //Randomly generate a new spot for the score zone to be
                zoneY = rand()%(maxDirY-minDirY-sizeOfZone)+minDirY;

                drawRect(zoneX, zoneY, sizeOfZone, sizeOfZone, YELLOW);
            }
        }
        else if(scoreTimer<maxScoreTimer){
            scoreTimer = maxScoreTimer;
            drawRect(zoneX, zoneY, sizeOfZone, sizeOfZone, YELLOW);
        }

        //Random Launch Section
        //(nextX>=zoneX && nextX<=zoneX+sizeOfZone) && (nextY>=zoneY && nextY<=zoneY+sizeOfZone)
        if((nextX>=hazardX && nextX<=hazardX+hazardSize) && (nextY>=hazardY && nextY<=hazardY+hazardSize)){
            //Launch the ball
            momentumX = rand()%maxLaunchSpeed;
            if(rand()%2){
                momentumX = -momentumX;
            }
            momentumY = rand()%maxLaunchSpeed;
            if(rand()%2){
                momentumY = -momentumY;
            }
            drawRect(hazardX, hazardY, hazardSize, hazardSize, BLACK);
            hazardX = rand()%(maxDirX-minDirX-hazardSize)+minDirX;
            hazardY = rand()%(maxDirY-minDirY-hazardSize)+minDirY;
            drawRect(hazardX, hazardY, hazardSize, hazardSize, MAGENTA);
        }

        //Delete previous ball
        fillCircle(prevX, prevY, radius, BLACK);
        MAP_UtilsDelay(20000);

        //Draw new ball
        fillCircle(nextX, nextY, radius, WHITE);
        MAP_UtilsDelay(waitingTime);


        //Friction
        if(gamemode ==2){
            //In silent mode loudness is set to 0.9 to act like friction right here
            loudness = 0.9;
        }
        momentumX = momentumX*loudness; //Loudness is used as a "friction" louder volume means ball go faster
        momentumY = momentumY*loudness; //Simply have motion decay over time... Closer fraction is to 1, the lower the friction

        gameTimer--;

        if(gameTimer%50==0){
            drawRect(hazardX, hazardY, hazardSize, hazardSize, BLACK);
            hazardX = rand()%(maxDirX-minDirX-hazardSize)+minDirX;
            hazardY = rand()%(maxDirY-minDirY-hazardSize)+minDirY;
            drawRect(hazardX, hazardY, hazardSize, hazardSize, MAGENTA);
        }

        if(gameTimer%10==0){

        }

    }

    return score;
}



















void main() {
    long lRetVal = -1;
    //
    // Initialize board configuration
    //
    BoardInit();

    srand(time(NULL)); //random number generator init


    PinMuxConfig();
    unsigned long uiAdcInputPin = PIN_58; // Default ADC input pin
    unsigned int uiChannel = ADC_CH_1;

    MAP_PinTypeADC(uiAdcInputPin, PIN_MODE_255);
    MAP_ADCTimerConfig(ADC_BASE, 2^17);
    MAP_ADCTimerEnable(ADC_BASE);
    MAP_ADCEnable(ADC_BASE);
    MAP_ADCChannelEnable(ADC_BASE, uiChannel);

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);


    InitTerm();
    ClearTerm();

    I2C_IF_Open(I2C_MASTER_MODE_FST);    // I2C Init

#ifdef CC3200_ES_1_2_1
    HWREG(GPRCM_BASE + GPRCM_O_ADC_CLK_CONFIG) = 0x00000043;
    HWREG(ADC_BASE + ADC_O_ADC_CTRL) = 0x00000004;
    HWREG(ADC_BASE + ADC_O_ADC_SPARE0) = 0x00000100;
    HWREG(ADC_BASE + ADC_O_ADC_SPARE1) = 0x0355AA00;
#endif

    UART_PRINT("My terminal works!\n\r");

//    while (FOREVER)
//        {
//            float loudness = MeasureLoudness();
//            UART_PRINT("\n\rLoudness (Peak-to-Peak Voltage): %f\n\r", loudness);
//        }




    DisplayBanner(APP_NAME);    // Display the banner followed by the usage description
    DisplayUsage();

    //
        // Reset the peripheral
        //
        MAP_PRCMPeripheralReset(PRCM_GSPI);

        //
        // Reset SPI
        //
        MAP_SPIReset(GSPI_BASE);

        //
        // Configure SPI interface
        //
        MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                         SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                         (SPI_SW_CTRL_CS |
                         SPI_4PIN_MODE |
                         SPI_TURBO_OFF |
                         SPI_CS_ACTIVEHIGH |
                         SPI_WL_8));
        //
        // Enable SPI for communication
        //
        MAP_SPIEnable(GSPI_BASE);





    /////////////////////////////  all initialization is before this line

        Adafruit_Init();
            int highscore =0;



            //http_get(lRetVal);






    char* myfinal;
    myfinal = myfinalbuffer;





// Combine the parts into the final buffer



/////////////

                    //Clear the buffer
                    int i=0;
                    while(i<512){
                        myfinalbuffer[i] = 0;
                        i++;
                    }

                    /*
                    //comment this out when doing real runs
                    myCharArray[0]='0';
                    myCharArray[1]='1';
                    myCharArray[2]='3';
                    myfinal = myfinalbuffer;



                    strcpy(myfinal, DATA1_PART1);
                    myfinal+= strlen(DATA1_PART1);

                    strcpy(myfinal, myCharArray);
                    myfinal += strlen(myCharArray);

                    strcpy(myfinal, DATA1_PART2);
                    myfinal += strlen(DATA1_PART2);

                    Report ("\n\r printing myfinalbuffer");
                    Report("%s\n", myfinalbuffer);

                    http_post(lRetVal);

                    //Comment this out when doing real runs
                    */




                    int globalhighscore = 0;




                    createBackground();
                    bool competitive;
                    bool waiting = true;
                    char holder1x[] = "Go for World Record?    ";
                    char holder2x[] = "Right selects YES     ";
                    char holder3x[] = "Left selects NO           ";
                    char globalHSDisplay[] = "Global HS is:             ";

                    int x = 0;
                    while(x<=22){ //17 total characters in above phrase
                       drawChar(x*6, 30, holder1x[x], MAGENTA, BLACK, 1);
                       drawChar(x*6, 50, holder2x[x], WHITE, BLACK, 1);
                       drawChar(x*6, 60, holder3x[x], WHITE, BLACK, 1);
                       //drawChar(x*6, 100, globalHSDisplay[x], YELLOW, BLACK, 1);
                       x++;
                   }
                    x=0;
                    while(x<3){
                        //drawChar(x*6+15*6, 100, myCharArray[x], YELLOW, BLACK, 1);
                        x++;
                    }
                    while(waiting){
                        x = 0;
                        MAP_UtilsDelay(100000); //Wait until user tilts up to start the game, other directions are an options select
                       if(ReadAcceleration(0x18, 0x3)>40){ //Right
                           while(x<=22){
                               drawChar(x*6, 30, holder1x[x], WHITE, BLACK, 1);
                               drawChar(x*6, 50, holder2x[x], MAGENTA, BLACK, 1);
                               drawChar(x*6, 60, holder3x[x], WHITE, BLACK, 1);
                               x++;
                               waiting = false;
                           }
                           competitive = true;
                       }
                       if(ReadAcceleration(0x18, 0x3)<-40){ //Left
                           while(x<=22){
                               drawChar(x*6, 30, holder1x[x], WHITE, BLACK, 1);
                               drawChar(x*6, 50, holder2x[x], WHITE, BLACK, 1);
                               drawChar(x*6, 60, holder3x[x], MAGENTA, BLACK, 1);
                               x++;
                               waiting = false;
                           }
                           competitive = false;
                       }
                    }

                    if(competitive){
                        // initialize global default app configuration
                            g_app_config.host = SERVER_NAME;
                            g_app_config.port = GOOGLE_DST_PORT;

                            //Connect the CC3200 to the local access point
                            lRetVal = connectToAccessPoint();
                            //Set time so that encryption can be used
                            lRetVal = set_time();
                            if(lRetVal < 0) {
                                UART_PRINT("Unable to set time in the device");
                                LOOP_FOREVER();
                            }
                            //Connect to the website with TLS encryption
                            lRetVal = tls_connect();
                            if(lRetVal < 0) {
                                ERR_PRINT(lRetVal);
                            }


                            //Clear the buffer
                            i=0;
                            while(i<512){
                                myfinalbuffer[i] = 0;
                                i++;
                            }
                            myfinal = myfinalbuffer;

                            strcpy(myfinal, DATA1_PART1);
                            myfinal+= strlen(DATA1_PART1);

                            strcpy(myfinal, DATA1_PART2);
                            myfinal += strlen(DATA1_PART2);

                            Report ("\n\r printing myfinalbuffer FOR GET");
                            Report("%s\n", myfinalbuffer);

                            http_get(lRetVal);

                            globalhighscore = (myCharArray[0]-'0')*100+ (myCharArray[1]-'0')*10 +(myCharArray[2]-'0')*1;
                    }


            while(FOREVER){
                //Clear the buffer
                /*
                myCharArray[0]='0';
                myCharArray[1]='2';
                myCharArray[2]='3';
                */
                if(competitive){
                    i=0;
                    while(i<512){
                        myfinalbuffer[i] = 0;
                        i++;
                    }
                    UART_PRINT(myCharArray);
                    UART_PRINT("\n\r\n\r");
                    myfinal = myfinalbuffer;



                    strcpy(myfinal, DATA1_PART1);
                    myfinal+= strlen(DATA1_PART1);

                    strcpy(myfinal, myCharArray);
                    myfinal += strlen(myCharArray);

                    strcpy(myfinal, DATA1_PART2);
                    myfinal += strlen(DATA1_PART2);

                    UART_PRINT(myCharArray);
                    UART_PRINT("\n\r\n\r");

                    Report ("\n\r printing myfinalbuffer");
                    Report("%s\n", myfinalbuffer);

                    Report ("\n\r PRINTTTTTTTTTT \n\r");

                    http_post(lRetVal);

                    Report ("\n\r DONNNNEEEEEE \n\r");
                }

                createBackground();




                int i=0;
                char mainMenu[] = "Tilt DOWN to Start!   ";
                while(i<=20){
                    drawChar(i*6, 10, mainMenu[i], MAGENTA, BLACK, 1);
                    i++;
                }
                if(competitive){
                    char globalHSDisplay[] = "Global HS is:             ";
                    i=0;
                    while(i<=20){
                        drawChar(i*6, 100, globalHSDisplay[i], YELLOW, BLACK, 1);
                        i++;
                    }
                    i=0;
                    while(i<3){
                        drawChar(i*6+15*6, 100, myCharArray[i], YELLOW, BLACK, 1);
                        i++;
                    }
                }

                //drawRect(zoneX, zoneY, sizeOfZone+zoneX, sizeOfZone+zoneY, YELLOW);
                bool waitForUser = true;
                int myGamemode = 2;
                char holder1[] = "Up selects Classic    ";
                char holder2[] = "Right selects Silent     ";
                char holder3[] = "Left selects Screamer   ";

                char holder4[] = "Local Highscore:          ";
                //Draw in the options menu
                i=0;
                //if(!competitive){
                    while(i<=22){ //17 total characters in above phrase
                        drawChar(i*6, 30, holder1[i], WHITE, BLACK, 1);
                        drawChar(i*6, 40, holder2[i], MAGENTA, BLACK, 1);
                        drawChar(i*6, 50, holder3[i], WHITE, BLACK, 1);
                        i++;
                    }
                //}
                i=0;
                while(i<22){
                    drawChar(i*6, 70, holder4[i], YELLOW, BLACK, 1);
                    i++;
                }
                drawChar(18*6, 70, '0'+highscore/100%10, YELLOW, BLACK, 1);
                drawChar(19*6, 70, '0'+highscore/10%10, YELLOW, BLACK, 1);
                drawChar(20*6, 70, '0'+highscore%10, YELLOW, BLACK, 1);
                while(waitForUser){
                    i=0;
                    MAP_UtilsDelay(100000); //Wait until user tilts up to start the game, other directions are an options select
                    //if(!competitive){
                        if(ReadAcceleration(0x18, 0x5)>40){
                            myGamemode=1;
                            while(i<=22){
                                drawChar(i*6, 30, holder1[i], MAGENTA, BLACK, 1);
                                drawChar(i*6, 40, holder2[i], WHITE, BLACK, 1);
                                drawChar(i*6, 50, holder3[i], WHITE, BLACK, 1);
                                i++;
                            }
                        }
                        if(ReadAcceleration(0x18, 0x3)>40){
                            myGamemode=2;
                            while(i<=22){
                                drawChar(i*6, 30, holder1[i], WHITE, BLACK, 1);
                                drawChar(i*6, 40, holder2[i], MAGENTA, BLACK, 1);
                                drawChar(i*6, 50, holder3[i], WHITE, BLACK, 1);
                                i++;
                            }
                        }
                        if(ReadAcceleration(0x18, 0x3)<-40){
                            myGamemode=3;
                            while(i<=22){ //17 total characters in above phrase
                                drawChar(i*6, 30, holder1[i], WHITE, BLACK, 1);
                                drawChar(i*6, 40, holder2[i], WHITE, BLACK, 1);
                                drawChar(i*6, 50, holder3[i], MAGENTA, BLACK, 1);
                                i++;
                            }
                        }
                    //}
                    if(ReadAcceleration(0x18, 0x5)<-40){ //start the game
                        waitForUser = false;
                    }
                }




                int newGlobal = 0;

                int ballscore = playGame(myGamemode);
                int myscore = ballscore;
                //int myscore = globalhighscore+1;


                /*UART_PRINT("\n\r LOUDNESS COMING \n\r");
                float myTest = MeasureLoudness();
                UART_PRINT("\n\r LOUDNESS AFTER \n\r");*/

                if(myscore>highscore){
                    highscore = myscore;
                }
                if(competitive){
                    if(myscore > globalhighscore){
                        globalhighscore = myscore;
                        newGlobal = 1;
                    }
                }

                createBackground(); //Make entire screen black (this functions as our "game over")


                //Note to self make this scoring function more complex eventually... Just know this is proof of concept...
                char localscore[] = "Previous Score:           ";
                char localhighscore[] = "Your High Score:        ";
                char globalhighscore[] ="World Record:            ";

                i =0;
                while(i<22){
                    drawChar(i*6, 20, localscore[i], WHITE, BLACK, 1);
                    drawChar(i*6, 40, localhighscore[i], YELLOW, BLACK, 1);
                    if(competitive){
                        drawChar(i*6, 60, globalhighscore[i], MAGENTA, BLACK, 1);
                    }

                    i++;
                }
                drawChar(18*6, 20, '0'+myscore/100%10, WHITE, BLACK, 1);
                drawChar(19*6, 20, '0'+myscore/10%10, WHITE, BLACK, 1);
                drawChar(20*6, 20, '0'+myscore%10, WHITE, BLACK, 1);


                drawChar(18*6, 40, '0'+highscore/100%10, YELLOW, BLACK, 1);
                drawChar(19*6, 40, '0'+highscore/10%10, YELLOW, BLACK, 1);
                drawChar(20*6, 40, '0'+highscore%10, YELLOW, BLACK, 1);

                if(competitive){
                    if(newGlobal){
                        myCharArray[0] = '0'+myscore/100%10;
                        myCharArray[1] = '0'+myscore/10%10;
                        myCharArray[2] = '0'+myscore%10;
                    }

                    drawChar(18*6, 60, myCharArray[0], MAGENTA, BLACK, 1);
                    drawChar(19*6, 60, myCharArray[1], MAGENTA, BLACK, 1);
                    drawChar(20*6, 60, myCharArray[2], MAGENTA, BLACK, 1);
                }

                MAP_UtilsDelay(50000000); //Wait for a good while

            }

}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(myfinalbuffer);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, myfinalbuffer);
    pcBufHeaders += strlen(myfinalbuffer);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);



    //
    // Send the packet to the server */
    //
    Report ("\n\r Printin right before sned valuer");
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    Report ("\n\r Printing right after send value \n\r");
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }
    return 0;
}


