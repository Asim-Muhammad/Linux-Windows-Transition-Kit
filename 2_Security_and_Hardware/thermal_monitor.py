#!/usr/bin/env python3
import time
import glob
import subprocess

WARNING_TEMP = 85.0 # Degrees Celsius
SAFE_TEMP = 70.0    # Degrees Celsius

def get_max_temp():
    max_t = 0.0
    for sensor in glob.glob('/sys/class/thermal/thermal_zone*/temp'):
        try:
            with open(sensor, 'r') as f:
                t = float(f.read().strip()) / 1000.0
                if t > max_t and t < 150.0:
                    max_t = t
        except:
            pass
    return max_t

def main():
    is_overheating = False
    
    while True:
        temp = get_max_temp()
        
        # Trigger alert if hitting 85C
        if temp >= WARNING_TEMP and not is_overheating:
            is_overheating = True
            subprocess.run([
                "notify-send", "-u", "critical", "-a", "Thermal Monitor", "-i", "dialog-warning",
                "🔥 High Temperature Alert!",
                f"Your system is running very hot: {temp:.1f}°C\nConsider letting it cool down."
            ])
            
        # Send an ALL CLEAR once it cools down to 70C
        elif temp <= SAFE_TEMP and is_overheating:
            is_overheating = False
            subprocess.run([
                "notify-send", "-u", "normal", "-a", "Thermal Monitor", "-i", "dialog-information",
                "❄️ Temperature Normalized",
                f"Your system has cooled down to a safe {temp:.1f}°C!"
            ])
            
        time.sleep(10)

if __name__ == "__main__":
    main()
