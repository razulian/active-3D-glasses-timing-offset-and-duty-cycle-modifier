#define SHUTTERHIGHSIDE 4
#define SHUTTERLOWSIDE 5

#define SIGINA 16
int displaySignalA = 0;
long oldMillis;

void setup()
{
  pinMode(SHUTTERHIGHSIDE, OUTPUT);
  pinMode(SHUTTERLOWSIDE, OUTPUT);
  pinMode(SIGINA, INPUT);
  oldMillis = millis();

  Serial.begin(115200);
}

int offTime = 12000;
int onTime = 4600;

bool triggered = false;
long currentTime = 0;
long triggerTime = 0;
long lastTriggerTime = 0;
long shutterOnDuration = 0;
long shutterOffDuration = 0;
float shutterDutyCycle = 0.7;

int sigPulseDuration = 0;
bool lastSigInputState = false;
bool currentSigInputState = false;
int shutterPhase = 0;
long lastShutterTime = 0;
bool shutterCycleInProgress = false;
long shutterCycleActivationTime = 0;
long lastShutterCycleActivationTime = 0;
int forcedShutterOn = 0;

int offset = 9000; // 10300 channel A, 17100 channel B

void loop()
{
  if (Serial.available() > 0)
  {
    int incomingByte = Serial.read();
    // Serial.println(incomingByte);
    switch (incomingByte)
    {
      case 97: // a 97
        offset = offset + 250;

        Serial.print("    TimingOffset: ");
        Serial.println(offset);
        break;
      case 115: // s
        shutterDutyCycle += 0.02;
        Serial.print("ShutterDutyCycle: ");
        Serial.print(shutterDutyCycle);
        break;
      case 113: // q
        forcedShutterOn++;
        if (forcedShutterOn > 2)
          forcedShutterOn = 0;
        switch (forcedShutterOn)
        {
          case 0:
            Serial.println("Shutter Forced disabled");
            break;
          case 1:
            Serial.println("Forced Mode: shutter on");
            break;
          case 2:
            Serial.println("Forced Mode: shutter off");
            break;
        }
        break;
    }

    if (offset > 9250)
      offset = 9000;
    if (shutterDutyCycle > 1)
      shutterDutyCycle = 0.5;
  }

  currentTime = micros();
  currentSigInputState = digitalRead(SIGINA);

  if (lastSigInputState != currentSigInputState)
  {
    triggerTime = currentTime;
    if (triggerTime - lastTriggerTime <= 16000)
    {

      Serial.println("Signal Exception Caught");
      //sigPulseDuration = (triggerTime - lastTriggerTime);
     // lastTriggerTime = triggerTime + sigPulseDuration;
    }
    else
    {
      sigPulseDuration = (triggerTime - lastTriggerTime);
      lastTriggerTime = triggerTime;
      shutterOnDuration = sigPulseDuration * shutterDutyCycle;
      shutterOffDuration = sigPulseDuration * (1 - shutterDutyCycle);
      if (currentSigInputState && !forcedShutterOn)
      {
        shutterCycleActivationTime = triggerTime;
        triggered = true;
      }
    }

    lastSigInputState = currentSigInputState;
  }

  if (triggered == true && shutterCycleInProgress == false && currentTime - shutterCycleActivationTime - offset >= 0)
  {
    triggered = false;
    lastShutterCycleActivationTime = currentTime;
    shutterCycleInProgress = true;
  }

  if (forcedShutterOn == 1)
  {
    shutterOff();
  }
  if (forcedShutterOn == 2)
  {
    shutterOn();
  }

  if (shutterCycleInProgress == true)
  {

    switch (shutterPhase)
    {
      case 0:
        shutterOff();
        shutterPhase++;
        lastShutterTime = currentTime;
        break;
      case 1:
        if (currentTime - lastShutterTime >= shutterOnDuration)
        {
          shutterOn();
          shutterPhase++;
          lastShutterTime = currentTime;
        }
        break;
      case 2:
        if (currentTime - lastShutterTime >= shutterOffDuration)
        {
          shutterOff(); // off
          shutterPhase++;
          lastShutterTime = currentTime;
        }
        break;
      case 3:
        if (currentTime - lastShutterTime >= shutterOnDuration)
        {
          shutterOn();
          shutterPhase = 0;
          shutterCycleInProgress = false;
        }
        break;
    }
  }
}

void shutterOff()
{
  digitalWrite(SHUTTERHIGHSIDE, HIGH);
  digitalWrite(SHUTTERLOWSIDE, LOW);
}

void shutterOn()
{
  digitalWrite(SHUTTERLOWSIDE, HIGH);
  digitalWrite(SHUTTERHIGHSIDE, LOW);
}
