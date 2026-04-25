#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <simpleble/SimpleBLE.h>
#include <jetgpio.h>
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <thread>
#include <chrono>

#include "../../../../../usr/include/errno.h"

using namespace std;

/* Global variable to interrupt the loop later on*/
static volatile int interrupt = 1;
unsigned long timestamp;
struct termios tty;
int serial_port;
bool initializing;

void searchAndConnect();
void connect(SimpleBLE::Peripheral* peripheral);
void setupGPIO();
void setupSerialComm();

int main() {
    initializing = true;
    thread gpioThread(setupGPIO);
    thread serialThread(setupSerialComm);

    // unsigned char msg[] = { 'H', 'e', 'l', 'l', 'o', '\r'};
    // this_thread::sleep_for(chrono::seconds(5));
    // cout << "Sending Hello over serial" << endl;
    // write(serial_port, msg, sizeof(msg));

    gpioThread.join();
    serialThread.join();

    return 0;

}

//* Ctrl-c signal function handler */
void inthandler(int signum)
{
    usleep(1000);
    printf("\nCaught Ctrl-c, coming out ...\n");
    interrupt = 0;
}

/* Function to be called upon if edge is detected */
void calling()
{
    searchAndConnect();
}

void setupGPIO()
{
    int Init;

    /* Capture Ctrl-c */
    signal(SIGINT, inthandler);

    Init = gpioInitialise();
    if (Init < 0)
    {
        /* jetgpio initialisation failed */
        printf("Jetgpio initialisation failed. Error code:  %d\n", Init);
        exit(Init);
    }
    else
    {
        /* jetgpio initialised okay*/
        printf("Jetgpio initialisation OK. Return code:  %d\n", Init);
    }

    // Setting up pin 7 as INPUT first
    int stat = gpioSetMode(7, JET_INPUT);
    if (stat < 0)
    {
        // gpio setting up failed
        printf("gpio setting up failed. Error code:  %d\n", stat);
        exit(Init);
    }
    else
    {
        // gpio setting up okay
        printf("gpio setting up okay. Return code:  %d\n", stat);
    }

    int level = gpioRead(7);
    printf("GPIO LEVEL: %d\n",level);
    // Now setting up pin 7 to detect edges, rising & falling edge with a 1000 useconds debouncing and when event is detected calling func "calling"
    int stat2 = gpioSetISRFunc(7, RISING_EDGE, 1000, &timestamp, &calling);
    if (stat2 < 0)
    {
        /* gpio setting up failed */
        printf("gpio edge setting up failed. Error code:  %d\n", stat2);
        exit(Init);
    }
    else
    {
        /* gpio setting up okay*/
        printf("gpio edge setting up okay. Return code:  %d\n", stat2);
    }

    /* Now wait for the edge to be detected */
    printf("Capturing edges, press Ctrl-c to terminate\n");
    while (interrupt) {
        // Do some stuff
        sleep(1);
    }
    // Terminating library
    gpioTerminate();
    exit(0);
}

void setupSerialComm() {
    serial_port = open("/dev/ttyACM2", O_RDWR);
    if (serial_port < 0) {
        cout << "Error opening serial port " << serial_port << endl;
    }
    cout << "Serial port opened" << endl;

    if (tcgetattr(serial_port, &tty) != 0) {
        cout << "Error " << errno << " from tcgetattr() " << endl;
    }
    cout << "Got terminos struct to configure uart settings" << endl;

    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 Stop bit
    tty.c_cflag &= ~CSIZE;         // Clear size
    tty.c_cflag |= CS8;            // 8 bits per byte
    tty.c_cflag &= ~CRTSCTS;       // Disable hardware control flow
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;        // Disable canonical mode
    tty.c_lflag &= ~ECHO;          // Disable echo
    tty.c_lflag &= ~ECHOE;         // Disable erasure
    tty.c_lflag &= ~ECHONL;        // Disable new-line echo
    tty.c_lflag &= ~ISIG;          // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200); // BAUD Rate 115200

    // Saving terminos with config
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        cout << "Error " << strerror(errno) << " from tcsetattr" << endl;
    }
    cout << "Terminos saved with config" << endl;
}

void searchAndConnect()
{
    string DEVICE_ADDRESS = "2C:CF:67:46:97:7F";

    auto adapters = SimpleBLE::Adapter::get_adapters();
    auto adapter = adapters[0];
    cout << "Using adapter: " << adapter.identifier() << " [" << adapter.address() << "]" << endl;

    vector<SimpleBLE::Peripheral> peripherals;

    adapter.set_callback_on_scan_start([]() { cout << "Scan started." << endl; });
    adapter.set_callback_on_scan_stop([]() { cout << "Scan stopped." << endl; });
    adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {
        cout << "Found device: " << peripheral.identifier() << " [" << peripheral.address() << "]" << endl;

        if (peripheral.is_connectable()) {
            peripherals.push_back(peripheral);
        }
    });

    adapter.scan_for(5000);

    SimpleBLE::Peripheral peripheral;
    for (int i = 0; i < peripherals.size(); i++) {
        SimpleBLE::Peripheral peripheralTemp = peripherals[i];
        if (peripheralTemp.address() == DEVICE_ADDRESS) {
            peripheral = peripherals[i];
            break;
        }
    }

    cout << "Device To Connect To" << peripheral.identifier() << " [" << peripheral.address() << "]" << endl;

    // TODO: This wil be conditional. It will call disconnect if the button is pressed and connected
    // peripheral.disconnect();
    connect(&peripheral);
}

void connect(SimpleBLE::Peripheral* peripheral) {
    cout << "Connecting to " << peripheral->identifier() << " [" << peripheral->address() << "]" << endl;
    peripheral->connect();

    vector<pair<SimpleBLE::BluetoothUUID, SimpleBLE::BluetoothUUID>> readable_characteristics;
    for (auto& service : peripheral->services()) {
        for (auto& characteristic : service.characteristics()) {
            if (characteristic.can_read()) {
                readable_characteristics.emplace_back(service.uuid(), characteristic.uuid());
            }
        }
    }

    if (readable_characteristics.empty()) {
        cerr << "The peripheral has no readable characteristics." << endl;
        peripheral->disconnect();
        // return EXIT_FAILURE;
    }

    cout << "Readable characteristics:" << endl;
    for (size_t i = 0; i < readable_characteristics.size(); i++) {
        cout << "[" << i << "] " << readable_characteristics[i].first << " " << readable_characteristics[i].second << endl;
    }
}
