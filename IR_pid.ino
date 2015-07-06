#include <avr/EEPROM.h>
#include <avr/interrupt.h>
#include <phys253.h>
#include <LiquidCrystal.h>

// Sensor Ports
#define IR_L 0
#define IR_R 1
#define LEFT_QRD 2
#define RIGHT_QRD 3
#define LEFT_MOTOR 3
#define RIGHT_MOTOR 0

class MenuItem
{
public:
	String    Name;
	uint16_t  Value;
	uint16_t* EEPROMAddress;
	static uint16_t MenuItemCount;
	MenuItem(String name)
	{
  	MenuItemCount++;
  	EEPROMAddress = (uint16_t*)(2 * MenuItemCount);
  	Name = name;
  	Value = eeprom_read_word(EEPROMAddress);
	}
	void Save()
	{
	  eeprom_write_word(EEPROMAddress, Value);
	}
};
 
uint16_t MenuItem::MenuItemCount = 0;
/* Add the menu items */
MenuItem Speed            = MenuItem("Speed");
MenuItem ProportionalGain = MenuItem("P-gain");
MenuItem DerivativeGain   = MenuItem("D-gain");
MenuItem IntegralGain     = MenuItem("I-gain");
MenuItem ThresholdVoltage = MenuItem("T-volt");
MenuItem menuItems[]      = {Speed, ProportionalGain, DerivativeGain, IntegralGain, ThresholdVoltage};

int value = 0;
int count = 0;
int average;
int difference;
int left_sensor;
int right_sensor;

int error;
int last_error = 0;
int recent_error = 0;
int P_error;
int D_error;
int I_error;
int net_error;
int t = 1;
int to;

int pro_gain;
int diff_gain;
int int_gain;
int base_speed;
int threshold;

void setup()
{
  #include <phys253setup.txt>
  LCD.clear(); LCD.home();
  
  base_speed = menuItems[0].Value;
  pro_gain = menuItems[1].Value;
  diff_gain = menuItems[2].Value;
  int_gain = menuItems[3].Value;
  threshold = menuItems[4].Value;
  
  LCD.print("Press Start.");
  while(!startbutton()){};
  LCD.clear();
}

void loop()
{        
        
  if (startbutton() && stopbutton()){          
    // Pause motors
    motor.speed(LEFT_MOTOR, 0);
    motor.speed(RIGHT_MOTOR, 0);
    Menu();
    // Set values after exiting menu
    base_speed = menuItems[0].Value;
    pro_gain = menuItems[1].Value;
    diff_gain = menuItems[2].Value;
    int_gain = menuItems[3].Value;
    threshold = menuItems[4].Value;
    // Restart motors
    motor.speed(LEFT_MOTOR, base_speed);
    motor.speed(RIGHT_MOTOR, base_speed);
  }

  // PID control
  left_sensor = analogRead(IR_L);
  right_sensor = analogRead(IR_R);
  difference = left_sensor - right_sensor;
  average = (left_sensor + right_sensor) >> 1;
  error = difference;

  // Differential control
  if( !(error - last_error < threshold)){
    recent_error = last_error;
    to = t;
    t = 1;
  }
  
  P_error = pro_gain * error;
  D_error = diff_gain * ((float)(error - recent_error)/(float)(t+to)); // time is present within the differential gain
  I_error += int_gain * error;
  net_error = ((P_error + D_error + I_error) * average) >> 10; // divide by 1024
  
  //if net error is positive, right_motor will be stronger, will turn to the left
  //motor.speed(LEFT_MOTOR, base_speed + net_error);
  //motor.speed(RIGHT_MOTOR, base_speed - net_error);
  
  if( count == 100 ){
    count = 0;
    LCD.clear(); LCD.home();
    LCD.print("L:"); LCD.print(left_sensor);
    LCD.print(" R:");LCD.print(right_sensor);
    LCD.setCursor(0, 1);
    LCD.print("ERR:"); LCD.print(net_error);
  }
  
  last_error = error;
  count++;
  t++;
}
 
void Menu()
{
	LCD.clear(); LCD.home();
	LCD.print("Entering menu");
	delay(500);
 
	while (true)
	{
		/* Show MenuItem value and knob value */
		int menuIndex = knob(6) * (MenuItem::MenuItemCount) >> 10;
		LCD.clear(); LCD.home();
		LCD.print(menuItems[menuIndex].Name); LCD.print(" "); LCD.print(menuItems[menuIndex].Value);
		LCD.setCursor(0, 1);
		LCD.print("Set to "); LCD.print(menuIndex != 0 ? knob(7) : knob(7) >> 2); LCD.print("?");
		delay(100);
 
		/* Press start button to save the new value */
		if (startbutton())
		{
			delay(100);
      int val = knob(7); // cache knob value to memory
      // Limit speed to 700
      if (menuIndex == 0) {
        val = val >> 2;
        LCD.clear(); LCD.home();
        LCD.print("Speed set to "); LCD.print(val);
        delay(250);
      }
      
			menuItems[menuIndex].Value = val;
			menuItems[menuIndex].Save();
			delay(250);

			}
		}
 
		/* Press stop button to exit menu */
		if (stopbutton())
		{
			delay(100);
			if (stopbutton())
			{
			  LCD.clear(); LCD.home();
			  LCD.print("Leaving menu");
			  delay(500);
			  return;
			}
		}
	}
