# Lidar-Sensor-Graph-Analysis
Project Overview:
This project is a C-based application that processes LIDAR sensor data, detects lines using the RANSAC (Random Sample Consensus) algorithm, and visualizes the results in real time.
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

How to Build
Make sure you have:
Allegro 5 installed
libcurl installed
A C compiler (e.g., MinGW / MSVC)
