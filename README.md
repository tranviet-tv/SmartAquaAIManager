# Smart Aqua AI Manager

**An AI-powered IoT solution for proactive aquarium health management.**

> ⚠️ **Work in Progress** — Academic Project (SSG104)

---

## 🐟 Project Overview
Smart Aqua AI Manager is an integrated IoT + AI system designed to automate and optimize aquarium care. It combines real-time sensor monitoring with machine learning-based decision making to keep water conditions stable, manage feeding schedules, and notify users when intervention is needed.

This project was developed as part of the **SSG104** course and focuses on creating a smart ecosystem for aquarium maintenance.

---

## 🚀 Key Features
- **Real-time Environmental Monitoring**: Tracks temperature, turbidity, and water level using onboard sensors.
- **AI-Powered Behavioral Logic**: Uses a simple linear regression model to decide when to activate pumps and fans based on sensor readings.
- **Automated Maintenance**: Controls relays for lighting, pump, and cooling fan; includes scheduled automatic feeding using a servo.
- **Remote Control & Telemetry**: Integrates with **Blynk** for remote control, data visualization, and notifications.

---

## 🛠 Tech Stack
- **Firmware**: Arduino/ESP32 (C++)
- **Cloud UI**: Blynk (mobile IoT dashboard)
- **Hardware**:
  - ESP32 microcontroller
  - Ultrasonic sensor (water level)
  - Temperature sensor (DS18B20)
  - Turbidity sensor
  - Servo motor (feeder)
  - Relays (pump, fan, lights)

---

## 📁 Repository Structure
- `code_esp32_smartAqua.ino` – Main Arduino sketch implementing sensor reading, AI control logic, and Blynk integration.

---

## ⚙️ Getting Started (Local Development)
1. **Install Arduino IDE** and set up the ESP32 board support.
2. Open `code_esp32_smartAqua.ino` in the Arduino IDE.
3. Update Wi-Fi and Blynk credentials at the top of the file:
   - `BLYNK_AUTH_TOKEN`
   - `ssid` and `pass`
4. Upload to your ESP32.

---

## 📌 Notes & Extensions
- The current AI logic is a simple linear regression model trained on a small dataset. It is intended as a proof-of-concept and can be replaced with more advanced ML models (e.g., TensorFlow Lite) in the future.
- You can extend the platform to include pH monitoring, dissolved oxygen sensors, or computer vision-based fish behavior analysis for advanced health scoring.

---

## 📜 License
This project is provided for academic learning purposes. Feel free to adapt and extend it for personal or educational use.

---

## 🙋‍♂️ Contact
For questions or collaboration ideas, feel free to open an issue or reach out via the repository.
