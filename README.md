# Arduino Led Music Pulse

Controls an LED light-strip using an Arduino to pulse to the beat of a input from a 3.5mm audio device
[View the Demo.](https://youtu.be/wHUdtZS7I78)

## Getting Started
---

### Prerequisites

You will need to download the following libraries:
* [ArduinoFFT](https://github.com/kosme/arduinoFFT)
* [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
* [MicroSmooth](https://github.com/asheeshr/Microsmooth)

### Hardware

Read [HARDWARE.md](HARDWARE.md) to make sure you are using the correct parts.

### Schematic

Please reference [audio-in-schematic.png](audio-in-schematic.png) to connect the audio-in circuit correctly.  
For the light strip: 5V connects to Arduino/external 5V, GND to Arduino/external GND and DATA to Arduino's Digital Pin 6.

### Installing

Once you have downloaded the files and libraries, upload the code to your Arduino.  
You can then plug in your phone or computer to the 3.5mm audio jack.  
Provde power to your arduino and begin playing music.  

## Authors

* **Jeff St. Jean** - *Initial work* - [JeffStJean](https://github.com/jeffstjean)

## License

This project is licensed under the GNU v3 License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

[Devon Crawford](https://www.youtube.com/channel/UCDrekHmOnkptxq3gUU0IyfA) for the inspiration for this project!  
[This StackExchange post](https://arduino.stackexchange.com/questions/25483/audio-input-into-a0) for help getting the voltage divider correctly wired.  
[This Arduino Forum post](https://forum.arduino.cc/index.php?topic=508537.0) for guidance.  
