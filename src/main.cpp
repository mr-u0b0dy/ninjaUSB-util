#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QCoreApplication>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>
#include <iostream>
#include <memory>
#include <vector>

class BLEScanner : public QObject {
  Q_OBJECT

public:
  BLEScanner(QObject *parent = nullptr) : QObject(parent) {
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    discoveryAgent->setLowEnergyDiscoveryTimeout(8000);

    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BLEScanner::onDeviceDiscovered);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &BLEScanner::onScanFinished);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BLEScanner::onScanError);

    std::cout << "Starting BLE device scan...\n";
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
  }

private slots:
  void onDeviceDiscovered(const QBluetoothDeviceInfo &info) {
    if (!(info.coreConfigurations() &
          QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
      return;

    for (const auto &d : devices)
      if (d.address() == info.address())
        return; // Already added

    devices.push_back(info);

    std::cout
        << devices.size() - 1 << ") "
        << (info.name().isEmpty() ? "(unknown)" : info.name()).toStdString()
        << " [" << info.address().toString().toStdString() << "]\n";
  }

  void onScanFinished() {
    if (devices.empty()) {
      std::cout << "No BLE devices found.\n";
      QCoreApplication::quit();
      return;
    }

    std::cout << "\nScan complete. Enter device index to connect: ";
    int index = -1;
    std::cin >> index;

    if (index < 0 || index >= static_cast<int>(devices.size())) {
      std::cerr << "Invalid index.\n";
      QCoreApplication::quit();
      return;
    }

    selectedDevice = devices[index];
    connectToDevice();
  }

  void connectToDevice() {
    std::cout << "Connecting to: " << selectedDevice.name().toStdString()
              << " [" << selectedDevice.address().toString().toStdString()
              << "]\n";

    controller.reset(QLowEnergyController::createCentral(selectedDevice, this));

    connect(controller.get(), &QLowEnergyController::connected, this,
            &BLEScanner::onConnected);
    connect(controller.get(), &QLowEnergyController::disconnected, this,
            &BLEScanner::onDisconnected);
    connect(controller.get(), &QLowEnergyController::errorOccurred, this,
            &BLEScanner::onControllerError);
    connect(controller.get(), &QLowEnergyController::serviceDiscovered, this,
            &BLEScanner::onServiceDiscovered);
    connect(controller.get(), &QLowEnergyController::discoveryFinished, this,
            &BLEScanner::onServiceDiscoveryFinished);

    controller->connectToDevice();
  }

  void onConnected() {
    std::cout << "Connected. Discovering services...\n";
    controller->discoverServices();
  }

  void onDisconnected() {
    std::cout << "Disconnected. Exiting...\n";
    QCoreApplication::quit();
  }

  void onControllerError(QLowEnergyController::Error error) {
    std::cerr << "Connection error: " << controller->errorString().toStdString()
              << "\n";
    QCoreApplication::quit();
  }

  void onServiceDiscovered(const QBluetoothUuid &uuid) {
    std::cout << "Service found: " << uuid.toString().toStdString() << "\n";
  }

  void onServiceDiscoveryFinished() {
    std::cout << "Service discovery complete. Getting service details...\n";

    const auto serviceUuids = controller->services();
    for (const auto &uuid : serviceUuids) {
      QLowEnergyService *service = controller->createServiceObject(uuid, this);
      if (!service) {
        std::cerr << "Cannot create service object for "
                  << uuid.toString().toStdString() << "\n";
        continue;
      }

      connect(service, &QLowEnergyService::stateChanged, this,
              [this, service](QLowEnergyService::ServiceState state) {
                if (state == QLowEnergyService::RemoteServiceDiscovered) {
                  std::cout << "\n✅ Service "
                            << service->serviceUuid().toString().toStdString()
                            << " discovered\n";

                  const auto characteristics = service->characteristics();
                  for (const auto &ch : characteristics) {
                    std::cout << "  Characteristic: "
                              << ch.uuid().toString().toStdString() << "\n";

                    auto props = ch.properties();
                    std::cout << "    Properties:";
                    if (props & QLowEnergyCharacteristic::Read)
                      std::cout << " Read";
                    if (props & QLowEnergyCharacteristic::Write)
                      std::cout << " Write";
                    if (props & QLowEnergyCharacteristic::WriteNoResponse)
                      std::cout << " WriteNoResp";
                    if (props & QLowEnergyCharacteristic::Notify)
                      std::cout << " Notify";
                    if (props & QLowEnergyCharacteristic::Indicate)
                      std::cout << " Indicate";
                    if (props & QLowEnergyCharacteristic::ExtendedProperty)
                      std::cout << " Extended";
                    std::cout << "\n";

                    if ((props & QLowEnergyCharacteristic::Write) ||
                        (props & QLowEnergyCharacteristic::WriteNoResponse)) {
                      std::cout
                          << "    → Writing data to this characteristic...\n";

                      QByteArray data;
                      data.append(static_cast<char>(0x02));
                      data.append(static_cast<char>(0x00));
                      data.append(static_cast<char>(0x04));
                      data.append(static_cast<char>(0x05));
                      data.append(static_cast<char>(0x00));
                      data.append(static_cast<char>(0x00));

                      service->writeCharacteristic(
                          ch, data,
                          (props & QLowEnergyCharacteristic::Write)
                              ? QLowEnergyService::WriteWithResponse
                              : QLowEnergyService::WriteWithoutResponse);

                      QByteArray data2;
                      data2.append(static_cast<char>(0x00));
                      data2.append(static_cast<char>(0x00));
                      data2.append(static_cast<char>(0x00));
                      data2.append(static_cast<char>(0x00));
                      data2.append(static_cast<char>(0x00));
                      data2.append(static_cast<char>(0x00));

                      service->writeCharacteristic(
                          ch, data2,
                          (props & QLowEnergyCharacteristic::Write)
                              ? QLowEnergyService::WriteWithResponse
                              : QLowEnergyService::WriteWithoutResponse);
                    }
                  }
                }
              });

      service->discoverDetails();
      // service->push_back(service);
    }

    // Optionally: disconnect after all service details printed
    QTimer::singleShot(1000, this, [this]() {
      std::cout << "\nDone. Disconnecting...\n";
      controller->disconnectFromDevice();
    });

    if (serviceUuids.isEmpty()) {
      std::cout << "No services found.\n";
      controller->disconnectFromDevice();
    }
  }

  void onScanError(QBluetoothDeviceDiscoveryAgent::Error error) {
    std::cerr << "Scan error: " << discoveryAgent->errorString().toStdString()
              << "\n";
    QCoreApplication::quit();
  }

private:
  QBluetoothDeviceDiscoveryAgent *discoveryAgent;
  std::vector<QBluetoothDeviceInfo> devices;
  QBluetoothDeviceInfo selectedDevice;
  std::unique_ptr<QLowEnergyController> controller;
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  BLEScanner scanner;
  return app.exec();
}
