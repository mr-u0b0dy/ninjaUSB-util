#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QByteArray>
#include <QCoreApplication>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QThread>
#include <QTimer>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <termios.h>
#include <unistd.h>

std::map<char, uint8_t> asciiToHid = {
    {'a', 0x04}, {'b', 0x05},  {'c', 0x06}, {'d', 0x07}, {'e', 0x08},
    {'f', 0x09}, {'g', 0x0A},  {'h', 0x0B}, {'i', 0x0C}, {'j', 0x0D},
    {'k', 0x0E}, {'l', 0x0F},  {'m', 0x10}, {'n', 0x11}, {'o', 0x12},
    {'p', 0x13}, {'q', 0x14},  {'r', 0x15}, {'s', 0x16}, {'t', 0x17},
    {'u', 0x18}, {'v', 0x19},  {'w', 0x1A}, {'x', 0x1B}, {'y', 0x1C},
    {'z', 0x1D}, {'1', 0x1E},  {'2', 0x1F}, {'3', 0x20}, {'4', 0x21},
    {'5', 0x22}, {'6', 0x23},  {'7', 0x24}, {'8', 0x25}, {'9', 0x26},
    {'0', 0x27}, {'\n', 0x28}, {' ', 0x2C}};

char getch() {
  struct termios oldt{}, newt{};
  char ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

int kbhit() {
  struct termios oldt{}, newt{};
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

QByteArray createHidReport(char key) {
  QByteArray report(8, 0x00);
  if (asciiToHid.count(key)) {
    report[2] = asciiToHid[key];
  }
  return report;
}

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QBluetoothDeviceDiscoveryAgent *discoveryAgent =
      new QBluetoothDeviceDiscoveryAgent();
  QList<QBluetoothDeviceInfo> foundDevices;
  QLowEnergyController *controller = nullptr;
  QLowEnergyService *service = nullptr;
  QLowEnergyCharacteristic targetChar;

  QObject::connect(
      discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
      [&](const QBluetoothDeviceInfo &info) {
        std::cout << foundDevices.size() << ": " << info.name().toStdString()
                  << " [" << info.address().toString().toStdString() << "]\n";
        foundDevices.append(info);
      });

  QObject::connect(
      discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, [&]() {
        std::cout << "Discovery complete. Choose device number: ";
        int index;
        std::cin >> index;

        if (index < 0 || index >= foundDevices.size()) {
          std::cerr << "Invalid index\n";
          app.quit();
          return;
        }

        const QBluetoothDeviceInfo &device = foundDevices[index];
        controller = QLowEnergyController::createCentral(device);

        QObject::connect(controller, &QLowEnergyController::connected, [&]() {
          std::cout << "Connected to device. Discovering services...\n";
          controller->discoverServices();
        });

        QObject::connect(controller, &QLowEnergyController::disconnected,
                         [&]() {
                           std::cerr << "Disconnected.\n";
                           app.quit();
                         });

        QObject::connect(controller, &QLowEnergyController::serviceDiscovered,
                         [&](const QBluetoothUuid &serviceUuid) {
                           std::cout << "Service discovered: "
                                     << serviceUuid.toString().toStdString()
                                     << "\n";
                         });

        QObject::connect(
            controller, &QLowEnergyController::discoveryFinished, [&]() {
              const QList<QBluetoothUuid> services = controller->services();
              for (const QBluetoothUuid &uuid : services) {
                std::cout << "Trying service: " << uuid.toString().toStdString()
                          << "\n";
                service = controller->createServiceObject(uuid);
                if (!service)
                  continue;

                QObject::connect(
                    service, &QLowEnergyService::stateChanged,
                    [&](QLowEnergyService::ServiceState s) {
                      if (s == QLowEnergyService::RemoteServiceDiscovered) {
                        const auto chars = service->characteristics();
                        for (const auto &c : chars) {
                          if (c.properties() &
                                  QLowEnergyCharacteristic::WriteNoResponse ||
                              c.properties() &
                                  QLowEnergyCharacteristic::Write) {
                            targetChar = c;
                            std::cout
                                << "Writable characteristic found: "
                                << targetChar.uuid().toString().toStdString()
                                << "\n";

                            std::cout
                                << "Start typing. Press Ctrl+C to exit.\n";
                            while (true) {
                              if (kbhit()) {
                                char key = getch();
                                QByteArray report = createHidReport(key);
                                service->writeCharacteristic(targetChar,
                                                             report);
                                QThread::msleep(10);
                                QByteArray release(8, 0x00);
                                service->writeCharacteristic(targetChar,
                                                             release);
                              }
                              QCoreApplication::processEvents();
                              QThread::msleep(5);
                            }
                          }
                        }
                      }
                    });
                service->discoverDetails();
                break;
              }
            });

        controller->connectToDevice();
      });

  discoveryAgent->start();
  return app.exec();
}
