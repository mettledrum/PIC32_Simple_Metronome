Simple Metronome for PIC32
Andrew Hoyle

This is a simple metronome program written for PIC32.  Upon reset, it shows the battery life, and then waits for the user to click the button twice to set the metronome tempo.  If the clicks are too slow, an error message shows, otherwise, the LED lights show the tempo that was clicked by the user until reset.  (BPM is displayed on the LCD screen, and the LEDs blip on the beat.)

This program is shared with y'all to show my competency with ISRs and timers when handling user taps.  (I didn't write the debouncing code for the buttons, those were written by my teacher while we were using the PIC32 for a Simon Says game.  I also didn't write the LCD display code; those were provided in class.)

Two different timers were used, one starts upon the first user tap, and the second one kicks into gear once the button has been pressed twice.  The ISR attached to the first timer simply increments a value between button presses.  It then feeds this interval of time to another timer that cues the blips.

I apologize for the spaghetti-ness of these files because I left several functions in here that were just used for prior projects.  But I think the main function and ISRs are worth a gander.