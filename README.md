# A Two Channel 30 MHz Serial Logging Frequency Counter For The Teensy 4.0
This is a two channel 30 MHz logging frequency counter based on the Teensy 4.0 Development Board.  Data is transmitted directly via serial port so that it can be recorded and later analyzed via spreadsheet.  There is no external display to show the measured frequency directly.  You must use a serial terminal program to view the data.   I use CoolTerm which offers a means to save the received data to a file.  Alternatively the data text can be copied directly from the screen of the serial terminal.  

## Background
When building a variable frequency oscillator (VFO) for amateur radio use it is good to know the drift stability and drift characteristics of the oscillator.  I have had many sessions manually logging time and frequency data points of a newly constructed VFO with a frequency counter and a clock so that I could calculate and plot the oscillator's drift rate.  This is a tedious and time consuming effort that is often repeated several times during a VFO's development.   I needed a better way to gather data.

## Details
Paul Stoffregen at PJRC provides the pieces for a nice solution.  By employing a Teensy 4.0 and a modification of his  FreqCountMany.ino code for the Teensy, I was able to automate the task of recording my VFO's frequency drift.  Interestingly; while PJRC has several frequency counter libraries for their Teensy product line, their FreqCountMany.ino code directly accesses the inner workings of the Teensy without using a library and enables up to 10 simultaneous channels of frequency measurement with less that 150 lines of code.  

My version of this code enables two channels of frequency measurement and adds some bells and whistles.  The primary channel, ch0 (Teensy 4.0 pin 6), is used to monitor the VFO's output frequency.  The second channel, ch1 (pin 9), is connected to an external stable and calibrated 10 MHz reference oscillator.  The purpose of the second channel is to record the measurement error and frequency drift of the Teensy's cpu oscillator.  With this reference information the raw ch0 VFO frequency measurement can be corrected with some simple calculations.   

My version of the code reports all the raw ch0 and ch1 frequency data as well as the the corrected ch0 data via serial port so that it may be recorded and analyzed.  The output includes a time stamp recording the elapsed time from the Teensy boot-up (in mS), the raw ch0 frequency data, the ch1 reference frequency data and the corrected ch0 frequency measurement.  

One of two data output formats can be selected at the time of compilation.  The code can be compiled to output the basic data in a form that can be easily imported into a spreadsheet for subsequent analysis and graphics.  Alternatively, it can be compiled to provide a report format that provides time stamps in hours, minutes and seconds, all the basic data and it also includes the calculated ch0 VFO and the Teensy clock drift rates.  

Data is recorded at a variable interval.  Initially data is recovered at a rate of every 15 seconds and incrementally slows to a period of every 6 minutes by the end of the first half hour and ultimately slows to every 15 minutes after the first hour.  This is done to avoid unnecessarily accumulating massive amounts of data.  Obviously this frequency counter is not intended for general use.  You can customize that data collection rates by modifying the code to suit your needs.

I have been using my NorCal FCC-2 DDS frequency generator as my ch1 10 MHz reference clock signal.  The FCC-2 has been zero beat to WWV after a warm-up period and is by all indications very stable.  Interestingly, I have found that the reported frequency error and drift rate for ch1 has been very small after a half hour or so of warm-up.  There are three possible reasons for the good performance. 

1.	The reference clock is rock solid and the reported ch1 frequency error and drift are due to be primarily from the Teensy cpu clock.  (This is the assumed to be the most probable reason as we assume that the 10 MHz reference frequency source has a better and more stable oscillator.)
2.	The Teensy clock is rock solid and the frequency error and drift are due to the reference oscillator.  (Probably not.)
3.	The minimal long term frequency drift of ch1 is the result of nearly equal and opposte drift rates for the reference clock and the Teensy cpu clock that cancel each other out.  This is safely assumed to be not the case.  

So one lesson learned here is that cpu clock on my particular Teensy 4.0 is very stable after a period of warm-up.  Frequency measurement error (again for my Teensy) has been less than 100 Hz after a warm-up period.  Drift is in the single digits.

The original FreqCountMany.ino code was setup with a 200 mS count interval resulting in a measurement precision of 5 Hz.  My code is setup with a 1 Second count interval to obtain a precision of 1 Hz.  The slower count interval is not a limitation for this application.  Remember, precision and accuracy are not the same. 

The counter can be used without the supplementary 10 MHz reference clock on ch1.  In this instance the data for ch0 is "as-measured" and none of the data for ch1 is included in the output.

A word about zero-beating your reference source to WWV.  This is a very good way to calibrate your reference oscillator.  I suspect most often people attempt this process by zero-beating WWV with the radio in AM mode.  This works but it is difficult to hear an audio zero-beat at zero Hz.  Particularly since the radio may have significant audio roll-off at zero Hz.  I find it to be much easier to zero-beat with the radio in CW mode (or SSB).  An advantage of this approach are that the radio's audio circuitry is tuned to your CW offset/sidetone frequency.  Secondly, assuming you CW offset is 600 Hz, it is much easier to judge a zero heterodyne beat with the 600 Hz tone generated by WWV when the radio is in CW mode.  Try it.  

## Circuit Diagram
No schematic is included as the input to the Teensy is very simple.  Input ch0 to pin 6 and input ch1 to pin 9.  While my signal conditioning circuit works, it does not provide what I consided to be a satisfactorily output.  So I have not included a schematic circuit.  I am still looking for a simple circuit that provides a clean output.  IMPORTANT:  The Teensy is a 3.3V device and is not 5V tolerant.  


## Future improvements
Add a toggle switch to select the report format of the serial output
Add a temperature probe to record the ambient temperature at the VFO.


## Links
https://www.pjrc.com/store/
https://www.pjrc.com/store/teensy40.html
https://github.com/PaulStoffregen/FreqCountMany/tree/main

