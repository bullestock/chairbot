EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
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
L Mechanical:MountingHole H1
U 1 1 5DD9469D
P 2350 1850
F 0 "H1" H 2450 1896 50  0000 L CNN
F 1 "MountingHole" H 2450 1805 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3" H 2350 1850 50  0001 C CNN
F 3 "~" H 2350 1850 50  0001 C CNN
	1    2350 1850
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H2
U 1 1 5DD9472C
P 2850 1850
F 0 "H2" H 2950 1896 50  0000 L CNN
F 1 "MountingHole" H 2950 1805 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3" H 2850 1850 50  0001 C CNN
F 3 "~" H 2850 1850 50  0001 C CNN
	1    2850 1850
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H3
U 1 1 5DD947D4
P 3300 1850
F 0 "H3" H 3400 1896 50  0000 L CNN
F 1 "MountingHole" H 3400 1805 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3" H 3300 1850 50  0001 C CNN
F 3 "~" H 3300 1850 50  0001 C CNN
	1    3300 1850
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H4
U 1 1 5DD94890
P 3750 1850
F 0 "H4" H 3850 1896 50  0000 L CNN
F 1 "MountingHole" H 3850 1805 50  0000 L CNN
F 2 "MountingHole:MountingHole_3.2mm_M3" H 3750 1850 50  0001 C CNN
F 3 "~" H 3750 1850 50  0001 C CNN
	1    3750 1850
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J2
U 1 1 5DD94AA1
P 2500 3000
F 0 "J2" V 2464 2812 50  0000 R CNN
F 1 "Conn_01x02" V 2373 2812 50  0000 R CNN
F 2 "Connector_JST:JST_XH_B02B-XH-A_1x02_P2.50mm_Vertical" H 2500 3000 50  0001 C CNN
F 3 "~" H 2500 3000 50  0001 C CNN
	1    2500 3000
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J3
U 1 1 5DD953CC
P 3100 3000
F 0 "J3" V 3064 2812 50  0000 R CNN
F 1 "Conn_01x02" V 2973 2812 50  0000 R CNN
F 2 "Connector_JST:JST_XH_B02B-XH-A_1x02_P2.50mm_Vertical" H 3100 3000 50  0001 C CNN
F 3 "~" H 3100 3000 50  0001 C CNN
	1    3100 3000
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J4
U 1 1 5DD955B0
P 3700 3000
F 0 "J4" V 3664 2812 50  0000 R CNN
F 1 "Conn_01x02" V 3573 2812 50  0000 R CNN
F 2 "Connector_JST:JST_XH_B02B-XH-A_1x02_P2.50mm_Vertical" H 3700 3000 50  0001 C CNN
F 3 "~" H 3700 3000 50  0001 C CNN
	1    3700 3000
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J5
U 1 1 5DD958E4
P 4300 3000
F 0 "J5" V 4264 2812 50  0000 R CNN
F 1 "Conn_01x02" V 4173 2812 50  0000 R CNN
F 2 "Connector_JST:JST_XH_B02B-XH-A_1x02_P2.50mm_Vertical" H 4300 3000 50  0001 C CNN
F 3 "~" H 4300 3000 50  0001 C CNN
	1    4300 3000
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J6
U 1 1 5DD95B10
P 4900 3000
F 0 "J6" V 4864 2812 50  0000 R CNN
F 1 "Conn_01x02" V 4773 2812 50  0000 R CNN
F 2 "Connector_JST:JST_XH_B02B-XH-A_1x02_P2.50mm_Vertical" H 4900 3000 50  0001 C CNN
F 3 "~" H 4900 3000 50  0001 C CNN
	1    4900 3000
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J1
U 1 1 5DD9717C
P 2000 3500
F 0 "J1" H 1918 3175 50  0000 C CNN
F 1 "Conn_01x02" H 1918 3266 50  0000 C CNN
F 2 "Connector_Phoenix_MC_HighVoltage:PhoenixContact_MCV_1,5_2-G-5.08_1x02_P5.08mm_Vertical" H 2000 3500 50  0001 C CNN
F 3 "~" H 2000 3500 50  0001 C CNN
	1    2000 3500
	-1   0    0    1   
$EndComp
Wire Wire Line
	2200 3500 2500 3500
Wire Wire Line
	4900 3500 4900 3200
Wire Wire Line
	4300 3200 4300 3500
Connection ~ 4300 3500
Wire Wire Line
	4300 3500 4900 3500
Wire Wire Line
	3700 3200 3700 3500
Connection ~ 3700 3500
Wire Wire Line
	3700 3500 4300 3500
Wire Wire Line
	3100 3200 3100 3500
Connection ~ 3100 3500
Wire Wire Line
	3100 3500 3700 3500
Wire Wire Line
	2500 3200 2500 3500
Connection ~ 2500 3500
Wire Wire Line
	2500 3500 3100 3500
Wire Wire Line
	2200 3400 2600 3400
Wire Wire Line
	5000 3400 5000 3200
Wire Wire Line
	4400 3200 4400 3400
Connection ~ 4400 3400
Wire Wire Line
	4400 3400 5000 3400
Wire Wire Line
	3800 3200 3800 3400
Connection ~ 3800 3400
Wire Wire Line
	3800 3400 4400 3400
Wire Wire Line
	3200 3200 3200 3400
Connection ~ 3200 3400
Wire Wire Line
	3200 3400 3800 3400
Wire Wire Line
	2600 3200 2600 3400
Connection ~ 2600 3400
Wire Wire Line
	2600 3400 3200 3400
$EndSCHEMATC
