#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
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
    discoveryAgent->setLowEnergyDiscoveryTimeout(10000); // 10 seconds

    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BLEScanner::addDevice);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &BLEScanner::scanFinished);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BLEScanner::scanError);

    std::cout << "Scanning for BLE devices..." << std::endl;
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
  }

private slots:
  void addDevice(const QBluetoothDeviceInfo &info) {
    if (info.coreConfigurations() &
        QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
      for (const auto &d : devices)
        if (d.address() == info.address())
          return;

      devices.push_back(info);
      std::cout
          << devices.size() - 1 << ") "
          << (info.name().isEmpty() ? "(Unknown)" : info.name()).toStdString()
          << " [" << info.address().toString().toStdString() << "]"
          << std::endl;
    }
  }

  void scanFinished() {
    if (devices.empty()) {
      std::cout << "No BLE devices found." << std::endl;
      QCoreApplication::quit();
      return;
    }

    std::cout << "\nScan finished. Select device index to connect: ";
    int index = -1;
    std::cin >> index;

    if (index < 0 || index >= static_cast<int>(devices.size())) {
      std::cerr << "Invalid index." << std::endl;
      QCoreApplication::quit();
      return;
    }

    selectedDevice = devices[index];
    connectToDevice();
  }

  void connectToDevice() {
    std::cout << "Connecting to: " << selectedDevice.name().toStdString()
              << " [" << selectedDevice.address().toString().toStdString()
              << "]" << std::endl;

    controller = QLowEnergyController::createCentral(selectedDevice, this);

    connect(controller, &QLowEnergyController::connected, this,
            &BLEScanner::deviceConnected);
    connect(controller, &QLowEnergyController::disconnected, this,
            &BLEScanner::deviceDisconnected);
    connect(controller, &QLowEnergyController::errorOccurred, this,
            &BLEScanner::controllerError);
    connect(controller, &QLowEnergyController::serviceDiscovered, this,
            &BLEScanner::addService);
    connect(controller, &QLowEnergyController::discoveryFinished, this,
            &BLEScanner::serviceDiscoveryDone);

    controller->connectToDevice();
  }

  void deviceConnected() {
    std::cout << "Connected. Discovering services..." << std::endl;
    controller->discoverServices();
  }

  void deviceDisconnected() {
    std::cerr << "Disconnected from device." << std::endl;
    QTimer::singleShot(500, QCoreApplication::instance(),
                       &QCoreApplication::quit);
  }

  void controllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error);
    std::cerr << "Connection error: " << controller->errorString().toStdString()
              << std::endl;
    QTimer::singleShot(500, QCoreApplication::instance(),
                       &QCoreApplication::quit);
  }

  void addService(const QBluetoothUuid &uuid) {
    bool isPrimary = controller->services().contains(uuid);
    std::cout << "Found service: " << uuid.toString().toStdString() << " ("
              << (isPrimary ? "Primary" : "Secondary") << ")" << std::endl;
  }

  void serviceDiscoveryDone() {
    std::cout << "Service discovery finished." << std::endl;
    QTimer::singleShot(500, QCoreApplication::instance(),
                       &QCoreApplication::quit);
  }

  void scanError(QBluetoothDeviceDiscoveryAgent::Error error) {
    std::cerr << "Scan error: " << discoveryAgent->errorString().toStdString()
              << std::endl;
    QTimer::singleShot(500, QCoreApplication::instance(),
                       &QCoreApplication::quit);
  }

private:
  QBluetoothDeviceDiscoveryAgent *discoveryAgent;
  QLowEnergyController *controller = nullptr;
  std::vector<QBluetoothDeviceInfo> devices;
  QBluetoothDeviceInfo selectedDevice;
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  BLEScanner scanner;
  return app.exec();
}
