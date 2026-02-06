# A Two Channel 30 MHz Serial Logging Frequency Counter For The Teensy 4.0
This is a two channel 30 MHz logging frequency counter based on the Teensy 4.0 Development Board.  Data is transmitted directly via serial port so that it can be recorded and later analyzed via spreadsheet.  There is no external display to show the measured frequency directly.  You must use a serial terminal program to view the data.   I use CoolTerm which offers a means to save the received data to a file.  Alternatively the data text can be copied directly from the screen of the serial terminal.  CoolTerm is available for MacOS, Windows, Linux and RaspberryPi. 

## Background
When building a variable frequency oscillator (VFO) for amateur radio use it is good to know the frequency stability and drift characteristics of the oscillator.  I have had many sessions manually logging time and frequency data points of a newly constructed VFO with a frequency counter and a clock so that I could calculate and plot the oscillator's drift rate.  This is a tedious and time consuming effort that is often repeated several times during a VFO's development.   I needed a better way to gather data.

## Details
Paul Stoffregen at PJRC provides the pieces for a nice solution.  By employing a Teensy 4.0 and a modification of his  FreqCountMany.ino code for the Teensy, I was able to automate the task of recording my VFO's frequency drift.  Interestingly; while PJRC has several frequency counter libraries for their Teensy product line, their FreqCountMany.ino code directly accesses the inner workings of the Teensy without using a library and enables up to 10 simultaneous channels of frequency measurement with less that 150 lines of code.  

My version of this code enables two channels of frequency measurement and adds some bells and whistles.  The primary channel, ch0 (Teensy 4.0 pin 6), is used to monitor the VFO's output frequency.  The second channel, ch1 (pin 9), is connected to an external stable and calibrated 10 MHz reference oscillator.  The purpose of the second channel is to record the measurement error and frequency drift of the Teensy's cpu oscillator.  With this reference information the raw ch0 VFO frequency measurement can be corrected with some simple calculations.   

The code reports all the raw ch0 and ch1 frequency data as well as the the corrected ch0 data via serial port so that it may be recorded and analyzed.  The output includes a time stamp recording the elapsed time from the Teensy boot-up (in mS), the raw ch0 frequency data, the ch1 reference frequency data and the corrected ch0 frequency measurement.  

### Output Data Rates and Formats
Data is recorded at a variable interval.  Initially data is recovered at a rate of every 15 seconds and incrementally slows to a period of every 6 minutes by the end of the first half hour and ultimately slows to every 15 minutes after the first hour.  This is done to avoid unnecessarily accumulating massive amounts of data.  Obviously this frequency counter is not intended for general use.  You can customize that data collection rates by modifying the code to suit your needs.  The data reporting rate can be reset back to inital values by pressing the reset button or re-powering the Teensy.  Currently the serial modem data rate is set to 9600 baud.  

One of two data output formats can be selected at the time of compilation.  The code can be compiled to output the basic data in a form that can be easily imported into a spreadsheet for subsequent analysis and graphics.  Alternatively, it can be compiled to provide a report format that provides time stamps in hours, minutes and seconds, all the basic data and it also includes the calculated ch0 VFO and the Teensy clock drift rates.  Look for the printMode variable and set it to BASIC_DATA or REPORT as required.

### 10 MHz Reference Clock Details
I have been using my NorCal FCC-2 DDS frequency generator as my ch1 10 MHz reference clock signal.  The FCC-2 has been zero beat to WWV after a warm-up period and is by all indications very stable.  Interestingly, I have found that the reported frequency error and drift rate for ch1 has been very small after a half hour or so of warm-up.  There are three possible reasons for the good performance. 

1.	The reference clock is rock solid and the reported ch1 frequency error and drift are due to be primarily from the Teensy cpu clock.  (This is the assumed to be the most probable reason as we assume that the 10 MHz reference frequency source has a better and more stable oscillator.)
2.	The Teensy clock is rock solid and the frequency error and drift are due to the reference oscillator.  (Probably not.)
3.	The minimal long term frequency drift of ch1 is the result of nearly equal and opposte drift rates for the reference clock and the Teensy cpu clock that cancel each other out.  This is safely assumed to be not the case.  

One lesson learned here is that cpu clock on my particular Teensy 4.0 is very stable after a period of warm-up.  Frequency measurement error (again for my Teensy) has been less than 100 Hz after a warm-up period.  The long term drift has been in the single digits.

The original FreqCountMany.ino code was setup with a 200 mS count interval resulting in a measurement precision of 5 Hz.  This code is setup with a 1 Second count interval to obtain a precision of 1 Hz.  The slower count interval is not a limitation for this application.  Remember, precision and accuracy are not the same. 

The counter can be used without the supplementary 10 MHz reference clock on ch1.  In this instance the data for ch0 is "as-measured" and none of the data for ch1 is included in the output.  

### Reviewing The Data
The drift rates calculated by the program are for the most recent reported time interval.  The calculation being simply (change in frequency)/(change in time).  When the counter first boots-up data is taken every 15 seconds.  If the VFO is simultaneously starting up from cold conditions, there may be large reported frequency drift rates even though the change in frequency is small.  For example a 16 Hz shift over 15 seconds works out to a rate of 3840 Hz/Hr.  It sounds bad but is probably not un-expected for a cold start-up.  Stepping back and looking at the larger picture, you may be seeing only 600 Hz drift over the first half hour.  Don't panic.  There may be better ways to statistically characterize the drift rate by averaging data from adjacent time steps.  For the moment I am not sure what that would be.  

You will also see some noise in the drift rates due to "chattering" of the 1 Hz digit in the frequency measurement.  This is most apparent when the drift rate is close to zero.  The reported drift rate bounces around zero.  For example the data may be similar to: 0, 0, -20, 10, 10, 0, 0.  The bouncing entirely due to measurement chatter.  The chatter is always there, but is less obvious when there is actual drift.  The data shown in the images below are for a 40m VFO based on pulling a ceramic reasonator.  Clearly not a high end oscillator, but it does perform better than many of the LC oscillators I have built in the past for hobby applications.  

## Calibrating Your Frequency Reference To WWV
A word about zero-beating your reference source to WWV.  This is a very good way to calibrate your reference oscillator.  I suspect most often people attempt this process by zero-beating WWV with the radio in AM mode.  This works but it is difficult to hear an audio zero-beat at zero Hz.  Particularly since the radio may have significant low end audio roll-off at zero Hz.  I find it to be much easier to zero-beat with the radio in CW mode (or SSB).  An advantage of this approach are that the radio's audio circuitry is tuned to your CW offset/sidetone frequency.  Secondly, (assuming your CW offset is 600 Hz), it is much easier to judge a zero heterodyne beat with the 600 Hz tone generated by the WWV carrier when the radio is in CW mode.  Try it.  

## Circuit Diagram
No schematic is included as the input to the Teensy is very simple.  Input ch0 to pin 6 and input ch1 to pin 9.  While my signal conditioning circuit works, it does not provide what I conside to be a satisfactorily output.  Subsequently I have not included a schematic circuit.  I am still looking for a simple circuit that provides a clean output.  IMPORTANT:  The Teensy is a 3.3V device and is not 5V tolerant.  


## Future improvements
- Add a toggle switch to select the report format of the serial output
- Add a temperature probe to record the ambient temperature at the VFO. (I have noted frequency swings that may be related to temperature swings when the heat comes on.)


## Links
- [PJRC.com](https://www.pjrc.com/store/)
- [Teensy 4.0](https://www.pjrc.com/store/teensy40.html)
- [FreqCountMany.ino](https://github.com/PaulStoffregen/FreqCountMany/tree/main)
- [CoolTerm](https://freeware.the-meiers.org)


## Images

![Breadboard](https://github.com/gweep/Teensy_Serial_Logging_Frequency_Counter/blob/main/images/IMG_5861%20small.jpeg)

![basic data](https://github.com/gweep/Teensy_Serial_Logging_Frequency_Counter/blob/main/images/Report.jpg)

![basic data](https://github.com/gweep/Teensy_Serial_Logging_Frequency_Counter/blob/main/images/basic_data.jpg)

![Spreadsheet Plot](https://github.com/gweep/Teensy_Serial_Logging_Frequency_Counter/blob/main/images/spreadsheet_plot.jpg)


