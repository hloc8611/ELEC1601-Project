 //-----------------------------------------------------------------------------------------------------------//
//                                                                                                           //
//  Slave_ELEC1601_Student_2019_v3                                                                           //
//  The Instructor version of this code is identical to this version EXCEPT it also sets PIN codes           //
//  20191008 Peter Jones                                                                                     //
//                                                                                                           //
//  Bi-directional passing of serial inputs via Bluetooth                                                    //
//  Note: the void loop() contents differ from "capitalise and return" code                                  //
//                                                                                                           //
//  This version was initially based on the 2011 Steve Chang code but has been substantially revised         //
//  and heavily documented throughout.                                                                       //
//                                                                                                           //
//  20190927 Ross Hutton                                                                                     //
//  Identified that opening the Arduino IDE Serial Monitor asserts a DTR signal which resets the Arduino,    //
//  causing it to re-execute the full connection setup routine. If this reset happens on the Slave system,   //
//  re-running the setup routine appears to drop the connection. The Master is unaware of this loss and      //
//  makes no attempt to re-connect. Code has been added to check if the Bluetooth connection remains         //
//  established and, if so, the setup process is bypassed.                                                   //
//                                                                                                           //
//-----------------------------------------------------------------------------------------------------------//

#include <SoftwareSerial.h>   //Software Serial Port

#include <Servo.h>                           // Include servo library
 
Servo servoLeft;                             // Declare left and right servos
Servo servoRight;

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################

int shieldPairNumber = 5;

// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// #######################################################

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);


int irLeft = 2; //Pin 2
int irFront = 3; //Pin 3
int irRight = 4; //Pin 4

int leftVal = HIGH; //HIGH = no obstacle detected 
int frontVal = HIGH;
int rightVal = HIGH;


// Need to store the moves we make so that we can complete the required automated replication 

char moves[100];
int moveCounter = 0;  

void setup()
{
  
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
     
      tone(4, 3000, 1000);                       // Play tone for 1 second
      delay(1000);                               // Delay to finish tone
    
      servoLeft.attach(13);                      // Attach left signal to pin 13
      servoRight.attach(12); 

      pinMode(irLeft, INPUT);
      pinMode(irFront, INPUT);
      pinMode(irRight, INPUT);
}

//300 is 45 degrees
//600should be 90
void loop()
{
    char recvChar;

    while(1){
        if(blueToothSerial.available()){   // Check if there's any data sent from the remote Bluetooth shield
            recvChar = blueToothSerial.read();
            Serial.print(recvChar);
            if(recvChar == 'w'){
              moves[moveCounter] = 'w';
              moveCounter++;
              forward(50);
            }
            else if(recvChar == 'a'){
              moves[moveCounter] = 'a';
              moveCounter++;
              turnLeft(600);  
              forward(50);
            }
            else if(recvChar == 'd'){
              moves[moveCounter] = 'd';
              moveCounter++;
              turnRight(500); 
              forward(50);
            }
            else if(recvChar == 's'){
              moves[moveCounter] = 's';
              moveCounter++;
              backward(50);   
            }

            // Automated Replication
            else if(recvChar == 'm'){
              for(int i = 0; i <= moveCounter; i++){
                if(moves[i] == 'w'){
                forward(1000);
                }
                if(moves[i] == 'a'){
                  turnLeft(600);
                  forward(1000);
    
                }
                if(moves[i] == 'd'){
                  turnRight(600); 
                  forward(1000);
                }
                if(moves[i] == 's'){
                  backward(1000);   
                } 
            }
          }

            // Begin automatic maze navigation
            
            else if (recvChar == 'z') {
                while (true) {
                  leftVal = digitalRead(irLeft);
                  frontVal = digitalRead(irFront);
                  rightVal = digitalRead(irRight);
                  
                  // Only Right free
                  if(leftVal == LOW && frontVal == LOW && rightVal == HIGH){
                    backward(1000); // Not sure how long to reverse or if we even need to go back if we adjust IR sensor so that it detects the wall 
                    turnRight(400);
                  }
                  
                  // Only Left free
                  else if(leftVal == HIGH && frontVal == LOW && rightVal == LOW){
                    backward(1000);
                    turnLeft(300); 
                  }
                  
                  // Both Left and Right free --> choose left turn
                  else if(leftVal == HIGH && frontVal == LOW && rightVal == HIGH){
                    backward(1000);
                    turnLeft(300); 
                  }
                  
                  //180 turn at deadend where the ball is 
                  else if(leftVal == LOW && frontVal == LOW && rightVal == LOW){

                    delay(5000); // 5 seconds to put ball on robot
                    Serial.println("found ball");
                    
                   if(Serial.available()){
                    recvChar  = "FOUND BALL!";
                    Serial.print(recvChar);
                    blueToothSerial.print(recvChar);
                   }
                    backward(1000);
//                    turnLeft(1200);
                    //break;     // break out of the automatic navigation loop so that we can send a found message to master -> input 'z' again to begin robots return 
                     
                  }                   
                  else{
                    forward(50);
                  }   
                }
            }
        }

        if(Serial.available()){
          recvChar  = Serial.read();
          Serial.print(recvChar);
          blueToothSerial.print(recvChar);
         }
    } 
}

void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}


void forward(int time)                       // Forward function
{
  servoLeft.writeMicroseconds(1600);         // Left wheel counterclockwise
  servoRight.writeMicroseconds(1400);        // Right wheel clockwise
  delay(time);                               // Maneuver for time ms
}

void turnLeft(int time)                      // Left turn function
{
  servoLeft.writeMicroseconds(1300);         // Left wheel clockwise
  servoRight.writeMicroseconds(1300);        // Right wheel clockwise
  delay(time);                               // Maneuver for time ms
}

void turnRight(int time)                     // Right turn function
{
  servoLeft.writeMicroseconds(1700);         // Left wheel counterclockwise
  servoRight.writeMicroseconds(1700);        // Right wheel counterclockwise
  delay(time);                               // Maneuver for time ms
}

void backward(int time)                      // Backward function
{
  servoLeft.writeMicroseconds(1400);         // Left wheel clockwise
  servoRight.writeMicroseconds(1600);        // Right wheel counterclockwise
  delay(time);                               // Maneuver for time ms
}

void pivotForwardLeft(int time){
  servoLeft.writeMicroseconds(1500);     
  servoRight.writeMicroseconds(1300);
  delay(time);     
}

void pivotForwardRight(int time){
  servoLeft.writeMicroseconds(1700);        
  servoRight.writeMicroseconds(1500);  
  delay(time);    
}

void pivotBackwardLeft(int time){
  servoLeft.writeMicroseconds(1500);         
  servoRight.writeMicroseconds(1700);    
  delay(time);    
}

void pivotBackwardRight(int time){
  servoLeft.writeMicroseconds(1300);        
  servoRight.writeMicroseconds(1500); 
  delay(time);       
}

// Gradual start to full speed
void gradualForward(int time){
  for(int speed = 0; speed <=200; speed += 2){
    servoLeft.writeMicroseconds(1500 + speed);
    servoRight.writeMicroseconds(1700 - speed);
    delay(time); 
  }
}

void gradualBackward(int time){
  for(int speed = 0; speed >= -200; speed -= 2){
    servoLeft.writeMicroseconds(1500 + speed);
    servoRight.writeMicroseconds(1700 - speed);
    delay(time); 
  }
}
