# Lidar-Sensor-Graph-Analysis
Project Overview:

This project is a C-based application that processes LIDAR sensor data, detects lines using the RANSAC (Random Sample Consensus) algorithm, and visualizes 

the results in real time.

The application:

Downloads .toml LIDAR data via libcurl

Converts polar coordinates to Cartesian coordinates

Detects dominant lines using RANSAC

Computes intersections between detected lines

Calculates intersection angles

Visualizes everything using Allegro 5



Algorithm Explanation:

Data Parsing

RANSAC Line Detection

Intersection Analysis

Visualization


How to Build:

Make sure you have;

Allegro 5 installed

libcurl installed

A C compiler (e.g., MinGW / MSVC)


Screenshots:

<img width="280" height="859" alt="Ekran görüntüsü 2026-02-19 140540" src="https://github.com/user-attachments/assets/356b87c3-e65d-4ebd-9250-c8e8caa3ca6e" />


<img width="277" height="449" alt="Ekran görüntüsü 2026-02-19 140459" src="https://github.com/user-attachments/assets/3ce423a1-5630-422e-b641-228b07a021cb" />


<img width="349" height="375" alt="Ekran görüntüsü 2026-02-19 140449" src="https://github.com/user-attachments/assets/8c3acc8d-0432-4e30-9ae3-93f5a643653f" />
