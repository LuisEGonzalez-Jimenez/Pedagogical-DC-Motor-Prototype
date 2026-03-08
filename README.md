# Pedagogical DC Motor Control Platform

This repository contains the software developed for an **IoT-based pedagogical platform for teaching automatic control and embedded systems** using a DC motor system controlled by an ESP32 microcontroller.

The platform allows students to interact with a real physical system and analyze motor responses using different control signals.

---

## System Overview

The system consists of an ESP32-based DC motor controller and a MATLAB App Designer graphical interface that allows users to send control commands and visualize motor signals in real time.

The system supports both **MQTT communication** and **direct TCP/IP communication via WiFi**.

---

## Repository Structure

Pedagogical-DC-Motor-Prototype
│
├── ESP32_Firmware
│   ├── main.ino
│   └── README.md
│
└── MATLAB_App_Designer
    ├── DC_Motor_Control_GUI.mlapp
    └── README.md
---

## Features

- Step response experiments
- Sinusoidal input experiments
- PID control of motor speed
- Real-time visualization of motor signals
- Communication using MQTT or TCP/IP
- Current measurement using INA219
- Touchscreen interface on ESP32

---

## Hardware Components

The platform is designed using the following hardware:

- ESP32 microcontroller
- DC motor with encoder
- TB6612FNG motor driver
- INA219 current sensor
- ILI9341 TFT touchscreen display

---

## Educational Purpose

This platform was developed as a teaching tool for courses related to:

- Automatic Control
- Embedded Systems
- Internet of Things (IoT)

Students can analyze real-time system responses and experiment with different control strategies.

---

## Authors

Luis Ricardo Rivera-Goitia  
Luis Enrique González-Jiménez  

Department of Electronics, Systems and Informatics  
ITESO – Universidad Jesuita de Guadalajara
