#include <Wire.h>
#define EMPTY 0
#define WORKING 1
#define FINISHED 2

//--------JA SAM MASTER-------

/**
   Prvi broj je broj slavova
   poslednjeg(ne i poslednje) su brojevi ID-eva slavova
   poslednji broj je broj sekundi na koliko treba da se ispisuje preostalo vreme
   poslednji broj se dobija -> params[params[0]+1]
*/
int params[] = {1, 8, 7};
/**
   Promenljiva sluzi za citanje karaktera koji su prosledjeni kroz Serial.
*/
char incomingByte;
/**
   Levi opseg
*/
long x;
/**
   Desni opseg
*/
long y;
/**
   State u kom se nalazi slave
*/
int slaveState;
/**
   State u kom se nalazi slave
*/
int numOfPrimes;
/**
   Da li uzimam prvi ili drugi broj sa Seriala
*/
boolean isFirstNumber = true;
/**
   Da li sam dobio brojeve sa seriala
*/
boolean done = false;
/**
   Racuna se range za x i y -> y-x
*/
long range;
/**
   Koliko iznosi jedan paket
*/
int packSize;
/**
   Koliko iznosi jedan paket
*/
int lastPackSize = 100;
/**
   Koliko je ostalo do kraja da se izvrsi
*/
long predictedTime = 0;
/**
   Na koliko sekundi treba da trazim od slave-a predicted time
*/
int xSeconds = params[params[0] + 1];
/**
   Trenutni broj milisekundi od pocetka rada slave-a
*/
unsigned long currentMilliseconds = 0;
/**
   Prethodni broj milisekundi
*/
unsigned long previousMilliseconds = 0;
/**
   Niz predvidjenog vremena da bi kasnije mogao da ga sortiram
*/
long arrayofpredictedVal[1];
/**
   Da bih znao kada da ispisujem i sortiram niz predvidjenih zavrsetaka vremena
*/
boolean flag = false;
/**
   Prethodni broj prostih brojeva
*/
int lastNumOfPrimes = 2;
/**
   Da li je zavrsio uzimanje brojeva od prvog slave-a
*/
boolean finishedGettingNumbers = false;

boolean finishedGettingPredictNumber = false;
void setup()
{
  Wire.begin();
  Serial.begin(9600);
}
void loop()
{

  if (Serial.available())
  {
    getRangeFromSerial();
    done = true;
    lastPackSize = 100;
  }
  if (flag) {
    sortPredicted();
    flag = false;
  }
  for (int i = 0; i < params[0]; i++) {
    //    Serial.println("------------------");
    //    Serial.println(params[i + 1]);
    //    Serial.println(i);
    checkSlavesState(params[i + 1]);
    printState();
    switch (slaveState)
    {
      case EMPTY:
        if (x < y)
        {
          countPackSize();
          Serial.println("Usao u done");
          long newRange = x + packSize;
          Serial.println(newRange);
          sendRangeToSlaves(x, newRange, params[i + 1]);
          x += packSize;
          delay(100);
        }
        break;
      case WORKING:
        currentMilliseconds = millis();
        if (currentMilliseconds - previousMilliseconds >= xSeconds * 1000) {
          Wire.beginTransmission(params[i + 1]);
          Wire.write(1);
          Wire.endTransmission();
          delay(100);
          getPredictedTime(params[i + 1]);
          while (!finishedGettingPredictNumber) {

          }
          finishedGettingPredictNumber;
          arrayofpredictedVal[i] = predictedTime;
          flag  = true;
          previousMilliseconds = currentMilliseconds;
        }
        break;
      case FINISHED:
        requestNumberOfPrimeNumbersFromSlave(params[i + 1]);
        getPrimeNumbers(params[i + 1]);
        //        while (!finishedGettingNumbers) {
        //
        //        }
        finishedGettingNumbers = false;
        break;
    }
    delay(500);
  }
}

void getRangeFromSerial()
{
  x = 0;
  y = 0;
  while (Serial.available()) {
    incomingByte = Serial.read();
    delay(10);
    if (isDigit(incomingByte)) {
      if (isFirstNumber) {
        x = (incomingByte - '0') + x * 10;
      } else {
        y = (incomingByte - '0') + y * 10;
      }
    } else {
      isFirstNumber = false;
    }
  }
  range = y - x;
  isFirstNumber = true;
}

void checkSlavesState(int slaveAddress)
{
  Wire.requestFrom(slaveAddress, 1);
  //  while (Wire.available() == 0);
  if (Wire.available()) {
    slaveState = Wire.read();
  }
}

void printState()
{
  Serial.println(slaveState);
}

void requestNumberOfPrimeNumbersFromSlave(int slaveAddress)
{
  Wire.requestFrom(slaveAddress, 1);
  //  while (Wire.available() == 0);
  if (Wire.available()) {
    numOfPrimes = Wire.read();
    lastNumOfPrimes = numOfPrimes;
    Serial.print("Broj prostih brojeva: ");
    Serial.println(numOfPrimes);
    delay(500);
  }
}

void getPrimeNumbers(int slaveID)
{
  long primeNumber;
  for (int i = 0; i < numOfPrimes; i++)
  {
    Wire.requestFrom(slaveID, 4);
    //    while (Wire.available() == 0);
    long tmp;
    primeNumber = Wire.read();
    delay(10);
    tmp = Wire.read();
    delay(10);
    primeNumber += (tmp) << 8;
    tmp = Wire.read();
    delay(10);
    primeNumber += (tmp) << 16;
    tmp = Wire.read();
    delay(10);
    primeNumber += (tmp) << 24;
    Serial.print("Prost broj: ");
    Serial.println(primeNumber);
  }

  finishedGettingNumbers = true;
}

void getPredictedTime(int slaveID)
{
  Wire.requestFrom(slaveID, 4);
  //  while (Wire.available() == 0);
  if (Wire.available()) {
    long tmp;
    predictedTime = Wire.read();
    Serial.println(predictedTime);
    delay(10);
    tmp = Wire.read();
    Serial.println(tmp);
    delay(10);
    predictedTime += (tmp) << 8;
    tmp = Wire.read();
    Serial.println(tmp);
    delay(10);
    predictedTime += (tmp) << 16;
    tmp = Wire.read();
    Serial.println(tmp);
    delay(10);
    predictedTime += (tmp) << 24;
    Serial.print("Predicted broj: ");
    Serial.print(predictedTime);
    Serial.println("ms");
    finishedGettingPredictNumber = true;
  }
}


void sendRangeToSlaves(long x, long y, int slaveAddress)
{

  Serial.print("X je: ");
  Serial.println(x);
  Serial.print("Y je ");
  Serial.println(y);
  Wire.beginTransmission(slaveAddress);
  byte buf [8];
  //  Serial.println(x);
  buf [0] = (x & 0xFF);
  //  Serial.println(buf[0]);
  buf [1] = ((x >> 8) & 0xFF);
  //  Serial.println(buf[1]);
  buf [2] = ((x >> 16) & 0xFF);
  //  Serial.println(buf[2]);
  buf [3] = ((x >> 24) & 0xFF);
  //  Serial.println(buf[3]);
  //  Serial.println(y);
  buf [4] = (y & 0xFF);
  //  Serial.println(buf[0]);
  buf [5] = ((y >> 8) & 0xFF);
  //  Serial.println(buf[1]);
  buf [6] = ((y >> 16) & 0xFF);
  //  Serial.println(buf[2]);
  buf [7] = ((y >> 24) & 0xFF);
  //  Serial.println(buf[3]);

  Wire.write(buf, 8);
  delay(10);
  Wire.endTransmission();
}

void countPackSize() {
  int counterForPackSize;
  if (lastNumOfPrimes == 0 || lastNumOfPrimes == 1) {
    lastPackSize *= 2;
  }
  if (params[0] * lastPackSize < (y - x)) {
    packSize = lastPackSize;
  } else if (params[0] > y - x) {
    packSize = 1;
  } else {
    packSize = (int) (y - x) / params[0];
  }
  lastPackSize = packSize;
  Serial.print("Pack size: ");
  Serial.println(packSize);
}

void sortPredicted() {

  for (int c = 0 ; c < params[0]; c++)
  {
    for (int d = 0 ; d < params[0] - c - 1; d++)
    {
      if (arrayofpredictedVal[d] > arrayofpredictedVal[d + 1]) /* For decreasing order use < */
      {
        long swap       = arrayofpredictedVal[d];
        arrayofpredictedVal[d]   = arrayofpredictedVal[d + 1];
        arrayofpredictedVal[d + 1] = swap;
      }
    }
  }

  for (int i = 0; i < params[0]; i++) {
    Serial.print("Predicted time: ");
    Serial.print(arrayofpredictedVal[i]);
    Serial.println("ms");
  }
}

