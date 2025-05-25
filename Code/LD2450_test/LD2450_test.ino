#include <LD2450.h>

#define RX 16
#define TX 17


// SENSOR INSTANCE
LD2450 ld2450;

void initRadar(){
  Serial1.begin(256000, SERIAL_8N1, RX, TX);
}

void setup()
{
  //SERIAL FOR HOST / DEBUG MESSAGES
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  initRadar();
  
  Serial.println("SETUP_STARTED");
  // SETUP SENSOR
  // HERE THE HARDWARE SERIAL INTERFACE 1 IS USED
  ld2450.begin(Serial1, false);


  
  if(!ld2450.waitForSensorMessage()){
    Serial.println("SENSOR CONNECTION SEEMS OK");
  }else{
    Serial.println("SENSOR TEST: GOT NO VALID SENSORDATA - PLEASE CHECK CONNECTION!");
  }

  Serial.println("SETUP_FINISHED");
  
}

void loop()
{

  // READ FUNCTION MUST BE CALLED IN LOOP TO READ THE INCOMMING DATA STREAM
  // RETURNS -1 or -2 as error flag and 0 to getSensorSupportedTargetCount() if valid targets found
  const int sensor_got_valid_targets = ld2450.read();
  if (sensor_got_valid_targets > 0)
  {

    /*
    PRINT DEBUG DATA STREAM LIKE THIS: 
    TARGET ID=1 X=-19mm, Y=496mm, SPEED=0cm/s, RESOLUTION=360mm, DISTANCE=496mm, VALID=1
    TARGET ID=2 X=-1078mm, Y=1370mm, SPEED=0cm/s, RESOLUTION=360mm, DISTANCE=1743mm, VALID=1
    TARGET ID=3 X=0mm, Y=0mm, SPEED=0cm/s, RESOLUTION=0mm, DISTANCE=0mm, VALID=0
    */
    Serial.print(ld2450.getLastTargetMessage());



    // GET THE DETECTED TARGETS
    // TARGET RANGE CAN BE FROM 0 TO ld2450.getSensorSupportedTargetCount(), DEPENDS ON SENSOR HARDWARE. REFER TO LD2450 DATASHEET
    for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++)
    {
      const LD2450::RadarTarget result_target = ld2450.getTarget(i);
      //CHECK IF THE TARGET IS MOVING
      // SEE LD2450.h RadarTarget FOR ALL POSSIBLE TARGET DATA SUCH AS X, Y POSITION, DISTANCE,...
      if (result_target.valid && abs(result_target.speed) > 0) // SENSOR SUPPORTS NEGATIVE SPEED IF MOVING TOWARDS SENSOR
      {
        Serial.println("TARGET DETECTED");
        Serial.println(result_target.x);
      }
      else
      {

      }
    }
  };
}
