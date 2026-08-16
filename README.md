# 🤖 AVYANA — Autonomous Underwater Vehicle
### RoboFest 5.0 | GEC Bhavnagar | 🏆 ₹2.5L Total Prize Money

![AUV](https://img.shields.io/badge/RoboFest-5.0-blue)
![Ideation](https://img.shields.io/badge/Ideation%20Prize-₹50K-silver)
![POC](https://img.shields.io/badge/POC%20Prize-₹2L-gold)
![ESP32](https://img.shields.io/badge/ESP32-Mission%20Control-green)
![Roboflow](https://img.shields.io/badge/Roboflow-Computer%20Vision-purple)
![KiCad](https://img.shields.io/badge/PCB-KiCad-blue)

---

## 📸 CAD Design

![AVYANA CAD](images/cad.png)

---

## 🎯 Overview

**AVYANA** is a fully autonomous underwater vehicle built
for RoboFest 5.0, inspired by the SAUVC (Singapore AUV
Challenge) task framework.

The vehicle operates completely untethered — fully autonomous
decision making using computer vision and IMU-based
stabilization. Competed against tethered vehicles and
won a total of ₹2.5 Lakh across two stages.

**Role:** Team Lead  
**Team Size:** 4 members  
**Institution:** GEC Bhavnagar, Gujarat  
**Competition:** RoboFest 5.0  

---

## 🏆 Competition Results

```
Stage 1 — Ideation:
Prize: ₹50,000
Task:  Concept presentation and technical documentation

Stage 2 — POC (Proof of Concept):
Prize: ₹2,00,000
Task:  SAUVC inspired — gate navigation,
       object detection, ball drop mechanism

Total Prize: ₹2,50,000

Note: Competed fully untethered against tethered vehicles.
      Last minute task change by organizers —
      adapted mission on the spot.
```

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────┐
│           Raspberry Pi              │
│  ┌─────────────┐  ┌──────────────┐  │
│  │  Roboflow   │  │   Mission    │  │
│  │  CV Model   │  │   Planner    │  │
│  │ (YOLO .pt)  │  │  (Python)    │  │
│  └──────┬──────┘  └──────┬───────┘  │
│         └────────┬────────┘          │
│              UART Serial             │
└──────────────────┼───────────────────┘
                   │ Commands
                   ▼
┌─────────────────────────────────────┐
│              ESP32                  │
│  ┌──────────┐  ┌──────────────────┐ │
│  │  BNO08x  │  │ Thruster Control │ │
│  │   IMU    │  │  FL FR RL RR     │ │
│  │Yaw/Pitch │  │  VF VR (depth)   │ │
│  └──────────┘  └──────────────────┘ │
│  ┌──────────┐  ┌──────────────────┐ │
│  │ Gripper  │  │   Arm Switch     │ │
│  │  Servo   │  │  Safety System   │ │
│  └──────────┘  └──────────────────┘ │
└─────────────────────────────────────┘
```

---

## ⚙️ Hardware Components

| Component | Specification | Purpose |
|---|---|---|
| Raspberry Pi 5 | Main computer | CV inference + mission planning |
| ESP32 | Microcontroller | Thruster control + IMU |
| IMX708 Camera x2 | Stereo vision | Computer vision input |
| BNO08x IMU | 9-DOF sensor | Yaw/Pitch/Roll stabilization |
| Thrusters x6 | 3D printed | Propulsion + depth control |
| ESCs x6 | 30A | Electronic speed controllers |
| LiPo Battery | High capacity | Power system |
| Buck Converters x2 | Custom PCB | Voltage regulation |
| Leak Sensor | Safety | Flood detection |
| Current Sensor | Monitoring | Power management |
| Servo | Gripper | Ball drop mechanism |

---

## 🔧 Custom PCB

Designed from scratch in KiCad — production ready Gerber
files included in `/pcb` directory.

```
Layers:
→ F_Cu / B_Cu     — Front and back copper routing
→ F_Mask / B_Mask — Solder mask layers
→ Silkscreen      — Component labels
→ Edge_Cuts       — Board outline
→ Drill files     — PTH and NPTH holes

Features:
→ Dual buck converter regulation
→ ESP32 integration headers
→ ESC power distribution
→ Sensor connection headers
→ Current monitoring circuit
```

[View PCB Gerber Files](pcb/avyana_auv_pcb.zip)

---

## 🧠 Software Stack

### Computer Vision (Raspberry Pi)
```python
# Roboflow SAUVC trained YOLO model
# Formats: .pt (PyTorch) + .onnx (deployment)
# Real-time detection:
# → Gate detection for navigation
# → Target object identification  
# → Path following underwater
```

### Dual Mode Operation
```
Primary Mode:
Roboflow CV model → Pi mission planner → ESP32 commands
Real time visual decision making

Fallback Mode:
If CV fails or low confidence →
Pre-programmed mission sequence executes automatically
Ensures AUV completes mission regardless of CV failure
```

### ESP32 Mission Control
```
→ Quaternion IMU reading (BNO08x ARVR stabilized)
→ Yaw PID correction loop (Kp = 0.3)
→ 6 thruster PWM control (1000-2000μs)
→ UART command parsing from Pi
→ Safety arm switch monitoring (50ms polling)
→ Auto-zero IMU calibration on boot
→ Gripper servo control (0-90 degrees)
```

### Command Protocol (Pi → ESP32)
```
PING        → Health check → returns READY
STOP        → Emergency stop all thrusters
FWD:[val]   → Forward speed (-100 to 100)
YAW:[deg]   → Yaw target in degrees
DEPTH:[val] → Vertical thruster speed
DROP        → Ball drop sequence
OPEN/CLOSE  → Gripper control
REZERO      → Recalibrate IMU zero point
```

---

## 🏭 Manufacturing

```
Hull:        Acrylic pressure tube (store bought)
Frame:       Custom 3D printed components
PCB:         KiCad designed — Gerber files included
             (manufactured at PCB fab)
Assembly:    Custom waterproofing and cable management
```

---

## 🔐 Security Analysis

*AI and embedded security perspective on AUV attack surfaces:*

```
[VULN-01] Computer Vision Adversarial Attack
Severity: Critical
Attack:   Adversarial pattern placed near gate/target
          CV model misidentifies object
          AUV navigates to wrong location
Fix:      Adversarial training, input preprocessing,
          multi-sensor fusion

[VULN-02] UART Communication Interception
Severity: High
Attack:   Inject fake commands on Pi→ESP32 serial
          Physical access to tether/electronics
Fix:      UART encryption, command authentication,
          checksum verification

[VULN-03] IMU Spoofing
Severity: High
Attack:   Strong magnetic field disrupts BNO08x
          AUV loses orientation — uncontrolled movement
Fix:      Multiple IMU cross-validation,
          anomaly detection on sensor readings

[VULN-04] Roboflow Model Extraction
Severity: Medium
Attack:   Repeated queries reverse engineer model weights
          Competitor learns your detection strategy
Fix:      Model watermarking, local inference only

[VULN-05] Training Data Poisoning
Severity: Medium
Attack:   Poison SAUVC public dataset on Roboflow
          Backdoor trigger causes mission failure
Fix:      Dataset auditing, private training data,
          model behavior verification
```
