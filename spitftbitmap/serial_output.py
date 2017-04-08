import serial
import time

ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    xonxoff=serial.XOFF,
    rtscts=False,
    dsrdtr=False
)

ser.open()
ser.isOpen()

print("Initializing the device ..")

while(1):
    ser.write(bytes(0xAA))
    print("Write command 20 or 0x41a00000")
    ser.write (bytes(0x00))
    ser.write (bytes(0x00))
    ser.write (bytes(0xa0))
    ser.write (bytes(0x41))
    print("Write command 40 or 0x42200000")
    ser.write (bytes(0x00))
    ser.write (bytes(0x00))
    ser.write (bytes(0x20))
    ser.write (bytes(0x42))

    time.sleep(1)
    
    ser.write(bytes(0xAA))
    print("Write command 20 or 0x42200000")
    ser.write (bytes(0x00))
    ser.write (bytes(0x00))
    ser.write (bytes(0x20))
    ser.write (bytes(0x42))
    print("Write command 40 or 0x41a00000")
    ser.write (bytes(0x00))
    ser.write (bytes(0x00))
    ser.write (bytes(0xa0))
    ser.write (bytes(0x41))    

print('Done')
