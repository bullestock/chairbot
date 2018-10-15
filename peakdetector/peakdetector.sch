EESchema Schematic File Version 4
LIBS:peakdetector-cache
EELAYER 26 0
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
L device:D_Schottky D1
U 1 1 5B76EC56
P 3900 2600
F 0 "D1" V 3854 2679 50  0000 L CNN
F 1 "BAT54" V 3945 2679 50  0000 L CNN
F 2 "Diodes_SMD:D_SOT-23_ANK" H 3900 2600 50  0001 C CNN
F 3 "" H 3900 2600 50  0001 C CNN
	1    3900 2600
	0    1    1    0   
$EndComp
$Comp
L device:D_Schottky D3
U 1 1 5B76ECC1
P 4150 2350
F 0 "D3" H 4150 2134 50  0000 C CNN
F 1 "BAT54" H 4150 2225 50  0000 C CNN
F 2 "Diodes_SMD:D_SOT-23_ANK" H 4150 2350 50  0001 C CNN
F 3 "" H 4150 2350 50  0001 C CNN
	1    4150 2350
	-1   0    0    1   
$EndComp
$Comp
L device:C C1
U 1 1 5B76ED30
P 4450 2600
F 0 "C1" H 4565 2646 50  0000 L CNN
F 1 "220n" H 4565 2555 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 4488 2450 50  0001 C CNN
F 3 "" H 4450 2600 50  0001 C CNN
	1    4450 2600
	1    0    0    -1  
$EndComp
$Comp
L device:R R1
U 1 1 5B76ED9A
P 4750 2600
F 0 "R1" H 4820 2646 50  0000 L CNN
F 1 "100K" H 4820 2555 50  0000 L CNN
F 2 "Resistors_SMD:R_0603" V 4680 2600 50  0001 C CNN
F 3 "" H 4750 2600 50  0001 C CNN
	1    4750 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 2450 3900 2350
Wire Wire Line
	3150 1800 3150 2250
Wire Wire Line
	3150 2250 3200 2250
Connection ~ 3900 2350
Wire Wire Line
	3900 2350 4000 2350
Wire Wire Line
	4450 1800 4450 2350
Wire Wire Line
	3150 1800 4450 1800
Wire Wire Line
	4300 2350 4450 2350
Wire Wire Line
	4750 2350 4750 2450
Connection ~ 4450 2350
Wire Wire Line
	4450 2350 4450 2450
Wire Wire Line
	4450 2350 4750 2350
Wire Wire Line
	4750 2350 4900 2350
Connection ~ 4750 2350
Wire Wire Line
	4900 2150 4750 2150
Wire Wire Line
	4750 2150 4750 1800
Wire Wire Line
	4750 1800 5600 1800
Wire Wire Line
	5600 1800 5600 2250
Wire Wire Line
	5600 2250 5500 2250
$Comp
L power:GND #PWR03
U 1 1 5B76F32E
P 3400 2800
F 0 "#PWR03" H 3400 2550 50  0001 C CNN
F 1 "GND" H 3550 2750 50  0000 C CNN
F 2 "" H 3400 2800 50  0001 C CNN
F 3 "" H 3400 2800 50  0001 C CNN
	1    3400 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 2800 3400 2750
Wire Wire Line
	3400 2750 3900 2750
Connection ~ 3400 2750
Connection ~ 3900 2750
Wire Wire Line
	3900 2750 4450 2750
Connection ~ 4450 2750
Wire Wire Line
	4450 2750 4750 2750
$Comp
L device:C C3
U 1 1 5B76F5D9
P 3300 5750
F 0 "C3" H 3415 5796 50  0000 L CNN
F 1 "220n" H 3415 5705 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 3338 5600 50  0001 C CNN
F 3 "" H 3300 5750 50  0001 C CNN
	1    3300 5750
	1    0    0    -1  
$EndComp
$Comp
L conn:Conn_01x07_Male J1
U 1 1 5B76FB68
P 2600 3050
F 0 "J1" H 2706 3528 50  0000 C CNN
F 1 "Conn_01x07_Male" H 2706 3437 50  0001 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x07_Pitch2.54mm" H 2600 3050 50  0001 C CNN
F 3 "~" H 2600 3050 50  0001 C CNN
	1    2600 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	2800 2750 3400 2750
$Comp
L power:+5V #PWR01
U 1 1 5B76FE87
P 2950 3350
F 0 "#PWR01" H 2950 3200 50  0001 C CNN
F 1 "+5V" V 2965 3478 50  0000 L CNN
F 2 "" H 2950 3350 50  0001 C CNN
F 3 "" H 2950 3350 50  0001 C CNN
	1    2950 3350
	0    1    1    0   
$EndComp
Wire Wire Line
	2950 3350 2800 3350
NoConn ~ 2800 3250
Wire Wire Line
	2800 2850 3150 2850
Wire Wire Line
	3150 2850 3150 2450
Wire Wire Line
	3150 2450 3200 2450
Connection ~ 5600 2250
$Comp
L device:D_Schottky D4
U 1 1 5B771542
P 4150 4000
F 0 "D4" H 4150 3784 50  0000 C CNN
F 1 "BAT54" H 4150 3875 50  0000 C CNN
F 2 "Diodes_SMD:D_SOT-23_ANK" H 4150 4000 50  0001 C CNN
F 3 "" H 4150 4000 50  0001 C CNN
	1    4150 4000
	-1   0    0    1   
$EndComp
$Comp
L device:D_Schottky D2
U 1 1 5B7715B3
P 3900 4250
F 0 "D2" V 3854 4329 50  0000 L CNN
F 1 "BAT54" V 3945 4329 50  0000 L CNN
F 2 "Diodes_SMD:D_SOT-23_ANK" H 3900 4250 50  0001 C CNN
F 3 "" H 3900 4250 50  0001 C CNN
	1    3900 4250
	0    1    1    0   
$EndComp
$Comp
L device:C C2
U 1 1 5B7719CF
P 4450 4250
F 0 "C2" H 4565 4296 50  0000 L CNN
F 1 "220n" H 4500 4150 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 4488 4100 50  0001 C CNN
F 3 "" H 4450 4250 50  0001 C CNN
	1    4450 4250
	1    0    0    -1  
$EndComp
$Comp
L device:R R2
U 1 1 5B771A39
P 4750 4250
F 0 "R2" H 4820 4296 50  0000 L CNN
F 1 "100K" H 4820 4205 50  0000 L CNN
F 2 "Resistors_SMD:R_0603" V 4680 4250 50  0001 C CNN
F 3 "" H 4750 4250 50  0001 C CNN
	1    4750 4250
	1    0    0    -1  
$EndComp
Wire Wire Line
	4300 4000 4450 4000
Wire Wire Line
	4750 4000 4750 4100
Connection ~ 4750 4000
Wire Wire Line
	4750 4000 5000 4000
Wire Wire Line
	4450 4100 4450 4000
Connection ~ 4450 4000
Wire Wire Line
	4450 4000 4750 4000
Wire Wire Line
	3800 4000 3900 4000
Wire Wire Line
	3900 4000 3900 4100
Connection ~ 3900 4000
Wire Wire Line
	3900 4000 4000 4000
$Comp
L power:GND #PWR04
U 1 1 5B77398A
P 3400 4600
F 0 "#PWR04" H 3400 4350 50  0001 C CNN
F 1 "GND" H 3405 4427 50  0000 C CNN
F 2 "" H 3400 4600 50  0001 C CNN
F 3 "" H 3400 4600 50  0001 C CNN
	1    3400 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 4500 3900 4500
Wire Wire Line
	4750 4500 4750 4400
Wire Wire Line
	3400 4500 3400 4600
Wire Wire Line
	4450 4400 4450 4500
Connection ~ 4450 4500
Wire Wire Line
	4450 4500 4750 4500
Wire Wire Line
	3900 4400 3900 4500
Connection ~ 3900 4500
Wire Wire Line
	3900 4500 4450 4500
Wire Wire Line
	3200 3900 3150 3900
Wire Wire Line
	3150 3900 3150 3550
Wire Wire Line
	3150 3550 4450 3550
Wire Wire Line
	4450 3550 4450 4000
Wire Wire Line
	5000 3800 4850 3800
Wire Wire Line
	4850 3800 4850 3550
Wire Wire Line
	4850 3550 5700 3550
Wire Wire Line
	5700 3550 5700 3900
Wire Wire Line
	5700 3900 5600 3900
Wire Wire Line
	2800 3050 2900 3050
Wire Wire Line
	2900 3050 2900 4100
Wire Wire Line
	2900 4100 3200 4100
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5B77A429
P 4950 5500
F 0 "#FLG0101" H 4950 5575 50  0001 C CNN
F 1 "PWR_FLAG" V 4950 5628 50  0000 L CNN
F 2 "" H 4950 5500 50  0001 C CNN
F 3 "" H 4950 5500 50  0001 C CNN
	1    4950 5500
	0    -1   -1   0   
$EndComp
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 5B77A49F
P 4950 5750
F 0 "#FLG0102" H 4950 5825 50  0001 C CNN
F 1 "PWR_FLAG" V 4950 5878 50  0000 L CNN
F 2 "" H 4950 5750 50  0001 C CNN
F 3 "" H 4950 5750 50  0001 C CNN
	1    4950 5750
	0    -1   -1   0   
$EndComp
$Comp
L power:+5V #PWR0101
U 1 1 5B77A531
P 5200 5500
F 0 "#PWR0101" H 5200 5350 50  0001 C CNN
F 1 "+5V" V 5215 5628 50  0000 L CNN
F 2 "" H 5200 5500 50  0001 C CNN
F 3 "" H 5200 5500 50  0001 C CNN
	1    5200 5500
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 5B77A5B5
P 5200 5800
F 0 "#PWR0102" H 5200 5550 50  0001 C CNN
F 1 "GND" H 5205 5627 50  0000 C CNN
F 2 "" H 5200 5800 50  0001 C CNN
F 3 "" H 5200 5800 50  0001 C CNN
	1    5200 5800
	1    0    0    -1  
$EndComp
Wire Wire Line
	4950 5750 5200 5750
Wire Wire Line
	5200 5750 5200 5800
Wire Wire Line
	5200 5500 4950 5500
$Comp
L Amplifier_Operational:TL074 U1
U 1 1 5B77019C
P 3500 2350
F 0 "U1" H 3450 2350 50  0000 C CNN
F 1 "LM324" H 3500 2626 50  0000 C CNN
F 2 "peakdetector:SOP14" H 3450 2450 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 3550 2550 50  0001 C CNN
	1    3500 2350
	1    0    0    1   
$EndComp
$Comp
L Amplifier_Operational:TL074 U1
U 2 1 5B77027F
P 5200 2250
F 0 "U1" H 5150 2250 50  0000 C CNN
F 1 "LM324" H 5200 2526 50  0000 C CNN
F 2 "peakdetector:SOP14" H 5150 2350 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 5250 2450 50  0001 C CNN
	2    5200 2250
	1    0    0    1   
$EndComp
$Comp
L Amplifier_Operational:TL074 U1
U 3 1 5B7702DE
P 3500 4000
F 0 "U1" H 3450 4000 50  0000 C CNN
F 1 "LM324" H 3500 4276 50  0000 C CNN
F 2 "peakdetector:SOP14" H 3450 4100 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 3550 4200 50  0001 C CNN
	3    3500 4000
	1    0    0    1   
$EndComp
$Comp
L Amplifier_Operational:TL074 U1
U 4 1 5B77035E
P 5300 3900
F 0 "U1" H 5250 3900 50  0000 C CNN
F 1 "LM324" H 5300 4176 50  0000 C CNN
F 2 "peakdetector:SOP14" H 5250 4000 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 5350 4100 50  0001 C CNN
	4    5300 3900
	1    0    0    1   
$EndComp
$Comp
L Amplifier_Operational:TL074 U1
U 5 1 5B7703E9
P 2950 5750
F 0 "U1" H 2908 5796 50  0000 L CNN
F 1 "LM324" H 2908 5705 50  0000 L CNN
F 2 "peakdetector:SOP14" H 2900 5850 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 3000 5950 50  0001 C CNN
	5    2950 5750
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR02
U 1 1 5B772300
P 2850 5300
F 0 "#PWR02" H 2850 5150 50  0001 C CNN
F 1 "+5V" H 2865 5473 50  0000 C CNN
F 2 "" H 2850 5300 50  0001 C CNN
F 3 "" H 2850 5300 50  0001 C CNN
	1    2850 5300
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR06
U 1 1 5B772379
P 2850 6200
F 0 "#PWR06" H 2850 5950 50  0001 C CNN
F 1 "GND" H 2855 6027 50  0000 C CNN
F 2 "" H 2850 6200 50  0001 C CNN
F 3 "" H 2850 6200 50  0001 C CNN
	1    2850 6200
	1    0    0    -1  
$EndComp
Wire Wire Line
	2850 6200 2850 6100
Wire Wire Line
	2850 5300 2850 5400
Wire Wire Line
	2850 5400 3300 5400
Wire Wire Line
	3300 5400 3300 5600
Connection ~ 2850 5400
Wire Wire Line
	2850 5400 2850 5450
Wire Wire Line
	3300 5900 3300 6100
Wire Wire Line
	3300 6100 2850 6100
Connection ~ 2850 6100
Wire Wire Line
	2850 6100 2850 6050
Wire Wire Line
	5600 2950 5600 2250
Wire Wire Line
	3800 2350 3900 2350
Wire Wire Line
	2800 2950 5600 2950
Wire Wire Line
	5700 3150 5700 3550
Wire Wire Line
	2800 3150 5700 3150
Connection ~ 5700 3550
$EndSCHEMATC
