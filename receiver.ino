// Wemos D1 Mini - Final ASK / OOK Decoder

#define MIC_PIN A0
#define NOISE_THRESHOLD 2  // Anything > 2 is considered a beep

bool is_receiving = false;
int chunks_collected = 0;
long volume_sum = 0;
String binary_buffer = "";
int bits_received = 0;
int silence_counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- ASK Decoder Ready ---");
  Serial.println("Waiting for Start Bit...");
}

void loop() {
  int min_val = 1024;
  int max_val = 0;
  
  // 1. Sample audio for 20ms (1/5th of a bit)
  unsigned long startMillis = millis();
  while (millis() - startMillis < 20) {
    int sample = analogRead(MIC_PIN);
    if (sample < min_val) min_val = sample;
    if (sample > max_val) max_val = sample;
  }
  
  int current_volume = max_val - min_val;

  // Track silence to know when the message is completely over
  if (current_volume < NOISE_THRESHOLD) {
     silence_counter++;
  } else {
     silence_counter = 0;
  }

  // 2. Wake up on the START BIT (the first loud sound)
  if (!is_receiving && current_volume >= NOISE_THRESHOLD) {
     is_receiving = true;
     chunks_collected = 0;
     volume_sum = 0;
     binary_buffer = "";
     bits_received = 0;
     silence_counter = 0;
     Serial.print("Receiving: ");
  }

  // 3. Process the bits
  if (is_receiving) {
     volume_sum += current_volume;
     chunks_collected++;

     // Every 5 chunks (100ms), we have completed exactly 1 bit
     if (chunks_collected == 5) {
        
        // If the total volume over 100ms was higher than noise, it's a 1
        int current_bit = (volume_sum > 5) ? 1 : 0;
        
        // We ignore the very first bit because it is just the wake-up Start Bit
        if (bits_received > 0) {
            //Serial.print(current_bit);
            binary_buffer += String(current_bit);

            // Every 8 bits, print the translated text character
            if (binary_buffer.length() == 8) {
                char decoded = (char)strtol(binary_buffer.c_str(), NULL, 2);
                //Serial.print(" [");
                Serial.print(decoded);
                //Serial.print("] ");
                binary_buffer = ""; // Clear buffer for the next letter
            }
        }
        
        bits_received++;
        chunks_collected = 0;
        volume_sum = 0;
     }

     // 4. Stop receiving if we hear silence for a full second (50 chunks)
     if (silence_counter > 50) {
         is_receiving = false;
         Serial.println("\nTransmission Ended. Waiting for next message...");
     }
  }
}