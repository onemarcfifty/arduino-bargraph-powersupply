
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// Netzteil - display current consumption
// of a lab power supply
// in a bar graph
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////


// ////////////////////////////////////////////////////////////////////////////////////////
// Include application, user and local libraries
// ////////////////////////////////////////////////////////////////////////////////////////

#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include <Bounce2.h>

// ////////////////////////////////////////////////////////////////////////////////////////
// defines for TFT library
// ////////////////////////////////////////////////////////////////////////////////////////


#ifdef ARDUINO_ARCH_STM32F1
#define TFT_RST PA1
#define TFT_RS  PA2
#define TFT_CS  PA0 // SS
#define TFT_SDI PA7 // MOSI
#define TFT_CLK PA5 // SCK
#define TFT_LED 0 // 0 if wired to +5V directly
#else
#define TFT_RST 8
#define TFT_RS  9
#define TFT_CS  7  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 3   // 0 if wired to +5V directly
#endif

#define TFT_BRIGHTNESS 0 // Initial brightness of TFT backlight (optional)

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
// Use software SPI (slower)
//TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED, TFT_BRIGHTNESS);

// ////////////////////////////////////////////////////////////////////////////////////////
// defines for Display
// and analogue input
// ////////////////////////////////////////////////////////////////////////////////////////


#define MAX_BARS 20           // the maximum number of graph bars we want to draw
#define MIN_BARS 3             // the minimum number of bars
#define NUM_CHANNELS 3         // the number of channels we have
#define MAX_DELAY 200           // the maximum delay in seconds
#define MIN_DELAY 0            // the minimum delay in seconds
//#define MAX_VALUE 20           // the max number (Y Value) to draw

String channel_Name[NUM_CHANNELS] = { "3.3V", "5V",  "12V"};
static int bar_Color[NUM_CHANNELS] = {COLOR_GREEN, COLOR_RED, COLOR_BLUE};
static int analog_Channel[NUM_CHANNELS] = {A7,A6,A5};

// ////////////////////////////////////////////////////////////////////////////////////////
// defines for Buttons and Operational mode
// ////////////////////////////////////////////////////////////////////////////////////////

#define BUTTON_UP 5  // the digital Pin which the UP button is attached to
#define BUTTON_OK 6  // ...OK Button ...
#define BUTTON_DN 4  // ... Down Button

Bounce button_Up = Bounce();
Bounce button_OK = Bounce();
Bounce button_Dn = Bounce();

#define OPMODES 3            // The number of operation modes we can address with the OK button
#define OPMODESCALE 0        // change the scaling
#define OPMODESPEED 1        // change the display speed
#define OPMODERESOLUTION 2   // change the number of bars

String op_Mode[OPMODES] = { "Scale", "Speed",  "Resolution"};
int currentOpMode=1;

#define NUMSCALEMODES 5
float scale_Mode[NUMSCALEMODES] = { 1, 0.4, 0.2, 0.1, 0.04};
int currentScaleMode=1;


// ////////////////////////////////////////////////////////////////////////////////////////
// the scaling Values of the ADC
// ////////////////////////////////////////////////////////////////////////////////////////

#define MAX_ADC_VALUE 1023         // the max value we should ever read
#define MIN_ADC_VALUE 510          // the min Value we should ever read
#define MAX_REAL_VALUE 5           // the real Value this corresponds to
#define MIN_REAL_VALUE 0           // ditto

// ////////////////////////////////////////////////////////////////////////////////////////
// X and Y Offset of the Text Area
// ////////////////////////////////////////////////////////////////////////////////////////

#define XOFFSET  graphArea[2] 
#define YOFFSET (graphArea[3]-graphArea[1]) / (NUM_CHANNELS+1) * i

// //////////////////////////////////////////////////////
// Variables and constants
// //////////////////////////////////////////////////////

uint16_t x, y;
boolean flag = false;

static int num_Bars = 5;                        // the _current_ number of graph bars
int draw_Delay = 5;                             // the delay factor limiting the speed of the display
static int bar_Counter=0;                       // the variable indicating the position in the circular memory
static int all_Values[MAX_BARS][NUM_CHANNELS];  // the circular buffer for the read values

static   uint16_t graphArea[]={1,1,0,0};        // the global var indicating the graphics area on the screen


// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////

/*   SCREEN LAYOUT
 *        
 *   ******************************************
 *   * GRAPH AREA                   * TEXT    *
 *   *                              * AREA    *
 *   *                              *         *
 *   *                              *         *
 *   *                              *         *
 *   *                              *         *
 *   ******************************************
 *   *  STATUS BAR                            *
 *   ******************************************
 * 
 */
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////


// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// Setup
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////



void setup() {

  // initialize the values of all channels

  for (int i = 0; i< MAX_BARS; i++)
    for (int j=0; j<NUM_CHANNELS; j++)
      all_Values[i][j]=MIN_ADC_VALUE;

  // init for TFT Display
  
  tft.begin();
  Serial.begin(9600);

  tft.setOrientation(1);

  // we use the upper left 2/3rd for the graph
  graphArea[2] = tft.maxX()*2/3;
  graphArea[3] = tft.maxY()*5/6;

  // define the Buttons as bouncers

  pinMode(BUTTON_UP,INPUT_PULLUP);
  pinMode(BUTTON_OK,INPUT_PULLUP);
  pinMode(BUTTON_DN,INPUT_PULLUP);

  button_Up.attach(BUTTON_UP);
  button_Up.interval(5);
  button_OK.attach(BUTTON_OK);
  button_OK.interval(50);
  button_Dn.attach(BUTTON_DN);
  button_Dn.interval(5);

  // TFT inits


  newScreen();

}

// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// newScreen
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////


void newScreen()
{

  // Sometimes we need to rebuild the screen, e.g. when the number of bars change
  
  tft.clear();

  //tft.drawRectangle(0, 0, tft.maxX() - 1, tft.maxY() - 1, COLOR_WHITE);
  //tft.drawRectangle(graphArea[0],graphArea[1],graphArea[2],graphArea[3], COLOR_BLUE);

  tft.setFont(Terminal6x8);
  for (int i=0; i<NUM_CHANNELS; i++) 
  {

    // Text area to the right of the graph area

    tft.drawText(XOFFSET + 10, YOFFSET +10, channel_Name[i], bar_Color[i]);

  }

}


// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// drawStatusBar
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////


void drawStatusBar ()
{

  // print the current operation mode into the status bar
  // current mode in red, all others in white
  
  tft.setFont(Terminal6x8);
  for (int i=0; i<OPMODES; i++)
  {
     tft.drawText(10 + tft.maxX() * i/OPMODES , graphArea[2]+10, op_Mode[i], (currentOpMode==i)?COLOR_RED:COLOR_WHITE); 
  }
/*
    String theMessage = String ("ctr: " + String( bar_Counter,DEC) + " ");
  theMessage += op_Mode[currentOpMode] + " " + String(scale_Mode[currentScaleMode],2)+ " " + String(currentScaleMode,DEC) + "       ";

  
  tft.drawText( 10, graphArea[2] +10, theMessage);
*/
  // print the measured values into the text area

  tft.setFont(Terminal11x16);
  for (int i=0; i<NUM_CHANNELS; i++) 
  {
    float percentage = (float)( all_Values[bar_Counter][i] - MIN_ADC_VALUE) / (float)(MAX_ADC_VALUE - MIN_ADC_VALUE);
    float theRealValue = (MIN_REAL_VALUE + percentage * (MAX_REAL_VALUE-MIN_REAL_VALUE))*1000;
     
    tft.drawText(XOFFSET + 10, YOFFSET +20, String(theRealValue,0) + " mA      ", bar_Color[i]);
  }
  tft.setFont(Terminal6x8);
}

// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// readTheValues
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////


void readTheValues()
{
  // we just read in the raw ADC Value into the array and do the scaling at draw time
  
  for (int i=0; i< NUM_CHANNELS; i++)
    all_Values[bar_Counter][i] = analogRead(analog_Channel[i]);
}


// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// Loop
// //////////////////////////////////////////////////////
// //////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  // update the bouncers
  
  button_Up.update();
  button_Dn.update();
  button_OK.update();

  // OK button pressed - change the operational mode

  int btnValue = button_OK.read();
  if (btnValue == LOW)
    if (currentOpMode < (OPMODES -1))
      currentOpMode++;
    else
      currentOpMode=0;

  // Up or down button pressed - give "adjust" the value of -1 (Down) or +1 (Up)

  int adjust=0;
  
  btnValue = button_Dn.read();
  if (btnValue == LOW)
    adjust=-1;
  else 
  {
    btnValue = button_Up.read();
    if (btnValue == LOW)
      adjust=1;
  }

  // If the mode is OPMODERESOLUTION, we increase / decrease the number of bars which are drawn

  if (currentOpMode==OPMODERESOLUTION)
  {

    int old_num_Bars = num_Bars;
    num_Bars += adjust;
    num_Bars = constrain (num_Bars, MIN_BARS, MAX_BARS);

    // if the number has changed we redraw the screen, otherwise it becomes messy
    
    if (old_num_Bars != num_Bars)
      newScreen();
      
  }  

  // In OPMODESPEED we change the speed of the display

  if (currentOpMode==OPMODESPEED)
  {
    draw_Delay -= adjust;
    if (draw_Delay < 0)
      draw_Delay=0;
  }  


  // In OPMODESCALE we change the Y-Axis-Scaling (5A, 1A, 500mA, 200mA etc.)

  if (currentOpMode==OPMODESCALE)
  {
    currentScaleMode += adjust;
    currentScaleMode = constrain(currentScaleMode,0,NUMSCALEMODES-1);
  }  

  // read the current values into a circular buffer

  readTheValues();

  // draw the values

  for (int i=0; i< num_Bars; i++)
  {

    // determine the current position in the circular memory
    
    int the_CurrentValue = bar_Counter -i ;
    if (the_CurrentValue < 0 ) the_CurrentValue += MAX_BARS;

    // the width of 1 bar is equal to the graph X-Size divided by the total number of visible bars (NUM_CHANNELS x num_Bars)

    float bar_Width = (graphArea[2] - graphArea[0]) / (NUM_CHANNELS * num_Bars);

    // see if we need to draw anything 

    if (i <= num_Bars)  // @@@ redundant?!?
    {

      // for each set of bars go through the different channels
      
      for (int j=0; j<NUM_CHANNELS; j++)
      {

        // the offset of one bar 
        // the latest value is at right, the older value to the left
        // the bargraph seems hence to float from right to left
        
        int bar_Offset = graphArea[2] + graphArea[0] * NUM_CHANNELS - ((i+1) * bar_Width*NUM_CHANNELS) + (j * bar_Width)- NUM_CHANNELS*i ;

        // determine the percentage of the bar height depending on MIN, MAX, Scale

        float percentage = (float)( all_Values[the_CurrentValue][j] - MIN_ADC_VALUE) / (float)(MAX_ADC_VALUE - MIN_ADC_VALUE);
        percentage /= scale_Mode[currentScaleMode];

        // if we are out of range we limit to MIN, MAX
        
        percentage = constrain(percentage,0.01,0.99);

        int bar_Height = (graphArea[3] - graphArea[1]) * percentage;

        // does the bar fit on the screen ?

        if (bar_Offset > 0)
        {
            // rather than erasing the whole screen which would result in blinking, we just draw a black bar where the old value might have been
          
            tft.fillRectangle( bar_Offset+1, graphArea[1],                bar_Offset + bar_Width-1,  graphArea[3] - bar_Height , COLOR_BLACK);

            // then we draw the bar as such
            
            tft.fillRectangle( bar_Offset+1, graphArea[3] - bar_Height  , bar_Offset + bar_Width-1,  graphArea [3], bar_Color[j]);
        }
      }
    }
  }
  
  // draw the status Bar

  drawStatusBar();

  // increase counter for next run
  
  if (bar_Counter < MAX_BARS -1 ) bar_Counter++; else bar_Counter=0;
  
  delay(draw_Delay*100);
  
}
