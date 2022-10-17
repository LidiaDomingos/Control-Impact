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

        data = self.ser.read()
        logging.debug("Received DATA: {}".format(data))
        print(data)

        if data == b'1':
            logging.info("Sending press")
            print('entrei no 1')
            self.j.set_button(self.mapping.button['UP'], 1)

        elif data == b'A':
            print('entrei no 10')
            self.j.set_button(self.mapping.button['UP'], 0)

        if data == b'2':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['DOWN'], 1)
        elif data == b'B':
            self.j.set_button(self.mapping.button['DOWN'], 0)  

        if data == b'3':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['LEFT'], 1)
        elif data == b'C':
            self.j.set_button(self.mapping.button['LEFT'], 0)  

        if data == b'4':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['RIGHT'], 1)
        elif data == b'D':
            self.j.set_button(self.mapping.button['RIGHT'], 0)  

        if data == b'5':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['RED'], 1)
        elif data == b'E':
            self.j.set_button(self.mapping.button['RED'], 0)  

        if data == b'6':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['YELLOW'], 1)
        elif data == b'F':
            self.j.set_button(self.mapping.button['YELLOW'], 0)  

        if data == b'7':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['GREEN'], 1)
        elif data == b'G':
            self.j.set_button(self.mapping.button['GREEN'], 0)  

        if data == b'8':
            logging.info("Sending press")
            self.j.set_button(self.mapping.button['BLUE'], 1)
        elif data == b'H':
            self.j.set_button(self.mapping.button['BLUE'], 0)  

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
