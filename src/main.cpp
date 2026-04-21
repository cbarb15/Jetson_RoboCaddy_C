#include <iostream>
#include <algorithm>
#include <simpleble/SimpleBLE.h>

using namespace std;

void connect(SimpleBLE::Peripheral* peripheral);

int main() {
    string DEVICE_ADDRESS = "2C:CF:67:46:97:7F";

    auto adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty()) {
        cerr << "No Bluetooth adapters found." << endl;
        return EXIT_FAILURE;
    }
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

    // connect(&peripheral);
    peripheral.disconnect();
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