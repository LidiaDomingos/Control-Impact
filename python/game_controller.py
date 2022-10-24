import serial
import argparse
import time
import logging
import pyvjoy # Windows apenas

class MyControllerMap:
    def __init__(self):
        self.button = {'UP': 1, 'DOWN': 2, 'LEFT': 3, 'RIGHT': 4, 'RED': 5, 'YELLOW' : 6, 'GREEN': 7, 'BLUE': 8}


class SerialControllerInterface:

    # Protocolo
    # byte 1 -> Botão 1 (estado - Apertado 1 ou não 0)
    # byte 2 -> EOP - End of Packet -> valor reservado 'X'

    def __init__(self, port, baudrate):
        self.ser = serial.Serial(port, baudrate=baudrate)
        self.mapping = MyControllerMap()
        self.j = pyvjoy.VJoyDevice(1)
        self.incoming = '0'

    def update(self):
        ## Sync protocol
        while self.incoming != b'X':
            self.incoming = self.ser.read()
            logging.debug("Received INCOMING: {}".format(self.incoming))

        id = self.ser.read()
        button = self.ser.read()
        value1 = self.ser.read()
        value2 = self.ser.read()

        logging.debug("Received DATA: {}, button {}, value1 {}".format(id, button, value1))

        if button == b'W':
            self.ser.write(b'w')
            logging.info("Mandando w de volta pro microship") 

        if button == b'1':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['UP'], 1)

        if button == b'A':
            self.j.set_button(self.mapping.button['UP'], 0)

        if button == b'2':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['DOWN'], 1)
        if button == b'B':
            self.j.set_button(self.mapping.button['DOWN'], 0)  

        if button == b'3':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['LEFT'], 1)
        if button == b'C':
            self.j.set_button(self.mapping.button['LEFT'], 0)  

        if button == b'4':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['RIGHT'], 1)
        if button == b'D':
            self.j.set_button(self.mapping.button['RIGHT'], 0)  

        if button == b'5':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['RED'], 1)
        if button == b'E':
            self.j.set_button(self.mapping.button['RED'], 0)  

        if button == b'6':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['YELLOW'], 1)
        if button == b'F':
            self.j.set_button(self.mapping.button['YELLOW'], 0)  

        if button == b'7':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['GREEN'], 1)
        if button == b'G':
            self.j.set_button(self.mapping.button['GREEN'], 0)  

        if button == b'8':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['BLUE'], 1)
        if button == b'H':
            self.j.set_button(self.mapping.button['BLUE'], 0)  

        if button == b'Y':
            if id == b'I':
                conv_valor1 = int.from_bytes(value2 + value1, byteorder="big")
                if (conv_valor1 > 3400 or conv_valor1 < 2000):
                    self.j.set_axis(pyvjoy.HID_USAGE_X, conv_valor1*int(32762/4095))
                else:
                    self.j.set_axis(pyvjoy.HID_USAGE_X, int(32762/2))

            if id == b'J':
                conv_valor2 = int.from_bytes(value2 + value1, byteorder="big")
                if (conv_valor2 > 3400 or conv_valor2 < 2000):
                    self.j.set_axis(pyvjoy.HID_USAGE_Y, conv_valor2*int(32762/4095))
                else:
                    self.j.set_axis(pyvjoy.HID_USAGE_Y, int(32762/2))

        if button == b'Z':
            if id == b'K':
                conv_valor3 = int.from_bytes(value2 + value1, byteorder="big")
                if (conv_valor3 > 3400 or conv_valor3 < 2000):
                    self.j.set_axis(pyvjoy.HID_USAGE_RX, conv_valor3*int(32762/4095))
                else:
                    self.j.set_axis(pyvjoy.HID_USAGE_RX, int(32762/2))
            
            if id == b'L':
                conv_valor4 = int.from_bytes(value2 + value1, byteorder="big")
                if (conv_valor4 > 3400 or conv_valor4 < 2000):
                    self.j.set_axis(pyvjoy.HID_USAGE_RY, conv_valor4*int(32762/4095))
                else:
                    self.j.set_axis(pyvjoy.HID_USAGE_RY, int(32762/2))


        self.incoming = self.ser.read()


class DummyControllerInterface:
    def __init__(self):
        self.mapping = MyControllerMap()
        self.j = pyvjoy.VJoyDevice(1)

    def update(self):
        self.j.set_button(self.mapping.button['A'], 1)
        time.sleep(0.1)
        self.j.set_button(self.mapping.button['A'], 0)
        logging.info("[Dummy] Pressed A button")
        time.sleep(1)


if __name__ == '__main__':
    interfaces = ['dummy', 'serial']
    argparse = argparse.ArgumentParser()
    argparse.add_argument('serial_port', type=str)
    argparse.add_argument('-b', '--baudrate', type=int, default=9600)
    argparse.add_argument('-c', '--controller_interface', type=str, default='serial', choices=interfaces)
    argparse.add_argument('-d', '--debug', default=False, action='store_true')
    args = argparse.parse_args()
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    print("Connection to {} using {} interface ({})".format(args.serial_port, args.controller_interface, args.baudrate))
    if args.controller_interface == 'dummy':
        controller = DummyControllerInterface()
    else:
        controller = SerialControllerInterface(port=args.serial_port, baudrate=args.baudrate)

    while True:
        controller.update()
