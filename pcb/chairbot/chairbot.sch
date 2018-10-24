EESchema Schematic File Version 4
LIBS:chairbot-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 2
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L 74xx_IEEE:7408 U?
U 3 1 5BC3AC55
P 3350 5450
F 0 "U?" H 3350 5815 50  0000 C CNN
F 1 "74HCT08" H 3350 5724 50  0000 C CNN
F 2 "" H 3350 5450 50  0001 C CNN
F 3 "" H 3350 5450 50  0001 C CNN
	3    3350 5450
	1    0    0    -1  
$EndComp
$Comp
L 74xx_IEEE:7408 U?
U 4 1 5BC3ACB2
P 3350 6250
F 0 "U?" H 3350 6615 50  0000 C CNN
F 1 "74HCT08" H 3350 6524 50  0000 C CNN
F 2 "" H 3350 6250 50  0001 C CNN
F 3 "" H 3350 6250 50  0001 C CNN
	4    3350 6250
	1    0    0    -1  
$EndComp
$Comp
L esp32mini:ESP32mini U?
U 1 1 5BC5AE9F
P 3200 3350
F 0 "U?" H 3175 5687 60  0000 C CNN
F 1 "ESP32mini" H 3175 5581 60  0000 C CNN
F 2 "ESP32mini:ESP32mini" H 3250 5650 60  0001 C CNN
F 3 "" H 2750 3800 60  0001 C CNN
	1    3200 3350
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x05 J?
U 1 1 5BC5AF8D
P 8300 3800
F 0 "J?" H 8380 3842 50  0000 L CNN
F 1 "Conn_01x05" H 8380 3751 50  0000 L CNN
F 2 "" H 8300 3800 50  0001 C CNN
F 3 "~" H 8300 3800 50  0001 C CNN
	1    8300 3800
	1    0    0    -1  
$EndComp
Text Notes 8500 3650 0    50   ~ 0
I2C
$Comp
L 74xx:74HCT244 U?
U 1 1 5BC4FC9B
P 5300 1900
F 0 "U?" H 5100 2600 50  0000 C CNN
F 1 "74HCT244" H 5550 2600 50  0000 C CNN
F 2 "" H 5300 1900 50  0001 C CNN
F 3 "http://www.nxp.com/documents/data_sheet/74HC_HCT244.pdf" H 5300 1900 50  0001 C CNN
	1    5300 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	5800 1600 6150 1600
$Comp
L power:GND #PWR0101
U 1 1 5BC52EFA
P 5300 2900
F 0 "#PWR0101" H 5300 2650 50  0001 C CNN
F 1 "GND" H 5305 2727 50  0000 C CNN
F 2 "" H 5300 2900 50  0001 C CNN
F 3 "" H 5300 2900 50  0001 C CNN
	1    5300 2900
	1    0    0    -1  
$EndComp
Wire Wire Line
	5300 2900 5300 2800
Wire Wire Line
	4800 2300 4700 2300
Wire Wire Line
	4700 2300 4700 2400
Wire Wire Line
	4700 2800 5300 2800
Connection ~ 5300 2800
Wire Wire Line
	5300 2800 5300 2700
Wire Wire Line
	4800 2400 4700 2400
Connection ~ 4700 2400
Wire Wire Line
	4700 2400 4700 2800
$Comp
L power:+5V #PWR0102
U 1 1 5BC56499
P 5300 900
F 0 "#PWR0102" H 5300 750 50  0001 C CNN
F 1 "+5V" H 5315 1073 50  0000 C CNN
F 2 "" H 5300 900 50  0001 C CNN
F 3 "" H 5300 900 50  0001 C CNN
	1    5300 900 
	1    0    0    -1  
$EndComp
$Comp
L Device:C C?
U 1 1 5BC56588
P 5550 1000
F 0 "C?" V 5298 1000 50  0000 C CNN
F 1 "C" V 5389 1000 50  0000 C CNN
F 2 "" H 5588 850 50  0001 C CNN
F 3 "~" H 5550 1000 50  0001 C CNN
	1    5550 1000
	0    1    1    0   
$EndComp
Wire Wire Line
	5300 900  5300 1000
Wire Wire Line
	5300 1000 5400 1000
Connection ~ 5300 1000
Wire Wire Line
	5300 1000 5300 1100
$Comp
L power:GND #PWR0103
U 1 1 5BC59006
P 5950 1050
F 0 "#PWR0103" H 5950 800 50  0001 C CNN
F 1 "GND" H 5955 877 50  0000 C CNN
F 2 "" H 5950 1050 50  0001 C CNN
F 3 "" H 5950 1050 50  0001 C CNN
	1    5950 1050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 1000 5950 1000
Wire Wire Line
	5950 1000 5950 1050
$Sheet
S 8550 1500 500  150 
U 5BC5FAEF
F0 "Motor driver" 50
F1 "motor.sch" 50
$EndSheet
Text GLabel 6150 1400 2    50   Input ~ 0
PWM_A_L
Text GLabel 6150 1500 2    50   Input ~ 0
PWM_A_R
Text GLabel 6150 1600 2    50   Input ~ 0
PWM_B_L
Text GLabel 6150 1700 2    50   Input ~ 0
PWM_B_R
Wire Wire Line
	5800 1700 6150 1700
Wire Wire Line
	5800 1500 6150 1500
Wire Wire Line
	6150 1400 5800 1400
$Comp
L power:GND #PWR0104
U 1 1 5BD26651
P 8000 4100
F 0 "#PWR0104" H 8000 3850 50  0001 C CNN
F 1 "GND" H 8005 3927 50  0000 C CNN
F 2 "" H 8000 4100 50  0001 C CNN
F 3 "" H 8000 4100 50  0001 C CNN
	1    8000 4100
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0105
U 1 1 5BD266EE
P 8000 3500
F 0 "#PWR0105" H 8000 3350 50  0001 C CNN
F 1 "+3.3V" H 8015 3673 50  0000 C CNN
F 2 "" H 8000 3500 50  0001 C CNN
F 3 "" H 8000 3500 50  0001 C CNN
	1    8000 3500
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J?
U 1 1 5BD267A0
P 5250 5350
F 0 "J?" H 5170 5025 50  0000 C CNN
F 1 "Conn_01x02" H 5170 5116 50  0000 C CNN
F 2 "" H 5250 5350 50  0001 C CNN
F 3 "~" H 5250 5350 50  0001 C CNN
	1    5250 5350
	-1   0    0    1   
$EndComp
Text Notes 4850 5300 0    50   ~ 0
24V IN
$Comp
L power:GND #PWR?
U 1 1 5BD2687D
P 5550 5700
F 0 "#PWR?" H 5550 5450 50  0001 C CNN
F 1 "GND" H 5555 5527 50  0000 C CNN
F 2 "" H 5550 5700 50  0001 C CNN
F 3 "" H 5550 5700 50  0001 C CNN
	1    5550 5700
	1    0    0    -1  
$EndComp
Wire Wire Line
	5450 5350 5550 5350
$Comp
L Device:Fuse F?
U 1 1 5BD26A01
P 5750 5250
F 0 "F?" V 5553 5250 50  0000 C CNN
F 1 "Fuse" V 5644 5250 50  0000 C CNN
F 2 "" V 5680 5250 50  0001 C CNN
F 3 "~" H 5750 5250 50  0001 C CNN
	1    5750 5250
	0    1    1    0   
$EndComp
$Comp
L Device:D_Schottky D?
U 1 1 5BD26B72
P 6050 5450
F 0 "D?" V 6004 5529 50  0000 L CNN
F 1 "SS34" V 6095 5529 50  0000 L CNN
F 2 "" H 6050 5450 50  0001 C CNN
F 3 "~" H 6050 5450 50  0001 C CNN
	1    6050 5450
	0    1    1    0   
$EndComp
Wire Wire Line
	5550 5350 5550 5650
Wire Wire Line
	6050 5600 6050 5650
Wire Wire Line
	6050 5650 5550 5650
Connection ~ 5550 5650
Wire Wire Line
	5550 5650 5550 5700
Wire Wire Line
	5450 5250 5600 5250
Wire Wire Line
	5900 5250 6050 5250
Wire Wire Line
	6050 5250 6050 5300
$Comp
L power:+24V #PWR?
U 1 1 5BD272C7
P 6550 5050
F 0 "#PWR?" H 6550 4900 50  0001 C CNN
F 1 "+24V" V 6565 5178 50  0000 L CNN
F 2 "" H 6550 5050 50  0001 C CNN
F 3 "" H 6550 5050 50  0001 C CNN
	1    6550 5050
	1    0    0    -1  
$EndComp
Connection ~ 6050 5250
$Comp
L buckconverter:buck-converter U?
U 1 1 5BD279F9
P 7200 5550
F 0 "U?" H 7225 5865 50  0000 C CNN
F 1 "buck-converter" H 7225 5774 50  0000 C CNN
F 2 "mylib:buck-converter" H 7200 5300 50  0001 C CNN
F 3 "" H 7200 5800 50  0001 C CNN
	1    7200 5550
	1    0    0    -1  
$EndComp
Wire Wire Line
	6550 5250 6550 5500
Wire Wire Line
	6550 5500 6850 5500
Wire Wire Line
	6050 5250 6550 5250
Wire Wire Line
	6550 5050 6550 5250
Connection ~ 6550 5250
Wire Wire Line
	6050 5650 6850 5650
Connection ~ 6050 5650
$Comp
L power:+5V #PWR?
U 1 1 5BD2832F
P 7850 5350
F 0 "#PWR?" H 7850 5200 50  0001 C CNN
F 1 "+5V" H 7865 5523 50  0000 C CNN
F 2 "" H 7850 5350 50  0001 C CNN
F 3 "" H 7850 5350 50  0001 C CNN
	1    7850 5350
	1    0    0    -1  
$EndComp
$Comp
L Device:CP C?
U 1 1 5BD28399
P 7850 5700
F 0 "C?" H 7968 5746 50  0000 L CNN
F 1 "10u" H 7968 5655 50  0000 L CNN
F 2 "" H 7888 5550 50  0001 C CNN
F 3 "~" H 7850 5700 50  0001 C CNN
	1    7850 5700
	1    0    0    -1  
$EndComp
Wire Wire Line
	7850 5350 7850 5500
Wire Wire Line
	7600 5500 7850 5500
Connection ~ 7850 5500
Wire Wire Line
	7850 5500 7850 5550
Wire Wire Line
	7600 5650 7600 5950
Wire Wire Line
	7600 5950 7850 5950
Wire Wire Line
	7850 5950 7850 5850
$Comp
L power:GND #PWR?
U 1 1 5BD28E16
P 7850 6050
F 0 "#PWR?" H 7850 5800 50  0001 C CNN
F 1 "GND" H 7855 5877 50  0000 C CNN
F 2 "" H 7850 6050 50  0001 C CNN
F 3 "" H 7850 6050 50  0001 C CNN
	1    7850 6050
	1    0    0    -1  
$EndComp
Wire Wire Line
	7850 6050 7850 5950
Connection ~ 7850 5950
$EndSCHEMATC
