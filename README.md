# Smart Aqua AI Manager 🐟🌊

[![Project Demo](https://img.shields.io/badge/Demo-YouTube-red?style=for-the-badge&logo=youtube)](https://youtube.com/shorts/U3v7Iv6NVRM)
*(Click the badge above or visit: [https://youtube.com/shorts/U3v7Iv6NVRM](https://youtube.com/shorts/U3v7Iv6NVRM) to watch our system in action)*

## 📖 About The Project

The **Smart Aqua AI Manager** is an automated, intelligent IoT system designed to address the challenges of maintaining a stable habitat for aquarium fish.

Developed as an academic project at **FPT University Danang**, this system utilizes a centralized architecture where an **ESP32** microcontroller serves as the primary processing unit. It handles both Cloud-synchronized AI inference and IoT connectivity via the **Blynk** platform. By integrating Machine Learning (Linear Regression) for automated environmental regulation, the system significantly reduces manual maintenance efforts while ensuring a healthy aquatic environment 24/7.

## ✨ Key Features

*   **Real-time Environmental Monitoring**: Continuously tracks vital water metrics including Temperature, Turbidity (water cleanliness), and Water Level.
*   **Hybrid Cloud-Edge AI Actuation**: Utilizes a Linear Regression model trained externally in Python. The trained weights and biases are synchronized via Firebase to the ESP32, which performs edge inference to optimally control the cooling fan and water filter pump.
*   **Automated Smart Feeding**: Schedule-based or on-demand fish feeding mechanism powered by a Servo motor.
*   **Remote IoT Dashboard**: Fully integrated with the **Blynk** mobile application for real-time data visualization (charts, gauges) and manual overrides.
*   **Cost-Effective Architecture**: Achieves high-end smart aquarium features using accessible microcontrollers and sensors without compromising performance.

## 🏗️ System Architecture & Hardware

> **Note**: For in-depth theoretical background, mathematical models, and complete project documentation, please refer to our `final-report.pdf`.

### Block Diagram

The system architecture revolves around the ESP32 acting as the central hub, fetching dynamic AI weights from Firebase while communicating with Blynk Cloud.

*(Place your architecture block diagram image here by replacing this placeholder)*
<!-- ![Architecture](images/architecture.png) -->

### Wiring Schematic

Isolated power distribution ensures the ESP32 operates stably at 3.3V logic while the 5V-2A power supply handles high-load actuators (Relays, Servo).

*(Place your circuit schematic image here by replacing this placeholder)*
<!-- ![Circuit Diagram](images/circuit_diagram.png) -->

## 🧰 Hardware Components

*   **Central Controller**: ESP32 Development Board
*   **Sensors**:
    *   DS18B20 (Water Temperature)
    *   Analog Turbidity Sensor (Water Cleanliness)
    *   HC-SR04 Ultrasonic Sensor (Water Level)
*   **Actuators**:
    *   Relay Module (Controls Water Pump, Lighting, and Cooling Fan)
    *   Servo Motor (Feeding Mechanism)
*   **Display**: LCD 16x2 with I2C module

## 💻 Tech Stack

*   **Embedded Software**: C++ / Arduino IDE
*   **Machine Learning**: Python 3 (`pandas`, `scikit-learn`, `numpy`)
*   **Cloud & IoT Services**:
    *   Firebase Realtime Database (Weight synchronization)
    *   Blynk IoT Platform (Mobile Dashboard)

## ⚙️ Setup & Installation

Follow these instructions to replicate the project on your local machine and hardware.

### 1. ESP32 Firmware Setup

1. Open the project in the Arduino IDE.
2. Install the required libraries via the Library Manager:
   *   `Blynk`
   *   `Firebase ESP32 Client`
   *   `DallasTemperature` & `OneWire`
   *   `NewPing` (for HC-SR04)
   *   `LiquidCrystal_I2C`
3. Navigate to the main `.ino` file and update your credentials:

```cpp
// WiFi Credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Blynk & Firebase Tokens
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"
#define FIREBASE_HOST "YOUR_FIREBASE_PROJECT.firebaseio.com"
#define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_SECRET"
```

4. Compile and upload the code to your ESP32.

### 2. Python AI Model Setup

The AI model predicts the optimal environmental states. It trains on a dataset and sends the calculated weights ($w$) and biases ($b$) to Firebase.

1. Ensure Python 3.x is installed.
2. Install the necessary Python packages:

```bash
pip install pandas scikit-learn pyrebase4 numpy
```

3. Update your Firebase configuration dictionary in `train-ai.py`.
4. Run the training script:

```bash
python train-ai.py
```

*This will read the dataset, train the Linear Regression model, and automatically push the new weights to your Firebase Realtime Database.*

## 👥 Team Members (FPT University Danang)

This project was brought to life by the following software engineering students:

| Name | Role / Contribution |
| :--- | :--- |
| **Trần Đức Việt** | Study conception, System design (25%) |
| **Trần Thanh Tiến Đạt** | ESP32 Programming & IoT integration (25%) |
| **Lê Tự Minh Quang** | Block diagram & Flowchart design (20%) |
| **Nguyễn Đình Bảo** | Documentation & Presentation (10%) |
| **Nguyễn Đình Đạt** | Documentation & Presentation (10%) |
| **Trần Huy Hoàng** | Documentation & Presentation (10%) |

**Supervised by**: Ms. Tra Huong Thi Le

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.