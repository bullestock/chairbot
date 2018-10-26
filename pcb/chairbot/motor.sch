EESchema Schematic File Version 4
LIBS:chairbot-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 2
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
L chairbot:Motor_driver U7
U 1 1 5BC5FCEE
P 7450 3700
F 0 "U7" H 7778 3746 50  0000 L CNN
F 1 "Motor_driver" H 7778 3655 50  0000 L CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x04_P2.54mm_Horizontal" H 7350 3850 50  0001 C CNN
F 3 "" H 7350 3850 50  0001 C CNN
	1    7450 3700
	1    0    0    -1  
$EndComp
$Comp
L Transistor_FET:BS107 Q1
U 1 1 5BC5FCF5
P 6950 5400
F 0 "Q1" H 7156 5446 50  0000 L CNN
F 1 "ZXMN4A06G" H 7156 5355 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-223" H 7150 5325 50  0001 L CIN
F 3 "http://www.onsemi.com/pub_link/Collateral/BS107-D.PDF" H 6950 5400 50  0001 L CNN
	1    6950 5400
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J3
U 1 1 5BC5FCFC
P 7400 5000
F 0 "J3" H 7480 4992 50  0000 L CNN
F 1 "Conn_01x02" H 7480 4901 50  0000 L CNN
F 2 "Connector_JST:JST_XH_B02B-XH-A_1x02_P2.50mm_Vertical" H 7400 5000 50  0001 C CNN
F 3 "~" H 7400 5000 50  0001 C CNN
	1    7400 5000
	1    0    0    -1  
$EndComp
Text Notes 7400 4900 0    50   ~ 0
Brake
Wire Wire Line
	7050 4850 7050 5000
Wire Wire Line
	7050 5000 7200 5000
Wire Wire Line
	7200 5100 7050 5100
Wire Wire Line
	7050 5100 7050 5200
Wire Wire Line
	7050 5700 7050 5600
$Comp
L power:+24V #PWR0106
U 1 1 5BC5FD09
P 7050 4850
F 0 "#PWR0106" H 7050 4700 50  0001 C CNN
F 1 "+24V" H 7065 5023 50  0000 C CNN
F 2 "" H 7050 4850 50  0001 C CNN
F 3 "" H 7050 4850 50  0001 C CNN
	1    7050 4850
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0107
U 1 1 5BC5FD0F
P 7050 5700
F 0 "#PWR0107" H 7050 5450 50  0001 C CNN
F 1 "GND" H 7055 5527 50  0000 C CNN
F 2 "" H 7050 5700 50  0001 C CNN
F 3 "" H 7050 5700 50  0001 C CNN
	1    7050 5700
	1    0    0    -1  
$EndComp
$Comp
L Device:R R1
U 1 1 5BC5FD15
P 6550 5400
F 0 "R1" V 6500 5250 50  0000 C CNN
F 1 "1K" V 6434 5400 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 6480 5400 50  0001 C CNN
F 3 "~" H 6550 5400 50  0001 C CNN
	1    6550 5400
	0    1    1    0   
$EndComp
$Comp
L 74xx_IEEE:7408 U2
U 1 1 5BC5FD1D
P 5900 2250
F 0 "U2" H 5900 2666 50  0000 C CNN
F 1 "74HCT08" H 5900 2575 50  0000 C CNN
F 2 "Package_SO:TSSOP-14_4.4x5mm_P0.65mm" H 5900 2250 50  0001 C CNN
F 3 "" H 5900 2250 50  0001 C CNN
	1    5900 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	6750 1850 6750 1950
Wire Wire Line
	6750 1950 6950 1950
Wire Wire Line
	6750 1850 6950 1850
Wire Wire Line
	6750 3650 6750 3750
Wire Wire Line
	6750 3750 6950 3750
Wire Wire Line
	6750 3650 6950 3650
$Comp
L power:GND #PWR0108
U 1 1 5BC5FD34
P 5900 2650
F 0 "#PWR0108" H 5900 2400 50  0001 C CNN
F 1 "GND" H 5905 2477 50  0000 C CNN
F 2 "" H 5900 2650 50  0001 C CNN
F 3 "" H 5900 2650 50  0001 C CNN
	1    5900 2650
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0109
U 1 1 5BC5FD3A
P 7650 2550
F 0 "#PWR0109" H 7650 2300 50  0001 C CNN
F 1 "GND" H 7655 2377 50  0000 C CNN
F 2 "" H 7650 2550 50  0001 C CNN
F 3 "" H 7650 2550 50  0001 C CNN
	1    7650 2550
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0110
U 1 1 5BC5FD40
P 5900 1700
F 0 "#PWR0110" H 5900 1550 50  0001 C CNN
F 1 "+5V" H 5915 1873 50  0000 C CNN
F 2 "" H 5900 1700 50  0001 C CNN
F 3 "" H 5900 1700 50  0001 C CNN
	1    5900 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:C C5
U 1 1 5BC5FD46
P 6150 1800
F 0 "C5" V 5898 1800 50  0000 C CNN
F 1 "100n" V 5989 1800 50  0000 C CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 6188 1650 50  0001 C CNN
F 3 "~" H 6150 1800 50  0001 C CNN
	1    6150 1800
	0    1    1    0   
$EndComp
Wire Wire Line
	5900 1700 5900 1800
Wire Wire Line
	5900 1800 6000 1800
Connection ~ 5900 1800
Wire Wire Line
	5900 1800 5900 2050
$Comp
L power:GND #PWR0111
U 1 1 5BC5FD51
P 6400 1900
F 0 "#PWR0111" H 6400 1650 50  0001 C CNN
F 1 "GND" H 6405 1727 50  0000 C CNN
F 2 "" H 6400 1900 50  0001 C CNN
F 3 "" H 6400 1900 50  0001 C CNN
	1    6400 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	6300 1800 6400 1800
Wire Wire Line
	6400 1800 6400 1900
$Comp
L Device:C C6
U 1 1 5BC5FD59
P 8000 1400
F 0 "C6" H 8115 1446 50  0000 L CNN
F 1 "100n" H 8115 1355 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 8038 1250 50  0001 C CNN
F 3 "~" H 8000 1400 50  0001 C CNN
	1    8000 1400
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0112
U 1 1 5BC5FD60
P 8000 1650
F 0 "#PWR0112" H 8000 1400 50  0001 C CNN
F 1 "GND" H 8005 1477 50  0000 C CNN
F 2 "" H 8000 1650 50  0001 C CNN
F 3 "" H 8000 1650 50  0001 C CNN
	1    8000 1650
	1    0    0    -1  
$EndComp
$Comp
L Device:C C7
U 1 1 5BC5FD66
P 8000 3200
F 0 "C7" H 8115 3246 50  0000 L CNN
F 1 "100n" H 8115 3155 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 8038 3050 50  0001 C CNN
F 3 "~" H 8000 3200 50  0001 C CNN
	1    8000 3200
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0113
U 1 1 5BC5FD6D
P 8000 3450
F 0 "#PWR0113" H 8000 3200 50  0001 C CNN
F 1 "GND" H 8005 3277 50  0000 C CNN
F 2 "" H 8000 3450 50  0001 C CNN
F 3 "" H 8000 3450 50  0001 C CNN
	1    8000 3450
	1    0    0    -1  
$EndComp
Wire Wire Line
	8000 3450 8000 3350
$Comp
L power:+5V #PWR0114
U 1 1 5BC5FD74
P 7250 3000
F 0 "#PWR0114" H 7250 2850 50  0001 C CNN
F 1 "+5V" V 7265 3128 50  0000 L CNN
F 2 "" H 7250 3000 50  0001 C CNN
F 3 "" H 7250 3000 50  0001 C CNN
	1    7250 3000
	0    -1   -1   0   
$EndComp
Wire Wire Line
	7250 3000 7450 3000
Wire Wire Line
	8000 3000 8000 3050
Wire Wire Line
	7450 3000 7450 3050
Connection ~ 7450 3000
Wire Wire Line
	7450 3000 8000 3000
$Comp
L power:+5V #PWR0115
U 1 1 5BC5FD7F
P 7250 1200
F 0 "#PWR0115" H 7250 1050 50  0001 C CNN
F 1 "+5V" V 7265 1328 50  0000 L CNN
F 2 "" H 7250 1200 50  0001 C CNN
F 3 "" H 7250 1200 50  0001 C CNN
	1    7250 1200
	0    -1   -1   0   
$EndComp
Wire Wire Line
	7250 1200 7450 1200
Wire Wire Line
	8000 1200 8000 1250
Wire Wire Line
	7450 1200 7450 1250
Connection ~ 7450 1200
Wire Wire Line
	7450 1200 8000 1200
Wire Wire Line
	8000 1550 8000 1650
Wire Wire Line
	7450 2550 7650 2550
$Comp
L power:GND #PWR0116
U 1 1 5BC5FD8C
P 7650 4350
F 0 "#PWR0116" H 7650 4100 50  0001 C CNN
F 1 "GND" H 7655 4177 50  0000 C CNN
F 2 "" H 7650 4350 50  0001 C CNN
F 3 "" H 7650 4350 50  0001 C CNN
	1    7650 4350
	1    0    0    -1  
$EndComp
Wire Wire Line
	7650 4350 7450 4350
$Comp
L Device:D_Schottky D2
U 1 1 5BC5FD93
P 6800 5100
F 0 "D2" H 6800 5316 50  0000 C CNN
F 1 "SS34" H 6800 5225 50  0000 C CNN
F 2 "Diode_SMD:D_SMC" H 6800 5100 50  0001 C CNN
F 3 "~" H 6800 5100 50  0001 C CNN
	1    6800 5100
	1    0    0    -1  
$EndComp
Wire Wire Line
	7050 5000 6600 5000
Wire Wire Line
	6600 5000 6600 5100
Wire Wire Line
	6600 5100 6650 5100
Connection ~ 7050 5000
Wire Wire Line
	6950 5100 7050 5100
Connection ~ 7050 5100
$Comp
L chairbot:Motor_driver U6
U 1 1 5BC5FDA0
P 7450 1900
F 0 "U6" H 7778 1946 50  0000 L CNN
F 1 "Motor_driver" H 7778 1855 50  0000 L CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x04_P2.54mm_Horizontal" H 7350 2050 50  0001 C CNN
F 3 "" H 7350 2050 50  0001 C CNN
	1    7450 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	6750 1950 6750 2250
Wire Wire Line
	6750 2250 6450 2250
Connection ~ 6750 1950
Wire Wire Line
	4750 1300 6750 1300
Wire Wire Line
	6750 1300 6750 1550
Wire Wire Line
	6750 1550 6950 1550
Wire Wire Line
	6950 1650 6650 1650
Wire Wire Line
	6650 1650 6650 1400
Wire Wire Line
	6650 1400 4750 1400
Wire Wire Line
	6550 2950 6550 3350
Wire Wire Line
	6550 3350 6950 3350
Wire Wire Line
	6950 3450 6450 3450
Wire Wire Line
	6450 3450 6450 3100
Text GLabel 4050 5400 0    50   Input ~ 0
Brake
Wire Wire Line
	4050 5400 5350 5400
Wire Wire Line
	5900 2500 5900 2650
Text GLabel 6500 2500 0    50   Input ~ 0
IS_A_L
Text GLabel 6500 2700 0    50   Input ~ 0
IS_A_R
Wire Wire Line
	6950 2250 6950 2700
Wire Wire Line
	6950 2700 6500 2700
Wire Wire Line
	6950 2150 6850 2150
Wire Wire Line
	6850 2150 6850 2500
Wire Wire Line
	6850 2500 6500 2500
Text GLabel 6650 3950 0    50   Input ~ 0
IS_B_L
Text GLabel 6650 4050 0    50   Input ~ 0
IS_B_R
Wire Wire Line
	6650 4050 6950 4050
Wire Wire Line
	6950 3950 6650 3950
Text GLabel 4750 1300 0    50   Input ~ 0
PWM_A_L
Text GLabel 4750 1400 0    50   Input ~ 0
PWM_A_R
Text GLabel 4750 2950 0    50   Input ~ 0
PWM_B_L
Text GLabel 4750 3100 0    50   Input ~ 0
PWM_B_R
Wire Wire Line
	4750 2950 6550 2950
Wire Wire Line
	4750 3100 6450 3100
Text GLabel 4750 2150 0    50   Input ~ 0
EN
Wire Wire Line
	4750 2150 5350 2150
$Comp
L Device:R R2
U 1 1 5BD21085
P 6550 5600
F 0 "R2" V 6500 5450 50  0000 C CNN
F 1 "10K" V 6700 5600 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 6480 5600 50  0001 C CNN
F 3 "~" H 6550 5600 50  0001 C CNN
	1    6550 5600
	0    1    1    0   
$EndComp
Wire Wire Line
	6700 5400 6750 5400
Wire Wire Line
	6700 5600 7050 5600
Connection ~ 7050 5600
Wire Wire Line
	6400 5600 6300 5600
Wire Wire Line
	6300 5600 6300 5400
Wire Wire Line
	6300 5400 6400 5400
Wire Wire Line
	6300 5400 5350 5400
Connection ~ 6300 5400
Connection ~ 5350 5400
Wire Wire Line
	5350 2350 5350 5400
Wire Wire Line
	6750 2250 6750 3650
Connection ~ 6750 2250
Connection ~ 6750 3650
$EndSCHEMATC
