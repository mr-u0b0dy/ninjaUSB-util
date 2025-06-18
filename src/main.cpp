#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothLocalDevice>
#include <QCoreApplication>
#include <QObject>
#include <QTimer>
#include <iostream>

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

    std::cout << "Starting BLE device scan..." << std::endl;
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
  }

private slots:
  void addDevice(const QBluetoothDeviceInfo &info) {
    if (info.coreConfigurations() &
        QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
      std::cout << "Found device: " << info.name().toStdString() << " ["
                << info.address().toString().toStdString() << "]" << std::endl;
    }
  }

  void scanFinished() {
    std::cout << "Device scan finished." << std::endl;
    QCoreApplication::quit();
  }

  void scanError(QBluetoothDeviceDiscoveryAgent::Error error) {
    std::cerr << "Scan error: " << discoveryAgent->errorString().toStdString()
              << std::endl;
    QCoreApplication::quit();
  }

private:
  QBluetoothDeviceDiscoveryAgent *discoveryAgent;
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  BLEScanner scanner;
  return app.exec();
}
