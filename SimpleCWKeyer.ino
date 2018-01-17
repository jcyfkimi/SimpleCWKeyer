// Simple Iambic Keyer v1.0.01
// Original written by Ernest PA3HCM, modified by BI4SQM
// Add DISPLAY support, currently support IIC LCD and OLED
#include <Arduino.h>

#define CALLSIGN    "BI4SQM"
#define VERSION     "V1.0.01"

#define _FEATURE_DISPLAY_
#ifdef _FEATURE_DISPLAY_
#define _SUPPORT_IIC_LCD_
//#define _SUPPORT_OLED_DISP_
#endif

#ifdef _SUPPORT_IIC_LCD_
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);
#elif defined _SUPPORT_OLED_DISP_
#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 11, /* data=*/ 10, /* cs=*/ 9, /* dc=*/ 8, /* reset=*/ 7);
#endif

#ifdef _FEATURE_DISPLAY_
enum lcd_statuses {
  LCD_CLEAR,
  LCD_REVERT,
  LCD_TIMED_MESSAGE,
  LCD_SCROLL_MSG
};
byte lcd_status = LCD_CLEAR;
unsigned long lcd_timed_message_clear_time = 0;
byte lcd_previous_status = LCD_CLEAR;
byte lcd_scroll_buffer_dirty = 0;
String lcd_scroll_buffer[LCD_ROWS];
byte lcd_scroll_flag = 0;
byte lcd_paddle_echo = 1;
byte lcd_send_echo = 1;
#endif

#define P_DOT    2   // Connects to the dot lever of the paddle
#define P_DASH   3   // Connects to the dash lever of the paddle
#define P_AUDIO 12   // Audio output
#define P_CW    13   // Output of the keyer, connect to your radio
#define P_SPEED A0   // Attached to center pin of potmeter, allows you
// to set the keying speed.

//Here defines the mapping of WPM range from pot value, if you want to change the WPM, just change the value of MAX_WPM and MIN_WPM
#define MAX_WPM       30
#define MIN_WPM       10
#define MAX_POT_READ  1023
#define MIN_POT_READ  0

long starttimehigh;
long highduration;
long lasthighduration;
long hightimesavg;
long lowtimesavg;
long startttimelow;
long lowduration;
long laststarttime = 0;

byte code[20];
int stop = LOW;
int old_wpm;
int wpm;
int pot_read;
// This is indicated the dit time based on T = 1200/WPM
// For more deatils please refer https://en.wikipedia.org/wiki/Morse_code section 4.5 Speed in words per minute.
int dit_time; 

#ifdef _SUPPORT_IIC_LCD_
int row_0_cur = 0;
int row_1_cur = 0;
#endif

#ifdef _FEATURE_DISPLAY_
#ifdef _SUPPORT_IIC_LCD_
void lcd_center_print_timed(String lcd_print_string, byte row_number, unsigned int duration)
{
  long cur_ms = 0;
  if (lcd_status != LCD_TIMED_MESSAGE) {
    lcd_previous_status = lcd_status;
    lcd_status = LCD_TIMED_MESSAGE;
    lcd.clear();
  } else {
    clear_display_row(row_number);
  }
  lcd.setCursor(((LCD_COLUMNS - lcd_print_string.length()) / 2), row_number);
  lcd.print(lcd_print_string);
  cur_ms = millis();
  lcd_timed_message_clear_time = cur_ms + duration;
  while (cur_ms < lcd_timed_message_clear_time)
  {
    cur_ms = millis();
    delay(1);
  }
  clear_display_row(row_number);
}
void clear_display_row(byte row_number)
{
  for (byte x = 0; x < LCD_COLUMNS; x++) {
    lcd.setCursor(x, row_number);
    lcd.print(" ");
  }
}
#endif
#endif

// Initializing the Arduino
void setup()
{
  pinMode(P_DOT, INPUT);
  pinMode(P_DASH, INPUT);
  pinMode(P_AUDIO, OUTPUT);
  pinMode(P_CW, OUTPUT);
  digitalWrite(P_CW, LOW);      // Start with key up
#ifdef  _FEATURE_DISPLAY_
#ifdef _SUPPORT_IIC_LCD_
  lcd.init();                      // initialize the lcd
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  byte info_buf_line1[16];
  byte ver_buf_line2[16];
  memset(info_buf_line1, 0, sizeof(info_buf_line1));
  memset(ver_buf_line2, 0, sizeof(ver_buf_line2));
  sprintf(info_buf_line1, "%s's Keyer", CALLSIGN);
  sprintf(ver_buf_line2, "Ver:%s", VERSION);
  lcd_center_print_timed(info_buf_line1, 0, 4000);
  lcd_center_print_timed(ver_buf_line2, 1, 4000);
#endif
#ifdef _SUPPORT_OLED_DISP_
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(0, 24, "BI4SQM");
  u8g2.sendBuffer();
#endif
#endif
}

// Main routine
void loop()
{
  pot_read = analogRead(P_SPEED); // Read the keying speed from potmeter
  wpm = map(pot_read, MIN_POT_READ, MAX_POT_READ, MIN_WPM, MAX_WPM);
  dit_time = 1200 / wpm;
#ifdef _FEATURE_DISPLAY_
  if (old_wpm != wpm)
  {
    update_wpm_info(wpm);
    old_wpm = wpm;
  }
#endif
  if (!digitalRead(P_DOT))
  {
    delay(10);
    if (!digitalRead(P_DOT))
    {
      disp_show(".");
      keyAndBeep(dit_time);           // ... send a dot at the given speed
      delay(dit_time);                //     and wait before sending next
    }
  }
  if (!digitalRead(P_DASH))      // If the dash lever is pressed...
  {
    delay(10);
    if (!digitalRead(P_DASH))
    {
      disp_show("-");
      keyAndBeep(dit_time * 3);       // ... send a dash at the given speed
      delay(dit_time);                //     and wait before sending next
    }
  }
}

// Key the transmitter and sound a beep
void keyAndBeep(int dur)
{
  digitalWrite(P_CW, HIGH);            // Key down
  for (int i = 0; i < (dur / 2); i++) // Beep loop
  {
    digitalWrite(P_AUDIO, HIGH);
    delay(1);
    digitalWrite(P_AUDIO, LOW);
    delay(1);
  }
  digitalWrite(P_CW, LOW);             // Key up
}
#ifdef _FEATURE_DISPLAY_
void disp_show(char *str)
{
#ifdef _SUPPORT_IIC_LCD_
  if(0 == row_0_cur)
  {
    lcd.setCursor(0,0);
    lcd.print("[");
    row_0_cur++;
  }
  lcd.setCursor(row_0_cur, 0);
  lcd.print(str);
  row_0_cur++;
  if(6 == row_0_cur)
  {
    lcd.setCursor(6, 0);
    lcd.print("]");
    row_0_cur = 0;
  }
#endif
#ifdef _SUPPORT_OLED_DISP_
  u8g2.clearBuffer();
  u8g2.drawStr(0, 24, str);
  u8g2.sendBuffer();
#endif
}
void update_wpm_info(int wpm_show)
{
#ifdef _SUPPORT_IIC_LCD_
  lcd.setCursor((LCD_COLUMNS - 6), 0);
  lcd.print("WPM:");
  lcd.print(wpm_show);
#endif
}
#endif

