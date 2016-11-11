# Import standard Python time library
from __future__ import print_function
import time
import sys
from struct import pack

# Import GPIO and FT232H modules.
import Adafruit_GPIO as GPIO
import Adafruit_GPIO.FT232H as FT232H

def int2bin(i):
    if i == 0: return "00000000"
    s = ''
    while i:
        if i & 1 == 1:
            s = "1" + s
        else:
            s = "0" + s
        i /= 2
    return s

def printHex(data):
	counter = 0
	s = ''
	for a in data:
		s +='{:02X} '.format(a)
		counter +=1
		if counter == 8:
			counter = 0
			print(s+'\n')
			s=''

def printBinary(data):
	counter = 0
	s=""
	for a in data:
		s +=str(counter)+": "+int2bin(a)+"\n"
		counter +=1
		print(s + '\n')


def readList(i2c, register, length):
	"""Read a length number of bytes from the specified register.  Results
	will be returned as a bytearray."""
	if length <= 0:
		raise ValueError("Length must be at least 1 byte.")
	i2c._idle()
	i2c._transaction_start()
	i2c._i2c_start()
	i2c._i2c_write_bytes([i2c._address_byte(False), register])
	i2c._i2c_stop()
	i2c._i2c_idle()
	i2c._i2c_start()
	i2c._i2c_write_bytes([i2c._address_byte(True)])
	i2c._i2c_read_bytes(length)
	i2c._i2c_stop()
	response = i2c._transaction_end()
	i2c._verify_acks(response[:-length])
	return response[-length:]

def hostLoad(fileName,i2c):
	print("Sending "+fileName)
	fileData = []
	f = open(fileName, "rb")
	counter = 0
	try:
		fileData.extend([0, 0, 0])
		byte = bytearray(f.read(1))
		while byte != "":
			fileData.extend(byte)
			counter+= 1
			if counter == 40:
				i2c.writeList(0x04, fileData)
				time.sleep(0.01)
				counter = 0
				fileData = []
				fileData.extend([0, 0, 0])
			byte = bytearray(f.read(1))
		i2c.writeList(0x04, fileData)
	finally:
		f.close()

def shortToBytes(n):
	b = bytearray([0, 0])  # init
	b[1] = n & 0xFF
	n >>= 8
	b[0] = n & 0xFF
	return b

# Temporarily disable the built-in FTDI serial driver on Mac & Linux platforms.
FT232H.use_FT232H()

# Create an FT232H object that grabs the first available FT232H device found.
ft232h = FT232H.FT232H()

# Configure digital inputs and outputs using the setup function.
# Note that pin numbers 0 to 15 map to pins D0 to D7 then C0 to C7 on the board.
ft232h.setup(9, GPIO.OUT)  # Make pin C1 a digital output.

# Create a SPI interface from the FT232H using pin 8 (C0) as chip select.
# Use a clock speed of 3mhz, SPI mode 0, and most significant bit first.

#spi = FT232H.SPI(ft232h, cs=8, max_speed_hz=5000000, mode=0, bitorder=FT232H.MSBFIRST)

print('Press Ctrl-C to quit.')
i2c = FT232H.I2CDevice(ft232h, 0x64)

def bootChip():

	ft232h.output(9, GPIO.LOW)

	time.sleep(0.1)

	ft232h.output(9, GPIO.HIGH)
	time.sleep(0.1)


	data = [
	0b10000000, # ARG1 Enable interrupt
	0b00010111, # ARG2 CLK_MODE=0x1 TR_SIZE=0x7
	0x48, # ARG3 IBIAS=0x48
	0x00, # ARG4 XTAL
	0xF8, # ARG5 XTAL // F8
	0x24, # ARG6 XTAL
	0x01, # ARG7 XTAL 19.2MHz
	0x1F, # ARG8 CTUN
	0b00010000, # ARG9
	0x00, # ARG10
	0x00, # ARG11
	0x00, # ARG12
	0x00, # ARG13 IBIAS_RUN
	0x00, # ARG14
	0x00, # ARG15
	]

	list=readList(i2c,0,4)
	printBinary(list)

	i2c.writeList(0x01,data)
	time.sleep(0.05)

	list=readList(i2c,0,4)
	printBinary(list)

	i2c.write8(0x06,0)
	time.sleep(0.05)

	list=readList(i2c,0,4)
	printBinary(list)

	hostLoad("boot.bin",i2c)

	list = readList(i2c, 0, 4)
	while (list[0] == 0):
		time.sleep(0.05)
		list = readList(i2c, 0, 4)

	i2c.write8(0x06,0)
	time.sleep(0.05)

	list=readList(i2c,0,4)
	printBinary(list)
	print("Bootloader Finished")

def bootFM(arguments):
	bootChip()
	i2c.writeList(0x05,
				  [
					  0x00,
					  0x00,
					  0x00,
					  0x00,
					  0x80,
					  0x00,
					  0x00,
					  0x00,
					  0x00,
					  0x00
				  ])

	list=readList(i2c,0,4)
	while(list[0] == 0):
		list=readList(i2c,0,4)

	i2c.write8(0x07,0)
	list=readList(i2c,0,4)
	while(list[0] == 0):
		time.sleep(0.05)
		list=readList(i2c,0,4)

	# hostLoad("fmhd.bin",i2c)
	print("Boot Finished")


def getSysInfo(arguments):
	i2c.write8(0x09, 0)
	list = readList(i2c, 0, 6)
	printBinary(list)

def getPartInfo(arguments):
	i2c.write8(0x09, 0)
	list = readList(i2c, 0, 6)
	printBinary(list)


def TuneRadio(arguments):
	khz =float(arguments[0])
	if khz <1000:
		khz=khz*100
	else:
		khz = khz/10
	khz = int(khz)
	khz = shortToBytes(khz)
	antcap = 0
	if len(arguments) > 1:
		antcap = int(arguments[1])
	antcap = shortToBytes(antcap)
	i2c.writeList(0x30,[
		0b00001000,
		khz[1] & 0xFF,
		khz[0] & 0xFF,
		antcap[1]  & 0xFF,
		antcap[0]  & 0xFF,
		0
	])
	list = readList(i2c, 0, 4)
	while (list[0] == 0):
		list = readList(i2c, 0, 4)
	print("Tune Complete")

def SeekRadio(arguments):
	seek = 0
	if len(arguments) > 0:
		if arguments[0] == "up":
			seek = 1
	antcap = 0
	if len(arguments) > 1:
		antcap = int(arguments[1])
	value = 0b0000001
	value = value & (seek <<1)
	antcap = shortToBytes(antcap)
	i2c.writeList(0x31, [
		0b00001000,
		value,
		0x00,
		antcap[1] & 0xFF,
		antcap[0] & 0xFF,
		0
	])
	list = readList(i2c, 0, 4)
	while (list[0] == 0):
		list = readList(i2c, 0, 4)
	print("Tune Complete")

def StationInfo(arguments):
	i2c.write8(0x34,0b00000100)
	list = readList(i2c, 0, 20)
	while (list[0] == 0):
		list = readList(i2c, 0, 20)
	printBinary(list)

def SignalInfo(arguments):
	i2c.write8(0x32, 0b00001000)
	list = readList(i2c, 0, 22)
	while (list[0] == 0):
		list = readList(i2c, 0, 22)
	printBinary(list)


def radioSwitch(argument):
	statements = argument.split()
	argument = statements.pop(0).lower()
	switcher = {
		"bootfm": bootFM,
		"sysinfo": getSysInfo,
		"tune": TuneRadio,
		"seek": SeekRadio,
		"stationinfo": StationInfo,
		"signalinfo": SignalInfo,
		"exit": lambda :sys.exit(),
		"end" : lambda :sys.exit(),
	}
	print(argument)
	# Get the function from switcher dictionary
	func = switcher.get(argument,lambda x: "nothing")
	# Execute the function
	func(statements)

s = raw_input("Enter a command:\n")
while True:
	radioSwitch(s)
	s = raw_input("Enter a command:\n")