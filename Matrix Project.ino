#include <LedControl.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
enum gameState { GAME, SETTINGS, BEST_TIME, RESET, PLAYING, END_GAME

};
const int numOptions = 3;
int state = 0;
bool newMenu = true;

const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;
const byte lcdAnodePin = 3;
int lcdBrightness = 20;
int selection = 0;
long gameStartTime = 0;
long bestTime = 9999999999;
bool canHaveScore = false;


LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int pinSW = 2;
const int pinX = A0;
const int pinY = A1;

const int dinPin = 12;                                   
const int clockPin = 11;                                
const int loadPin = 10;    

const int buzzerPin = 13;

const int wallsNumber = 10;              
const int matrixSize = 8;   
const int matrixLength = matrixSize * matrixSize;

int playerPosition = 0;
bool isSpawned = 0;
int playerIsFacing = 0;
bool newScore = 0;

int freq = 220;

unsigned long playerBlinkTime = 100;
unsigned long lastPlayerBlink = 0;

bool isBlinking = 0;
const int minThreshold = 400;
const int maxThreshold = 900;
bool joyMoved = false;
bool joyIsNeutral = false;
int remainingWalls = 0;
int debounceDelay = 50;
int buttonState = HIGH;
int lastButtonState = HIGH;
int lastButtonPush = 0;
long gameTime = 0;
bool reloading = 0;
long reloadTime = 1000;
long startReload = 0;

bool soundIsOn = 0;
int soundDuration = 250;

bool gameIsStarted = 0;
int xValue = 0;
int yValue = 0;


bool walls[matrixSize*matrixSize];

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); 

void setup()
{
  pinMode(buzzerPin, OUTPUT);
  pinMode(pinX, INPUT);
  pinMode(pinY, INPUT);
  analogWrite(lcdAnodePin, lcdBrightness);
  lcd.begin(16, 2);
  EEPROM.update(0, 0);

  // Serial.begin(9600);

  lc.shutdown(0, false);                 
  lc.setIntensity(0, 8);                 
  lc.clearDisplay(0);  

  randomSeed(analogRead(0));

  pinMode(pinSW, INPUT_PULLUP);
}

void loop()
{
  if (state != PLAYING && state != END_GAME)
    navigateMenu();
  switch(state)
  {
    case GAME:
    { 
      if (newMenu)
      {
      lcd.clear();
      lcd.print("START GAME");
      newMenu = false;
      }
      buttonState = digitalRead(pinSW);
      if (buttonState == LOW && buttonState != lastButtonState)
      {
        spawnPlayer();
        generateWorld();
        state = PLAYING;
        gameStartTime = millis();
        canHaveScore = true;
        lcd.clear();
      }  
      lastButtonState = buttonState;
      break;
    }
    case SETTINGS:
    {
      if (newMenu)
    { 
      lcd.clear();
      if(soundIsOn)
        lcd.print("SOUND ON");
      else
        lcd.print("SOUND OFF");
      newMenu = false;
    }
      buttonState = digitalRead(pinSW);
      if(buttonState == LOW && buttonState != lastButtonState)
        {
          soundIsOn = !soundIsOn;
          newMenu = true;
        }
      break;
    }
    case BEST_TIME:
    {
      if (newMenu)
      {
        lcd.clear();
        if(canHaveScore)
        {
          lcd.print("BEST TIME: ");
          lcd.print(bestTime / 1000);
        }
        else 
          lcd.print("BEST TIME: 0");
        newMenu = false;
      }
      buttonState = digitalRead(pinSW);
      if(buttonState == LOW && buttonState != lastButtonState)
      {
        lcd.clear();
        state = RESET;
      }
      lastButtonState = buttonState;
      break;
    }
    case PLAYING:
      {
        lcd.clear();
        lcd.print((millis() - gameStartTime) / 1000);
        draw();
        movePlayer();
        shoot();
        for(int i = 0; i < matrixLength; i++)
          if(walls[i] == true)
            remainingWalls++;
        if(remainingWalls == 0)
        {
          state = END_GAME;
          updatebestTime(millis() - gameStartTime);
          lc.clearDisplay(0);
          newMenu = true;
        }
        else
          remainingWalls = 0;
        break;
      }
    case END_GAME:
      {
        if(newMenu)
        {
          lcd.clear();
          if(newScore)
            lcd.print("NEW BEST TIME!");
          else
            lcd.print("YOU WIN!");
          newMenu = false;
        }
        buttonState = digitalRead(pinSW);
        if (buttonState == LOW && buttonState != lastButtonState)
        {
          lcd.clear();
          state = GAME;
          newMenu = true;
        }
        lastButtonState = buttonState;
        break;
      }
    case RESET:
      {
        bestTime = 9999999999;
        if(newMenu)
        {
          lcd.clear();
          lcd.print("BEST TIME: 0");
          newMenu = false;
        }
        buttonState = digitalRead(pinSW);
        if(buttonState == LOW && buttonState != lastButtonState)
          {
            canHaveScore = false;
            state = BEST_TIME;
            newMenu = true;
          }
        lastButtonState = buttonState;
        break;
      }
  }
}



void navigateMenu()
{
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);

  if (xValue >= minThreshold && xValue <= maxThreshold && yValue >= minThreshold && yValue <= maxThreshold)
    joyIsNeutral = true;
  if (yValue > maxThreshold && joyIsNeutral && selection < numOptions - 1) 
  {
    newMenu = true;
    selection++;
    if(soundIsOn)
      tone(buzzerPin, freq, soundDuration);
    state = gameState{selection};
    joyIsNeutral = false;
  }
  if (yValue < minThreshold && joyIsNeutral && selection >= 0) 
  {
    if(soundIsOn)
      tone(buzzerPin, freq, soundDuration);
    newMenu = true;
    selection--;
    state = gameState{selection};
    joyIsNeutral = false;
  }
}

void updatebestTime(long score) {
  if (score < bestTime)
  {
    bestTime = score;
    newScore = true;
  }
  else
  newScore = false;
  EEPROM.update(0, bestTime);
}

void spawnPlayer()
{
  playerPosition = random(0, matrixLength);
}

void movePlayer()
{
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);
  int playerNextPosition = playerPosition;
  if (xValue >= minThreshold && xValue <= maxThreshold && yValue >= minThreshold && yValue <=maxThreshold) 
    joyIsNeutral = true;
  
  if(xValue > maxThreshold && joyIsNeutral)
  {
    if(playerPosition > matrixSize)
    {
      playerNextPosition -= matrixSize;
      joyIsNeutral = false;
    }
  }

  if(xValue < minThreshold && joyIsNeutral && playerPosition)
  {
    if(playerPosition < matrixLength - matrixSize)
      playerNextPosition += matrixSize;
      joyIsNeutral = false;
  }

  if(yValue > maxThreshold && joyIsNeutral )
  {
    if(playerPosition % matrixSize > 0)
    {
      playerNextPosition--;
      joyIsNeutral = false;
    }
  }

  if(yValue < minThreshold && joyIsNeutral)
  {
    if(playerPosition % matrixSize < matrixSize - 1)
    {
      playerNextPosition++;
      joyIsNeutral = false;  
    }
  }

  if (walls[playerNextPosition] == false )
    playerPosition = playerNextPosition;
}

void shoot()
{
  if (reloading)
  {
    if (millis() - startReload > reloadTime)
      reloading = 0;
  }
  else
  {
    buttonState = digitalRead(pinSW);
    if(millis() - lastButtonPush > debounceDelay)
    {
      if(buttonState == LOW && buttonState != lastButtonState)
      {
        walls[playerPosition + 1] = false;
        walls[playerPosition + matrixSize] = false;
        walls[playerPosition - 1] = false;
        reloading = true;
        startReload = millis();
        if(soundIsOn)
          tone(buzzerPin, freq, reloadTime);
      }
      lastButtonState = buttonState;
    }
  }
}

void generateWorld()
{
  int wallCount = 0;
  int off[5] = {0, 1, -1, matrixSize, -matrixSize};

  while(wallCount < wallsNumber)
  {
    int i = random(0, matrixLength);
    
    bool valid = !walls[i];

    for(int j = 0; j< 5; j++)
    {
      if(playerPosition == i + off[j])
      {
        valid = false;
        break;
      }    
    }
    if (valid)
    {
      walls[i] = true;
      wallCount++;
    } 
  }
}

void draw()
{
  while(millis() - lastPlayerBlink >= playerBlinkTime)
  {
    isBlinking = !isBlinking;
    lastPlayerBlink = millis();
  }

  for(int i = 0; i< matrixLength; i++)
    lc.setLed(0, i/matrixSize,matrixSize -1 - i%matrixSize,(i == playerPosition) ? isBlinking : walls[i]);
}