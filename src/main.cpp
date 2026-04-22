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

using namespace std;

/* Global variable to interrupt the loop later on*/
static volatile int interrupt = 1;
unsigned long timestamp;
struct termios tty;

void searchAndConnect();
void connect(SimpleBLE::Peripheral* peripheral);
void setupGPIO();

int main() {
    setupGPIO();

    int serial_port = open("/dev/ttyACM0", O_RDWR);
    if (serial_port < 0) {
        cout << "Error opening serial port " << serial_port << endl;
    }

    if (tcgetattr(serial_port, &tty) != 0) {
        cout << "Error " << errno << " from tcgetattr() " << endl;
    }
    // peripheral.disconnect();
}


/* Ctrl-c signal function handler */
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

    // Now setting up pin 7 to detect edges, rising & falling edge with a 1000 useconds debouncing and when event is detected calling func "calling"
    int stat2 = gpioSetISRFunc(7, EITHER_EDGE, 1000, &timestamp, &calling);
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