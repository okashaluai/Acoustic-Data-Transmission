import numpy as np
import sounddevice as sd

# --- ASK Protocol Configuration ---
FREQ = 2000         
BAUD_RATE = 10      # 10 bits per second = 100ms per bit
SAMPLE_RATE = 44100 
VOLUME = 1.0        # Max volume!

BIT_DURATION = 1.0 / BAUD_RATE 

def text_to_binary(text):
    # Convert text to binary
    binary_payload = ''.join(format(ord(char), '08b') for char in text)
    # Prepend a '1' Start Bit to wake up the receiver
    return '1' + binary_payload

def generate_ask_audio(binary_string):
    audio_signal = np.array([])
    t = np.linspace(0, BIT_DURATION, int(SAMPLE_RATE * BIT_DURATION), endpoint=False)

    for bit in binary_string:
        if bit == '1':
            wave = VOLUME * np.sin(2 * np.pi * FREQ * t)
        else:
            wave = np.zeros(len(t))
            
        audio_signal = np.concatenate((audio_signal, wave))

    return audio_signal

# --- Main Execution ---
while True:
    message = input("Enter a message to transmit: ")
    print(f"Original Message: {message}")
    binary_msg = text_to_binary(message)
    print(f"Payload (with Start Bit): {binary_msg}")
    
    print("Generating audio...")
    audio_data = generate_ask_audio(binary_msg)
    
    print("Transmitting...")
    sd.play(audio_data, SAMPLE_RATE)
    sd.wait() 
    print("Transmission complete!")