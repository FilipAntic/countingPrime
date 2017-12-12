#include <Wire.h>
#define ID 8
#define EMPTY 0
#define WORKING 1
#define FINISHED 2
#define FINISHED_STATE_SENDING 0
#define FINISHED_PRIME_NUM_SENDING 1
#define FINISHED_PRIME_SENDING 2
//------- JA SAM SLAVE 1--------

/**
   Stanja koje ima slave :
   -EMPTY za stanje u kojem kaze da ceka brojeve koje treba da racuna
   -WORKING za stanje u kojem kaze da jos izvrsava proste brojeve
   -FINISHED za stanje u kojem kaze da je zavrsio sa racunanjem brojeva
*/
int state = EMPTY;
/**
   Stanja koje ima FINISHED stanje :
   -FINISHED_STATE_SENDING stanje u kojem se salje stanje da je 2
   -FINISHED_PRIME_NUM_SENDING stanje u kojem saljem broj prostih brojeva u tom opsegu
   -FINISHED_PRIME_SENDING stanje u kojem saljem proste brojeve jedan po jedan
*/
int finishedState = FINISHED_STATE_SENDING;
/*
   Broj prostih brojeva u opsegu od x-y
*/
int numOfPrimes = 0;
/*
   Levi opseg
*/
long x;
/*
   Desni opseg
*/
long y;
/*
   Trenutni prost broj koji se salje
*/
int currentPrime = 0;
/*
   Niz prostih brojeva
*/
long primes[128];
/*
   Razlika y-x da bi znali koliko brojeva racunamo
*/
long razlika;
/*
   Da li da startujem racunanje prostih brojeva
*/
boolean startWorking = false;
/**
   Da li je dosao broj x
*/
boolean firstCame = false;
/**
   Trenutna iteracija koja je potrebna zbog racunanja procenta
*/
long currentIteration = 0;
/**
   Koliko iznosi jedan procenat da bi mogli da racunamo preostale procente do zavrsetka posla
*/
float onePercent = 0.0;
/**
   Trenutni broj milisekundi od pocetka rada slave-a
*/
unsigned long currentMilliseconds = 0;
/**
   Prethodni broj milisekundi
*/
unsigned long previousMilliseconds = 0;
/**
    Koliko je proslo milisekundi do sada ukupno
*/
unsigned long untilNowMilliseconds = 1;
/**
   Predvidjanje koliko je milisekundi ostalo do kraja
*/
long res = 0;
/**
  Ovaj flag koristim da bih video da li treba da posaljem predicted time na master
*/
boolean sendPredictedTimeValue = false;

/**
  Ovo opisuje wokring da moze da salje ili predicted time ili da moze da salje u kom je stateu
*/
boolean workingState = 0;

void setup()
{
  Wire.begin(ID);
  Serial.begin(9600);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  memset(primes, 0, 128);
}
void loop()
{
  if (startWorking) {
    Serial.println("Usao ovde");
    changeStateToWorking();
    countPrimeNumbers(x, y);
    startWorking = false;
  }
}

void receiveEvent(int howMany)
{
  if (howMany == 8) {
    long tmp;
    if (Wire.available()) {
      //      Serial.println("OVDE PRIMAM BROJEVE");
      x = Wire.read();
      //      Serial.println(x);
      tmp = Wire.read();
      x += (tmp) << 8;
      //      Serial.println(tmp);
      tmp = Wire.read();
      x += (tmp) << 16;
      //      Serial.println(tmp);
      tmp = Wire.read();
      x += (tmp) << 24;
      //      Serial.println(tmp);
      //-----
      //      Serial.println("--------------");
      y = Wire.read();
      //      Serial.println(y);
      tmp = Wire.read();
      y += (tmp) << 8;
      //      Serial.println(tmp);
      tmp = Wire.read();
      y += (tmp) << 16;
      //      Serial.println(tmp);
      tmp = Wire.read();
      y += (tmp) << 24;
      //      Serial.println(tmp);
      Serial.println("----");
      Serial.println(x);
      Serial.println(y);
      startWorking = true;
      razlika = y - x;
      tmp = 0;
      onePercent = razlika / 100.00;
    }
  }
  else if (howMany == 1) {
    //    sendPredictedTimeValue = true;
    workingState = 1;
    int number = Wire.read();
  }

}

void requestEvent()
{
  switch (state) {
    case EMPTY:
      Wire.write(state);
      break;
    case WORKING:
      switch (workingState) {
        case 0:
          Wire.write(state);
          if (sendPredictedTimeValue) {
            workingState = 1;
          }
          break;
        case 1:
          sendPredictedResult();
          sendPredictedTimeValue = false;
          workingState = 0;
          break;
      }
      break;
    case FINISHED:
      switch (finishedState) {
        case FINISHED_STATE_SENDING:
          Wire.write(state);
          finishedState = FINISHED_PRIME_NUM_SENDING;
          break;
        case FINISHED_PRIME_NUM_SENDING:
          Serial.print("Broj prostih brojeva: ");
          Serial.println(numOfPrimes);
          Wire.write(numOfPrimes);
          finishedState = FINISHED_PRIME_SENDING;
          break;
        case FINISHED_PRIME_SENDING:
          sendPrimeNumber();
          //          sendPredictedResult();
          break;
          break;

      }
  }
}


void countPrimeNumbers(long x, long y) {
  numOfPrimes = 0;
  long i;
  for ( i = x; i < y; i++) {
    currentIteration = i - x;
    int isPrimeNumber = 1;
    unsigned long currentHere = millis();
    long j;
    for (j = 2; j < i; j++) {
      printFinishedWork();
      if (i % j == 0) {
        isPrimeNumber = 0;
        break;
      }
    }
    untilNowMilliseconds += millis() - currentHere;
    long moreIters = y - i - 1;
    predictFinishTime(untilNowMilliseconds, currentIteration, moreIters);
    if (isPrimeNumber) {
      primes[i - x] = i;
      numOfPrimes++;
      Serial.print("Do sada prosti brojevi ");
      Serial.println(numOfPrimes);
    } else {
      primes[i - x] = 0;
    }

  }
  int brojac = 0;
  for (int p = 0; p < 128; p++) {
    if (primes[p] != 0) {
      primes[brojac] = primes[p];
      brojac++;
    }
  }
  Serial.print("100");
  Serial.println("%");
  changeStateToFinished();
}

void sendPrimeNumber()
{
  byte buf[4];
  buf[0] = (primes[currentPrime] & 0xFF);
  buf[1] = ((primes[currentPrime] >> 8) & 0xFF);
  buf[2] = ((primes[currentPrime] >> 16) & 0xFF);
  buf[3] = ((primes[currentPrime] >> 24) & 0xFF);
  Wire.write(buf, 4);
  delay(10);
  currentPrime++;
  if (currentPrime == numOfPrimes)
  {
    changeStateToEmpty();
    finishedState = FINISHED_STATE_SENDING;
    currentPrime = 0;
    memset(primes, 0, 128);
    x = 0;
    y = 0;
    currentMilliseconds = 0;
    previousMilliseconds = 0;
  }
}

void sendPredictedResult()
{
  byte buf [4];
  Serial.print("Ovo saljem iz send predicted time-a");
  Serial.println(res);
  buf[0] = (res & 0xFF);
  Serial.println(buf[0]);
  buf[1] = ((res >> 8) & 0xFF);
  Serial.println(buf[1]);
  buf[2] = ((res >> 16) & 0xFF);
  Serial.println(buf[2]);
  buf[3] = ((res >> 24) & 0xFF);
  Serial.println(buf[3]);
  Wire.write(buf, 4);
}
void changeStateToFinished()
{
  state = FINISHED;
}

void changeStateToEmpty()
{
  state = EMPTY;
}

void changeStateToWorking()
{
  state = WORKING;
}

void countPercentage()
{
  Serial.print(currentIteration / onePercent);
  Serial.println("%");
}

void printFinishedWork()
{
  currentMilliseconds = millis();
  if (currentIteration / onePercent < 25 && currentMilliseconds - previousMilliseconds >= 10000) {
    countPercentage();
    previousMilliseconds = currentMilliseconds;
  } else if (currentIteration / onePercent > 25 && currentIteration / onePercent < 50 && currentMilliseconds - previousMilliseconds >= 7000) {
    countPercentage();
    previousMilliseconds = currentMilliseconds;
  } else if (currentIteration / onePercent > 50 && currentIteration / onePercent < 75 && currentMilliseconds - previousMilliseconds >= 5000) {
    countPercentage();
    previousMilliseconds = currentMilliseconds;
  } else if (currentIteration / onePercent > 75 && currentMilliseconds - previousMilliseconds >= 3000) {
    countPercentage();
    previousMilliseconds = currentMilliseconds;
  }
}

void predictFinishTime(long currentMillisForIters, long numberOfIters, long moreIters)
{
  res = (long)(currentMillisForIters / numberOfIters) * (moreIters + 1);
  Serial.print(res);
  Serial.println("ms");
}
