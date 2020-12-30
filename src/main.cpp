#include <Arduino.h>
#include "Joystick.h"
#include "DigitalWriteFast.h"
#include "YokeConfig.h"
#include "ConfigManager.h"
#include "PID_V2.h"

#ifdef _VARIANT_ARDUINO_DUE_X_
#define Serial  SerialUSB
#endif

#define USING_DAC

#ifdef USING_DAC
#include "DAC8563.h"
DAC8563 pwm=DAC8563(CS_PIN);
#else
#include "PWM.h"
_Pwm pwm;
#endif

#include "QEncoder.h"
QEncoder encoder; 

int32_t xy_force[2] = {0,0};
 //int32_t last_xy_force[2] = {0,0};

double Setpoint[2], Input[2], Output[2];
//double Kp=2, Ki=5, Kd=1;
//double aggKp=4, aggKi=0.2, aggKd=1;
 double Kp[2] = {KP,KP};
 double Ki[2] = {KI,KI};
 double Kd[2] = {KD,KD};

PID myPID[]={PID(&Input[X_AXIS], &Output[X_AXIS], &Setpoint[X_AXIS], Kp[X_AXIS], Ki[X_AXIS], Kd[X_AXIS], DIRECT), PID(&Input[Y_AXIS], &Output[Y_AXIS], &Setpoint[Y_AXIS], Kp[Y_AXIS], Ki[Y_AXIS], Kd[Y_AXIS], DIRECT)};

//PID myPID_X(&Input[X_AXIS], &Output[X_AXIS], &Setpoint[X_AXIS], Kp[X_AXIS], Ki[X_AXIS], Kd[X_AXIS], DIRECT);
//PID myPID_Y(&Input[Y_AXIS], &Output[Y_AXIS], &Setpoint[Y_AXIS], Kp[Y_AXIS], Ki[Y_AXIS], Kd[Y_AXIS], DIRECT);

volatile long debouncing_time = DEBOUNCE_TIME; //Debouncing Time in Milliseconds

volatile bool initialRun = false;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_JOYSTICK,
1, 0, // Button Count, Hat Switch Count
true, true, false, // X and Y, but no Z Axis
false, false, false, // No Rx, Ry, or Rz
false, false, // No rudder or throttle
false, false, false); // No accelerator, brake, or steering

BUTTONS Buttons; 

GainsConfig gains_cf[2];
PidsConfig  pids_cf[2];
System_Control sys_cf;
 
Gains gain[2];
EffectParams effects[2];

ConfigManager Cfg_Mangager = ConfigManager();

void calculateEncoderPostion(int idx);
void YokeSetGains();
void SetEffects();
void gotoPosition(int idx, int32_t targetPosition);
void CalculateMaxSpeedAndMaxAcceleration(int idx);
void findCenter(int idx);
void Push_Button_01_ISR();
void Update_Joystick_Buttons();

//void calculateEncoderPostion_X();
//void calculateEncoderPostion_Y();


void setup() {
 
  Buttons.pinNumber = PUSH_BUTTON_01;
  Buttons.CurrentState = HIGH;
  Buttons.LastState = HIGH;
  Buttons.millis_time = millis();
  pinMode(Buttons.pinNumber,INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(Buttons.pinNumber), Push_Button_01_ISR, CHANGE);

  /*
  attachInterrupt(digitalPinToInterrupt(encoderPin_XA), calculateEncoderPostion_X, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPin_XB), calculateEncoderPostion_X, CHANGE);  
  attachInterrupt(digitalPinToInterrupt(encoderPin_YA), calculateEncoderPostion_Y, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPin_YB), calculateEncoderPostion_Y, CHANGE); 
  */ 
 
  delay(200);
  pwm.begin();
  //delay(200);
  pwm.setPWM(X_AXIS,0);  
  pwm.setPWM(Y_AXIS,0);
  pwm.servo_off(X_AXIS);
  pwm.servo_off(Y_AXIS);
  
  for (int i = 0; i < 2; i++)
  {
    Input[i] = encoder.axis[i].currentPosition;
    myPID[i].SetMode(AUTOMATIC);
    myPID[i].SetSampleTime(PID_SAMPLE_TIME);
    myPID[i].SetOutputLimits(-PID_OUTPUT_LIMIT, PID_OUTPUT_LIMIT);
  }
  
  Serial.begin(BAUD_RATE);

  Joystick.begin(true);
  YokeSetGains();

  if (!Buttons.CurrentState)
  {
    initialRun = true;
  }
  
}

void loop() {
  
  
  if (initialRun == true ) {
//    position control is not correctly, wheel runs over disired postion serveral times before stop 
    findCenter(X_AXIS);
    delay(1000);
    findCenter(Y_AXIS);
    initialRun = false;
  } else

  {
    encoder.updatePosition(X_AXIS);

    if (encoder.axis[X_AXIS].currentPosition > encoder.axis[X_AXIS].maxValue) {
      Joystick.setXAxis(encoder.axis[X_AXIS].maxValue);
    } else if (encoder.axis[X_AXIS].currentPosition < encoder.axis[X_AXIS].minValue) {
      Joystick.setXAxis(encoder.axis[X_AXIS].minValue);
    } else {
      Joystick.setXAxis(encoder.axis[X_AXIS].currentPosition);
    }

    encoder.updatePosition(Y_AXIS);
    if (encoder.axis[Y_AXIS].currentPosition > encoder.axis[Y_AXIS].maxValue) {
      Joystick.setYAxis(encoder.axis[Y_AXIS].maxValue);
    } else if (encoder.axis[Y_AXIS].currentPosition < encoder.axis[Y_AXIS].minValue) {
      Joystick.setYAxis(encoder.axis[Y_AXIS].minValue);
    } else {
      Joystick.setYAxis(encoder.axis[Y_AXIS].currentPosition);
        
    }


    SetEffects();
    Joystick.getForce(xy_force);
    xy_force[X_AXIS] = constrain(xy_force[X_AXIS], -255, 255);
    xy_force[Y_AXIS] = constrain(xy_force[Y_AXIS], -255, 255);
    
    if (encoder.axis[X_AXIS].currentPosition >= encoder.axis[X_AXIS].maxValue) {
      xy_force[X_AXIS] = 255;
    } else if (encoder.axis[X_AXIS].currentPosition <= encoder.axis[X_AXIS].minValue) {
      xy_force[X_AXIS] = -255;
    }
    if (encoder.axis[Y_AXIS].currentPosition >= encoder.axis[Y_AXIS].maxValue) {
      xy_force[Y_AXIS] = 255;
    } else if (encoder.axis[Y_AXIS].currentPosition <= encoder.axis[Y_AXIS].minValue) {
      xy_force[Y_AXIS] = -255;
    }
      
  }

  pwm.setPWM(X_AXIS,xy_force[X_AXIS]);
  CalculateMaxSpeedAndMaxAcceleration(X_AXIS);
  pwm.setPWM(Y_AXIS,xy_force[Y_AXIS]);
  CalculateMaxSpeedAndMaxAcceleration(Y_AXIS);
  Update_Joystick_Buttons();
  Cfg_Mangager.GetUpdate(gains_cf,pids_cf, sys_cf);

}


void Update_Joystick_Buttons()
{

      Joystick.setButton(0, !Buttons.CurrentState);
     

}

void SetEffects()
{
effects[X_AXIS].springPosition = encoder.axis[X_AXIS].currentPosition;
effects[Y_AXIS].springPosition = encoder.axis[Y_AXIS].currentPosition;
effects[X_AXIS].springMaxPosition = encoder.axis[X_AXIS].maxValue/2;
effects[Y_AXIS].springMaxPosition = encoder.axis[Y_AXIS].maxValue/2;

effects[X_AXIS].frictionPositionChange = encoder.axis[X_AXIS].positionChange; //lastX - posX;
effects[Y_AXIS].frictionPositionChange = encoder.axis[Y_AXIS].positionChange; //lastY - posY;
effects[X_AXIS].frictionMaxPositionChange = encoder.axis[X_AXIS].maxPositionChange;
effects[Y_AXIS].frictionMaxPositionChange = encoder.axis[Y_AXIS].maxPositionChange;

effects[X_AXIS].inertiaAcceleration = encoder.axis[X_AXIS].currentPosition;;
effects[Y_AXIS].inertiaAcceleration = encoder.axis[Y_AXIS].currentPosition;;
effects[X_AXIS].inertiaMaxAcceleration = encoder.axis[X_AXIS].maxAcceleration;
effects[Y_AXIS].inertiaMaxAcceleration = encoder.axis[Y_AXIS].maxAcceleration;

effects[X_AXIS].damperVelocity=encoder.axis[X_AXIS].currentVelocity;
effects[Y_AXIS].damperVelocity=encoder.axis[Y_AXIS].currentVelocity;
effects[X_AXIS].damperMaxVelocity = encoder.axis[X_AXIS].maxVelocity;
effects[Y_AXIS].damperMaxVelocity = encoder.axis[Y_AXIS].maxVelocity;

Joystick.setEffectParams(effects);

}   

void YokeSetGains()
{
  memcpy(gain,gains_cf,sizeof(gains_cf));

/*
//set x axis gains
gain[X_AXIS].totalGain = gains_cf[X_AXIS].ToBytes[0];
gain[X_AXIS].constantGain = gains_cf[X_AXIS].ToBytes[1];
gain[X_AXIS].rampGain = gains_cf[X_AXIS].ToBytes[2];
gain[X_AXIS].squareGain = gains_cf[X_AXIS].ToBytes[3];
gain[X_AXIS].sineGain = gains_cf[X_AXIS].ToBytes[4];
gain[X_AXIS].triangleGain = gains_cf[X_AXIS].ToBytes[5];
gain[X_AXIS].sawtoothdownGain = gains_cf[X_AXIS].ToBytes[6];
gain[X_AXIS].sawtoothupGain = gains_cf[X_AXIS].ToBytes[7];
gain[X_AXIS].springGain = gains_cf[X_AXIS].ToBytes[8];
gain[X_AXIS].damperGain = gains_cf[X_AXIS].ToBytes[9];
gain[X_AXIS].inertiaGain = gains_cf[X_AXIS].ToBytes[10];
gain[X_AXIS].frictionGain = gains_cf[X_AXIS].ToBytes[11];
gain[X_AXIS].customGain = gains_cf[X_AXIS].ToBytes[12];

//set y axis gains
gain[Y_AXIS].totalGain = gains_cf[Y_AXIS].ToBytes[0];
gain[Y_AXIS].constantGain = gains_cf[Y_AXIS].ToBytes[1];
gain[Y_AXIS].rampGain = gains_cf[Y_AXIS].ToBytes[2];
gain[Y_AXIS].squareGain = gains_cf[Y_AXIS].ToBytes[3];
gain[Y_AXIS].sineGain = gains_cf[Y_AXIS].ToBytes[4];
gain[Y_AXIS].triangleGain = gains_cf[Y_AXIS].ToBytes[5];
gain[Y_AXIS].sawtoothdownGain = gains_cf[Y_AXIS].ToBytes[6];
gain[Y_AXIS].sawtoothupGain = gains_cf[Y_AXIS].ToBytes[7];
gain[Y_AXIS].springGain = gains_cf[Y_AXIS].ToBytes[8];
gain[Y_AXIS].damperGain = gains_cf[Y_AXIS].ToBytes[9];
gain[Y_AXIS].inertiaGain = gains_cf[Y_AXIS].ToBytes[10];
gain[Y_AXIS].frictionGain = gains_cf[Y_AXIS].ToBytes[11];
gain[Y_AXIS].customGain = gains_cf[Y_AXIS].ToBytes[12];
*/

Joystick.setGains(gain);

}

/*
void calculateEncoderPostion_X() {
  encoder.tick_X();
}

void calculateEncoderPostion_Y() {
  encoder.tick_Y();
}
*/

void Push_Button_01_ISR()
{
  int bState = digitalReadFast(Buttons.pinNumber);
  if(Buttons.LastState != bState )
  {
     if((long)(millis() - Buttons.millis_time) > debouncing_time ) {

        Buttons.CurrentState = bState;
        Buttons.LastState = Buttons.CurrentState; 
        Buttons.millis_time = millis();
    }
  }
      
}

void AutoCalibration(uint8_t idx)
{
    
    if (encoder.axis[idx].currentPosition < encoder.axis[idx].minValue)
    {
      encoder.axis[idx].minValue = encoder.axis[idx].currentPosition;
    }
    if (encoder.axis[idx].currentPosition > encoder.axis[idx].maxValue)
    {
      encoder.axis[idx].maxValue = encoder.axis[idx].currentPosition;
    }

}

void Reset_Encoder(int idx)
{
     
      encoder.axis[idx].currentPosition=0;
      encoder.updatePosition(idx);

}

void findCenter(int idx)
{
  char buff[48];
  int32_t LastPos=0, Axis_Center=0 ,Axis_Range =0;
 
  encoder.axis[idx].minValue =0;
  encoder.axis[idx].maxValue =0;
  Reset_Encoder(idx);

  pwm.servo_on(idx);
   delay(2000);
  //Serial.println("Find Center");
  while (Buttons.CurrentState)
  {
    encoder.updatePosition(idx);
    if(LastPos != encoder.axis[idx].currentPosition)
    {
    AutoCalibration(idx);
    LastPos = encoder.axis[idx].currentPosition;
    sprintf(buff,"[%d]: %ld,%ld", idx, encoder.axis[idx].minValue, encoder.axis[idx].maxValue);
    Serial.println(buff);
    }
  }
    Axis_Center= (encoder.axis[idx].minValue + encoder.axis[idx].maxValue)/2;
    Axis_Range =  abs(encoder.axis[idx].minValue) + abs(encoder.axis[idx].maxValue);
    encoder.axis[idx].maxValue = Axis_Range/2 -20;
    encoder.axis[idx].minValue = -encoder.axis[idx].maxValue;
    gotoPosition(idx, Axis_Center);    //goto center X
    Reset_Encoder(idx);
    sprintf(buff,"[%d]: %ld - 0 - %ld", idx, encoder.axis[idx].minValue, encoder.axis[idx].maxValue);
    Serial.println(buff);
    switch (idx)
    {
    case 0:
      Joystick.setXAxisRange(encoder.axis[idx].minValue, encoder.axis[idx].maxValue);
      Joystick.setXAxis(encoder.axis[idx].currentPosition);
      break;
    case 1:
      Joystick.setYAxisRange(encoder.axis[idx].minValue, encoder.axis[idx].maxValue);
      Joystick.setYAxis(encoder.axis[idx].currentPosition);
      break;
    default:
      break;
    }

    pwm.setPWM(idx, 0);
}


void gotoPosition(int idx, int32_t targetPosition) {
  int32_t LastPos=0;
  char buff[64];
  Setpoint[idx] = targetPosition;
  while (encoder.axis[idx].currentPosition != targetPosition) {
    Setpoint[idx] = targetPosition;
    encoder.updatePosition(idx);
    Input[idx] = encoder.axis[idx].currentPosition ;
    myPID[idx].Compute();
    pwm.setPWM(idx, -Output[idx]);
    CalculateMaxSpeedAndMaxAcceleration(idx);
    if (LastPos !=encoder.axis[idx].currentPosition )
    {
    sprintf(buff,"[%d] P: %ld,T: %ld,F: %d",idx,encoder.axis[idx].currentPosition, (int32_t)Setpoint[idx], (int)Output[idx] );
    Serial.println(buff);
    LastPos = encoder.axis[idx].currentPosition;
    }
    
  }
  
}

void CalculateMaxSpeedAndMaxAcceleration(int idx) {
  if (encoder.axis[idx].maxVelocity < abs(encoder.axis[idx].currentVelocity)) {
    encoder.axis[idx].maxVelocity = abs(encoder.axis[idx].currentVelocity);
  }
  if (encoder.axis[idx].maxAcceleration < abs(encoder.axis[idx].currentAcceleration)) {
    encoder.axis[idx].maxAcceleration = abs(encoder.axis[idx].currentAcceleration);
  }
  if (encoder.axis[idx].maxPositionChange < abs(encoder.axis[idx].positionChange)) {
    encoder.axis[idx].maxPositionChange = abs(encoder.axis[idx].positionChange);
  }
}
