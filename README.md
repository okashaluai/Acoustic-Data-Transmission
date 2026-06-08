# Acoustic Data Exfiltration

## Used Hardware:
1. `esp8266` microcontroller
2. `LM393` Analog microphone module
3. `pins` and `soldering wire` (to solder the pins on the microcontroller)
4. `3x jumper wires`
5. `Micro-B` usb
***
## Setup
First of all we need to solder the pins on the `esp8266` microcontroller:

![microcontrollerandpins.png](../_resources/microcontrollerandpins.png)
<div style="page-break-after: always;"></div>

After soldering the pins:
![photo_5841502251434839951_y.jpg](../_resources/photo_5841502251434839951_y.jpg)

Now, after we soldered the pins on the microcontroller, we need to connect the analog microphone module via the jumper wires:
![photo_5841502251434839959_y.jpg](../_resources/photo_5841502251434839959_y.jpg)
<div style="page-break-after: always;"></div>

**We connect them as follows:**

Microphone:A0 -> Microcontroller:A0
Microphone:G -> Microcontroller:G
Microphone:+ -> Microcontroller:3V3
Microphone:D0 -> digital output leave it disconnected.


![photo_5841502251434839956_y.jpg](../_resources/photo_5841502251434839956_y.jpg)

Connect it to the compute and configure the arduino IDE, choose the respective board (in our case Generic ESP8266 Module) and the port.

<div style="page-break-after: always;"></div>

## Python Script (Software-Defined Transmitter)
**The Algorithm: On-Off Keying (OOK)**
-  To send a 1, we turn the carrier wave (the 2000 Hz tone) ON.
-  To send a 0, we turn the carrier wave OFF (complete silence).

**Physics Configuration**
- Frequency = 2000 Hz (the carrier wave)
- Baud Rate = 10 bits/second (every single 1 or 0 is being played for 0.1 seconds)
- Sample Rate = 44100

**The Payload Builder**
```
def text_to_binary(text):
    # Convert text to binary
    binary_payload = ''.join(format(ord(char), '08b') for char in text)
    # Prepend a '1' Start Bit to wake up the receiver
    return '1' + binary_payload

```

Could be improved by adding compression, encryption and error detection.

**The Digital Signal Processor**

```
t = np.linspace(0, BIT_DURATION, int(SAMPLE_RATE * BIT_DURATION), endpoint=False)
```

`t` is a "Time Array", since we defined our bit duration to be 0.1s, t is exactly 4,410 microscopic time slices from 0.0 to 0.1.

```
    for bit in binary_string:
        if bit == '1':
            wave = VOLUME * np.sin(2 * np.pi * FREQ * t)
        else:
            wave = np.zeros(len(t))

        audio_signal = np.concatenate((audio_signal, wave))
```

If the bit is a 1: we apply the standard physics equation for a continuous sine wave ($y(t)=A \cdot sin(2 \cdot \pi \cdot f \cdot t)$). This generates a perfect 2000Hz wave for 100ms.
If the bit is a 0, we use np.zeros to create an array of absolute silence for 100ms.

All 100ms chunks are being concatenated to the `audio_data` array (example If the message is 101, it literally glues [100ms Beep] + [100ms Silence] + [100ms Beep] into one giant, seamless audio file).

**The Hardware Execution**

```
    sd.play(audio_data, SAMPLE_RATE)
    sd.wait()

```

- The `sounddevice`  library (sd) acts as the bridge between the software math and the computer's physical hardware.
-  It takes the giant numpy array of voltage points and streams them into the computer's Digital-to-Analog Converter (DAC) at the sample rate (in this cate 44,100 points per second).
-  The sd.wait() freezes the python script untill the audio finishes playing to prevent overlapping.

***
## Arduino Code (Receiver)
We define the noise threshold based on our controlled environment to be 2, this should be adjusted based on the hosting entironment.

```
#define NOISE_THRESHOLD 2  // Anything > 2 is considered a beep
```

**The Peak-to-Peak Envelope Detector (The 20ms Chunk)**

```
// Sample audio for 20ms (1/5th of a bit)
unsigned long startMillis = millis();

while (millis() - startMillis < 20) {
	int sample = analogRead(MIC_PIN);
	if (sample < min_val) min_val = sample;
	if (sample > max_val) max_val = sample;
}

int current_volume = max_val - min_val;

```

The code listens to 20ms windows (chuncks) and records the absolute highest and lowest peaks during the chunk, then it calculates the true amplitude or current volume of the room as follows: $current_volume = max\_volume - min\_volume$ for the exact slice of time.

**The Wake-Up Trigger**

```
if (!is_receiving && current_volume >= NOISE_THRESHOLD) {
	is_receiving = true;
	// ... resets all variables
}
```

Once the `current_volume` crosses the `NOISE_THRESHOLD`, it gets into "Data Collection Mode". Otherwise, it is ignored.

**The Digital Integrator Filter**

```
volume_sum += current_volume;
chunks_collected++;

// Every 5 chunks (100ms), we have completed exactly 1 bit
if (chunks_collected == 5) {
	int current_bit = (volume_sum > 5) ? 1 : 0;
	...
```

The peaks are being summed every 5 chunks (100ms) which represents 1 bit (100ms). The code checks if the accumulated sum is at least 5, it decides that the bit was a 1. Otherwise, it was a 0.
This is a try to resist the loss of information such as sound waves cancellation due to some disruption or whatever.

**The ASCII Reconstructor**
```
if (bits_received > 0) {
	binary_buffer += String(current_bit);

	if (binary_buffer.length() == 8) {
		char decoded = (char)strtol(binary_buffer.c_str(), NULL, 2);
		Serial.print(decoded);
		binary_buffer = "";
	}
}
```

Every 8 bits (wake-up bit is ignored) represent a 1 byte number that is being translated to an ascii caharacter

**The Reset Mechanism**

```
if (silence_counter > 50) {
         is_receiving = false;
         Serial.println("\nTransmission Ended. Waiting for next message...");
}
```

If the room goes quiet, the `silence_counter` ticks up and the receiver resets itself and busy waits for the next data transmission. Because each loop is 20ms, waiting for 50 silent chunks means waiting for exactly 1 full second of silence ($50 \cdot 20ms = 1000ms$).

<div style="page-break-after: always;"></div>

## Attachments

1. `transmitter.py`
2. `receiver.ino`
3. `demo.mp4`
