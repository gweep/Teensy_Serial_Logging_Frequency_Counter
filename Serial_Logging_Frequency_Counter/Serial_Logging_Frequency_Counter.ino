// Two channel data loging 30 MHz frequency counter.  Data logged via serial port.
// Michael Sciascia - AB2TS  20260206
//
// Based on the FreqCountCode.ino for the Teensy 4.0 by Paul Stoffregen at PJRC
//
// Designed to read and log a VFO signal for the purposes of determining stability and drift characteristics.
// The first frequency counter input is for the signal to be measured.
// The counter expects a stable and calibrated 10 MHz signal delivered to the second counter input channel (ch1).
// The 10 MHz signal is used to determine the Teensy CPU clock frequency and drift so that the VFO measurment can be error corrected.
// All data is time stamped and transmitted via the serial port.  There is no frequency display.
//
//
//
// The orginal code by Paul Stoffregen can measure 10 channels at once.  Pretty cool.
//
// Measures 10 frequencies by counting rising edges.  Best for 10kHz to 30 MHz
// Connect frequencies to pins 6, 9, 10, 11, 12, 13, 14, 15, 18, 19

// https://forum.pjrc.com/threads/71193-Teensy-4-Measuring-multiple-frequencies

// Timer	Pin   Pad	ALT	input mux
// QuadTimer4_1   6   B0_10	1
// QuadTimer4_2   9   B0_11	1
// QuadTimer1_0  10   B0_00	1
// QuadTimer1_2  11   B0_02	1
// QuadTimer1_1  12   B0_01	1
// QuadTimer2_0  13   B0_03	1	IOMUXC_QTIMER2_TIMER0_SELECT_INPUT=1
// QuadTimer3_2  14   AD_B1_02	1	IOMUXC_QTIMER3_TIMER2_SELECT_INPUT=1
// QuadTimer3_3  15   AD_B1_03	1	IOMUXC_QTIMER3_TIMER3_SELECT_INPUT=1
// QuadTimer3_1  18   AD_B1_01	1	IOMUXC_QTIMER3_TIMER1_SELECT_INPUT=1
// QuadTimer3_0  19   AD_B1_00	1	IOMUXC_QTIMER3_TIMER0_SELECT_INPUT=1

// #define GATE_INTERVAL 2000  // microseconds for each gate interval
// #define GATE_ACCUM    100   // number of intervals to accumulate
// #define MULT_FACTOR   5     // multiply to get Hz output (5 Hz resolution)


// Adapted and expanded by Michael Sciascia - Please credit PJRC for their hard work -
// 20260201 mcs
#define VERSION "20260207:1415"

#define GATE_INTERVAL 2000  // microseconds for each gate interval
#define GATE_ACCUM 500      // number of intervals to accumulate
#define MULT_FACTOR 1       // multiply to get Hz output (1 Hz resolution)

#define REF_CLOCK_FREQ 10000000.0  // 10 MHz typically - use a decimal point for floating point calculation
enum printMode_ENUM { BASIC_DATA = 0,
                      REPORT };

// printMode_ENUM printMode = BASIC_DATA;
printMode_ENUM printMode = REPORT;
bool printHeader = true;

// Always and a trailing decimal zero (.0) as is used in a double floating point calculation
// 20260201 end - mcs


// 20260201 mcs
unsigned long reportInterval = 3600 * 1000 / 10;  // every 6 minutes = 1/10th of an hour
#define INITAL_REPORT 1000                        // will be subtracting 1 second to accomodate the gate interval so this restores nice time intervals
unsigned long lastReport = INITAL_REPORT;
// 20260201 end - mcs


typedef struct {
  IMXRT_TMR_t *timer;
  int timerchannel;
  int pin;
  int pinconfig;
  volatile uint32_t *inputselectreg;
  int inputselectval;
} timerinfo_t;


// const timerinfo_t timerlist[] = {
// 	// Timer     Ch  Pin  Alt Input Select
// 	{&IMXRT_TMR4, 1,   6,  1, NULL, 0},
// 	{&IMXRT_TMR4, 2,   9,  1, NULL, 0},
// 	{&IMXRT_TMR1, 0,  10,  1, NULL, 0},
// 	{&IMXRT_TMR1, 2,  11,  1, NULL, 0},
// 	{&IMXRT_TMR1, 1,  12,  1, NULL, 0},
// 	{&IMXRT_TMR2, 0,  13,  1, &IOMUXC_QTIMER2_TIMER0_SELECT_INPUT, 1},
// 	{&IMXRT_TMR3, 2,  14,  1, &IOMUXC_QTIMER3_TIMER2_SELECT_INPUT, 1},
// 	{&IMXRT_TMR3, 3,  15,  1, &IOMUXC_QTIMER3_TIMER3_SELECT_INPUT, 1},
// 	{&IMXRT_TMR3, 1,  18,  1, &IOMUXC_QTIMER3_TIMER1_SELECT_INPUT, 0},
// 	{&IMXRT_TMR3, 0,  19,  1, &IOMUXC_QTIMER3_TIMER0_SELECT_INPUT, 1},
// 	// TODO: can 6 more be used with XBAR1 and GPR6 ?
// };


const timerinfo_t timerlist[] = {
  // Timer     Ch  Pin  Alt Input Select
  { &IMXRT_TMR4, 1, 6, 1, NULL, 0 },
  { &IMXRT_TMR4, 2, 9, 1, NULL, 0 }
};


#define NUM_TIMERS (sizeof(timerlist) / sizeof(timerinfo_t))

// gate interval interrupt deposits data here
volatile bool count_update = false;
volatile uint32_t count_output[NUM_TIMERS];


// 20260201 mcs
uint32_t lastCount[NUM_TIMERS];
// 20260201 end - mcs


uint16_t read_count(unsigned int n) {
  static uint16_t prior[NUM_TIMERS];
  if (n >= NUM_TIMERS) return 0;
  uint16_t count = (timerlist[n].timer)->CH[timerlist[n].timerchannel].CNTR;
  uint16_t inc = count - prior[n];
  prior[n] = count;
  return inc;
}


void gate_timer() {
  static unsigned int count = 0;
  static uint32_t accum[NUM_TIMERS];

  for (unsigned int i = 0; i < NUM_TIMERS; i++) {
    accum[i] += read_count(i);
  }
  if (++count >= GATE_ACCUM) {
    for (unsigned int i = 0; i < NUM_TIMERS; i++) {
      count_output[i] = accum[i];
      accum[i] = 0;
    }
    count_update = true;
    count = 0;
  }
}


// this function set the reporting interval.  Data is recorded frequently at startup
// (initially every 15 seconds) and less frequently as time passes (every 15 minutes).
void calc_reportInterval(unsigned long timeNow_mS) {
  if (timeNow_mS > 1000 * 3600) {      // if elasped time greater than an hour
    reportInterval = 1000 * 3600 / 4;  // report every 15 minutes
  }

  else if (timeNow_mS >= 1000 * 3600 / 2) {  // if elasped time greater than an 30 min
    reportInterval = 1000 * 3600 / 10;       // report every 6 minutes
  }

  else if (timeNow_mS >= 1000 * 3600 / 6) {  // if elasped time greater than 10 min
    reportInterval = 3600 * 1000 / 30;       // every 2 minutes
  }

  else if (timeNow_mS >= 1000 * 3600 / 30) {  // if elasped time greater than 2 min
    reportInterval = 3600 * 1000 / 60;        // every 1 minutes
  }

  else {                                      // all other:: elapsed ctime less than 2 min
    reportInterval = 3600 * 1000 / (60 * 4);  // every 15 seconds
    //  reportInterval = 3600 * 1000 / (60 * 12);  // every 5 seconds
    // reportInterval = 3600 * 1000 / (60 * 30);  // every 2 seconds
  }

  // for some reason the gate counting adds to the interval, so subtract it from the reportInterval
  reportInterval -= ((GATE_INTERVAL * GATE_ACCUM) / 1000);  // milli-Seconds
}


void setup() {
  // // create some test frequencies
  // analogWriteFrequency(0, 3000000);
  // analogWriteFrequency(1, 15000000);
  // analogWriteFrequency(2, 220000);
  // analogWriteFrequency(4, 10000000);
  // analogWriteFrequency(5, 455000);
  // analogWrite(0, 128);
  // analogWrite(1, 128);
  // analogWrite(2, 128);
  // analogWrite(4, 128);
  // analogWrite(5, 128);

  // Welcome message
  Serial.begin(9600);

  delay(5000);  // pause so that any previous data can be cleared from the serial terminal after a reset/restart of the Teensy 4.0.

  Serial.print("\nAB2TS - Version: ");
  Serial.println(VERSION);

  Serial.print("FreqCountMany, maximum frequency = ");
  Serial.print(65535.0 / ((double)GATE_INTERVAL), 3);
  Serial.println(" MHz");

  Serial.println("\nInput channel ch0_raw is for the primary signal to be counted and logged.");
  Serial.println("Input channel ch1_ref is for a calibrated 10 MHz reference signal which is used to correct any Teensy cpu frequency error and drift.");
  Serial.println("Output channel f(ch0_cal) == ch0_raw*10^6/ch1_ref");

  Serial.println("\nThe elapsed time count is based on the Teensy internal clock and is not corrected for any error.");
  Serial.println("This error should remain relatively small in comparison to the corrected frequency measurement.");




  // turn on clock to all quad timers
  CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON) | CCM_CCGR6_QTIMER2(CCM_CCGR_ON)
               | CCM_CCGR6_QTIMER3(CCM_CCGR_ON) | CCM_CCGR6_QTIMER4(CCM_CCGR_ON);

  // configure all counting timers
  for (unsigned int i = 0; i < NUM_TIMERS; i++) {
    IMXRT_TMR_t *timer = timerlist[i].timer;
    int ch = timerlist[i].timerchannel;
    timer->CH[ch].CTRL = 0;
    timer->CH[ch].CNTR = 0;
    timer->CH[ch].LOAD = 0;
    timer->CH[ch].COMP1 = 65535;
    timer->CH[ch].CMPLD1 = 65535;
    timer->CH[ch].SCTRL = 0;
    timer->CH[ch].CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(ch) | TMR_CTRL_LENGTH;
    int pin = timerlist[i].pin;
    *portConfigRegister(pin) = timerlist[i].pinconfig;
    if (timerlist[i].inputselectreg) {
      *timerlist[i].inputselectreg = timerlist[i].inputselectval;
    }
  }

  // start gate interval timer
  static IntervalTimer t;
  t.begin(gate_timer, GATE_INTERVAL);
}

void loop() {
  static uint32_t ch0_raw = 0;
  static uint32_t ch1_ref = 0;
  static uint32_t ch0_calibrated = 0;

  static uint32_t last_ch0_raw = 0;
  static uint32_t last_ch1_ref = REF_CLOCK_FREQ;
  static uint32_t last_ch0_calibrated = 0;

  static unsigned long now_mS = 0;

  static long ch0_raw_drift = 0;
  static long ch1_ref_drift = 0;
  static long ch0_cal_drift = 0;

  static double tempFactor = 0.0;



  calc_reportInterval(now_mS);
  // reportInterval = 3600 * 1000 / (60 * 30);                 // every 2 seconds
  // reportInterval -= ((GATE_INTERVAL * GATE_ACCUM) / 1000);  // milli-Seconds


  if (count_update) {

    now_mS = millis();
    ch0_raw = count_output[0] * MULT_FACTOR;
    ch1_ref = count_output[1] * MULT_FACTOR;


    if (now_mS > lastReport + reportInterval) {  // report on the interval

      tempFactor = (1000 * 3600) / (double)(now_mS - lastReport);
      ch0_raw_drift = tempFactor * ((double)ch0_raw - (double)last_ch0_raw);


      if (ch1_ref > 0) {
        ch0_calibrated = (double)ch0_raw * REF_CLOCK_FREQ / (double)ch1_ref;
        ch1_ref_drift = tempFactor * ((double)ch1_ref - (double)last_ch1_ref);
        ch0_cal_drift = tempFactor * ((double)ch0_calibrated - (double)last_ch0_calibrated);

        last_ch1_ref = ch1_ref;
        last_ch0_calibrated = ch0_calibrated;
      }


      if (lastReport > INITAL_REPORT) {  // don't report the first data.  It appears to be short counting.

        if (printMode == REPORT) {

          if (printHeader == true) {
            // Serial.println("\n Time(mS)     f(ch0_raw)Hz  f(ch1_ref)Hz  f(ch0_cal)Hz");
            Serial.println("\n  Elapsed            ch0_raw                ch1_ref                ch0_cal");
            Serial.println("   Time            f        drift         f        drift         f        drift");
            Serial.println(" hh:mm:ss          Hz       Hz/Hr         Hz       Hz/Hr         Hz       Hz/Hr");
            Serial.println(" -------------------------------------------------------------------------------");

            printHeader = false;
          }

          unsigned long allSeconds = now_mS / 1000;
          int runHours = allSeconds / 3600;
          int secsRemaining = allSeconds % 3600;
          int runMinutes = secsRemaining / 60;
          int runSeconds = secsRemaining % 60;

          // char buf[20];  // Buffer to hold the formatted string
          // // Format string: %02d prints an integer with at least 2 digits, padded with zeros
          // sprintf(buf, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);

          Serial.printf(" %02d:%02d:%02d      ", runHours, runMinutes, runSeconds);
          Serial.printf("%8u    %5d      ", ch0_raw, ch0_raw_drift);

          if (ch1_ref > 0) {
            Serial.printf("%8u    %5d      ", ch1_ref, ch1_ref_drift);
            Serial.printf("%8u    %5d      ", ch0_calibrated, ch0_cal_drift);
          }

          Serial.println();


        } else if (printMode == BASIC_DATA) {

          if (printHeader == true) {
            Serial.println("\n Time(mS)         f(ch0_raw)Hz   f(ch1_ref)Hz   f(ch0_cal)Hz");
            Serial.println(" -------------------------------------------------------------------------------");

            printHeader = false;
          }

          Serial.printf("%10u          %8u       ", now_mS, ch0_raw);

          if (ch1_ref > 0) {
            Serial.printf("%8u       %8u", ch1_ref, ch0_calibrated);
          }

          Serial.println();
        }
      }

      last_ch0_raw = ch0_raw;
      lastReport = now_mS;
    }

    count_update = false;
  }
}
