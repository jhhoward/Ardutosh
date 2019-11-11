#include <Arduboy2.h>
//#include <ArduboyTones.h>
#include <EEPROM.h>
#include <Keyboard.h>
#include <Mouse.h>

#define USE_GAMEPAD_REMOTE 0

#if USE_GAMEPAD_REMOTE
#include <Joystick.h>
#endif

#include "System.h"
#include "Platform.h"
#include "Defines.h"

Arduboy2Base arduboy;
//ArduboyTones sound(arduboy.audio.enabled);
Sprites sprites;

#if USE_GAMEPAD_REMOTE
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  2, 0,                  // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering
#endif

uint8_t Platform::GetInput()
{
  uint8_t result = 0;
  
  if(arduboy.pressed(A_BUTTON))
  {
    result |= INPUT_A;  
  }
  if(arduboy.pressed(B_BUTTON))
  {
    result |= INPUT_B;  
  }
  if(arduboy.pressed(UP_BUTTON))
  {
    result |= INPUT_UP;  
  }
  if(arduboy.pressed(DOWN_BUTTON))
  {
    result |= INPUT_DOWN;  
  }
  if(arduboy.pressed(LEFT_BUTTON))
  {
    result |= INPUT_LEFT;  
  }
  if(arduboy.pressed(RIGHT_BUTTON))
  {
    result |= INPUT_RIGHT;  
  }

  return result;
}

void Platform::PlaySound(const uint16_t* audioPattern)
{
//	sound.tones(audioPattern);
}

void Platform::SetLED(uint8_t r, uint8_t g, uint8_t b)
{
  arduboy.setRGBled(r, g, b);
}

void Platform::DrawPixel(int16_t x, int16_t y, uint8_t colour)
{
  arduboy.drawPixel(x, y, colour);
}

void Platform::DrawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t color)
{
	arduboy.drawFastVLine(x, y, h, color);
}

void Platform::DrawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t color)
{
	arduboy.drawFastHLine(x, y, w, color);
}

void Platform::FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
	arduboy.fillRect(x, y, w, h, color);
}

void Platform::DrawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color)
{
	arduboy.drawRect(x, y, w, h, color);
}


uint8_t* Platform::GetScreenBuffer()
{
  return arduboy.getBuffer();
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{
  sprites.drawPlusMask(x, y, bitmap, frame);
}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
  uint8_t w = pgm_read_byte(&bitmap[0]);
  uint8_t h = pgm_read_byte(&bitmap[1]);
  arduboy.drawBitmap(x, y, bitmap + 2, w, h);
}

void Platform::DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t colour)
{
	arduboy.drawLine(x1, y1, x2, y2, colour);
}

void Platform::FillScreen(uint8_t colour)
{
  arduboy.fillScreen(colour);
}

unsigned long lastTimingSample;

bool Platform::IsAudioEnabled()
{
	return arduboy.audio.enabled();
}

void Platform::SetAudioEnabled(bool isEnabled)
{
	if(isEnabled)
		arduboy.audio.on();
	else
		arduboy.audio.off();
}

void PlatformComm::SetBaud(uint32_t rate)
{
	Serial.flush();
	Serial.begin(rate);
}

bool PlatformComm::IsAvailable()
{
	return Serial.available();
}

void PlatformComm::Write(uint8_t data)
{
	Serial.write(data);
}

uint8_t PlatformComm::Read()
{
	return (uint8_t) Serial.read();
}

uint8_t PlatformStorage::GetByte(uint16_t address)
{
	return EEPROM.read(address);
}

void PlatformStorage::SetByte(uint16_t address, uint8_t value)
{
	return EEPROM.update(address, value);
}

#define ADC_TEMP (_BV(REFS0) | _BV(REFS1) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0))
#define ADC_VOLTAGE (_BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1))

uint16_t rawADC(uint8_t adc_bits)
{
  power_adc_enable(); // ADC on
  ADMUX = adc_bits;
  // we also need MUX5 for temperature check
  if (adc_bits == ADC_TEMP) {
    ADCSRB = _BV(MUX5);
  }
  else {
	ADCSRB &= ~_BV(MUX5); 
  }
  delay(2); // Wait for ADMUX setting to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  const uint16_t variable = ADC;
  power_adc_disable();
  return variable;
}

uint16_t Platform::GetBatteryVoltage()
{
	return 1126400L / rawADC(ADC_VOLTAGE);
}

int16_t Platform::GetTemperature()
{
	constexpr int offset = -7;
	return rawADC(ADC_TEMP) - 273 + offset;
}

void Platform::Reboot()
{
	void(* resetFunc) (void) = 0;
	resetFunc();
}

bool remoteKeyboardEnabled = false;
void PlatformRemote::SetKeyboardEnabled(bool enabled)
{
	if(enabled != remoteKeyboardEnabled)
	{
		if(enabled)
		{
			Keyboard.begin();
		}
		else
		{
			Keyboard.end();
		}
		remoteKeyboardEnabled = enabled;
	}
}
bool PlatformRemote::IsKeyboardEnabled()
{
	return remoteKeyboardEnabled;
}

bool remoteMouseEnabled = false;
void PlatformRemote::SetMouseEnabled(bool enabled)
{
	if(enabled)
	{
		Mouse.begin();
	}
	else
	{
		Mouse.end();
	}
	if(enabled != remoteMouseEnabled)
	{
		remoteMouseEnabled = enabled;
	}
}
bool PlatformRemote::IsMouseEnabled()
{
	return remoteMouseEnabled;
}

void PlatformRemote::KeyboardWrite(uint8_t data)
{
	Keyboard.write(data);
}

void PlatformRemote::MouseMove(int dirX, int dirY)
{
	Mouse.move(dirX, dirY);
}

void PlatformRemote::MouseDown()
{
	Mouse.press(MOUSE_LEFT);
}

void PlatformRemote::MouseUp()
{
	Mouse.release(MOUSE_LEFT);
}

bool remoteGamepadEnabled = false;
void PlatformRemote::SetGamepadEnabled(bool enabled)
{
#if USE_GAMEPAD_REMOTE
	if(enabled != remoteGamepadEnabled)
	{
		if(enabled)
		{
			Joystick.begin();
		}
		else
		{
			Joystick.end();
		}
		remoteGamepadEnabled = enabled;
	}
#endif
}
bool PlatformRemote::IsGamepadEnabled()
{
	return remoteGamepadEnabled;
}

void setup()
{
  arduboy.boot();
  arduboy.flashlight();
  arduboy.systemButtons();
  //arduboy.bootLogo();
  arduboy.setFrameRate(TARGET_FRAMERATE);

  //arduboy.audio.off();
  
  arduboy.setRGBled(0, 0, 0);
  
  Serial.begin(9600);

//  SeedRandom((uint16_t) arduboy.generateRandomSeed());
  System::Init();
  
  lastTimingSample = millis();
}

void loop()
{
  static int16_t tickAccum = 0;
  unsigned long timingSample = millis();
  tickAccum += (timingSample - lastTimingSample);
  lastTimingSample = timingSample;
  
#if USE_GAMEPAD_REMOTE
  if(remoteGamepadEnabled)
  {
	Joystick.setButton(0, arduboy.pressed(A_BUTTON));
	Joystick.setButton(1, arduboy.pressed(B_BUTTON));
	
	Joystick.setXAxisRange(-1, 1);
	Joystick.setYAxisRange(-1, 1);
	int dirX = 0, dirY = 0;
	if(arduboy.pressed(LEFT_BUTTON))
		dirX--;
	if(arduboy.pressed(RIGHT_BUTTON))
		dirX++;
	if(arduboy.pressed(UP_BUTTON))
		dirY--;
	if(arduboy.pressed(DOWN_BUTTON))
		dirY++;
	Joystick.setXAxis(dirX);
	Joystick.setYAxis(dirY);
  }
#endif
	
#if DEV_MODE
  if(arduboy.nextFrameDEV())
#else
  if(arduboy.nextFrame())
#endif
  {
	constexpr int16_t frameDuration = 1000 / TARGET_FRAMERATE;
	while(tickAccum > frameDuration)
	{
		System::Tick();
		tickAccum -= frameDuration;
	}
	
	System::Draw();
    
    //Serial.write(arduboy.getBuffer(), 128 * 64 / 8);

#if DEV_MODE
	// CPU load bar graph	
	int load = arduboy.cpuLoad();
	uint8_t* screenPtr = arduboy.getBuffer();
	
	for(int x = 0; x < load && x < 128; x++)
	{
		screenPtr[x] = (screenPtr[x] & 0xf8) | 3;
	}
	screenPtr[100] = 0;
#endif
	
    arduboy.display(false);
  }
}
