import serial
import threading
import time
from colorama import init, Fore, Style

# Initialize colorama for Windows/Mac support
init(autoreset=True)

# Update these to match your actual laptop COM ports
ESP32_PORT = 'COM3'  
STM32_PORT = 'COM8'
BAUD_RATE = 115200

def read_port(port_name, color, prefix):
    try:
        with serial.Serial(port_name, BAUD_RATE, timeout=1) as ser:
            print(f"{color}Connected to {prefix} on {port_name}{Style.RESET_ALL}")
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"{color}[{prefix}] {line}{Style.RESET_ALL}")
    except Exception as e:
        print(f"{Fore.RED}Error on {port_name}: {e}{Style.RESET_ALL}")

# Launch a thread for each microcontroller
t1 = threading.Thread(target=read_port, args=(ESP32_PORT, Fore.CYAN, 'ESP32-TX'), daemon=True)
t2 = threading.Thread(target=read_port, args=(STM32_PORT, Fore.GREEN, 'STM32-RX'), daemon=True)

t1.start()
t2.start()

print("Stress Test Running... Press Ctrl+C to stop.")
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nStress Test Halted.")