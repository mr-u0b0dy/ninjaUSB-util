#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QCoreApplication>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>
#include <array>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <libudev.h>
#include <map>
#include <memory>
#include <poll.h>
#include <set>
#include <unistd.h>
#include <vector>

// Linux key code to HID usage ID (partial)
std::map<int, uint8_t> linuxToHID = {
    {KEY_A, 0x04},          {KEY_B, 0x05},        {KEY_C, 0x06},
    {KEY_D, 0x07},          {KEY_E, 0x08},        {KEY_F, 0x09},
    {KEY_G, 0x0A},          {KEY_H, 0x0B},        {KEY_I, 0x0C},
    {KEY_J, 0x0D},          {KEY_K, 0x0E},        {KEY_L, 0x0F},
    {KEY_M, 0x10},          {KEY_N, 0x11},        {KEY_O, 0x12},
    {KEY_P, 0x13},          {KEY_Q, 0x14},        {KEY_R, 0x15},
    {KEY_S, 0x16},          {KEY_T, 0x17},        {KEY_U, 0x18},
    {KEY_V, 0x19},          {KEY_W, 0x1A},        {KEY_X, 0x1B},
    {KEY_Y, 0x1C},          {KEY_Z, 0x1D},        {KEY_1, 0x1E},
    {KEY_2, 0x1F},          {KEY_3, 0x20},        {KEY_4, 0x21},
    {KEY_5, 0x22},          {KEY_6, 0x23},        {KEY_7, 0x24},
    {KEY_8, 0x25},          {KEY_9, 0x26},        {KEY_0, 0x27},
    {KEY_ENTER, 0x28},      {KEY_ESC, 0x29},      {KEY_BACKSPACE, 0x2A},
    {KEY_TAB, 0x2B},        {KEY_SPACE, 0x2C},    {KEY_LEFTSHIFT, 0xE1},
    {KEY_RIGHTSHIFT, 0xE5}, {KEY_LEFTCTRL, 0xE0}, {KEY_RIGHTCTRL, 0xE4},
    {KEY_LEFTALT, 0xE2},    {KEY_RIGHTALT, 0xE6},
};

std::set<uint8_t> pressedKeys;
uint8_t modifiers = 0;

void handle_key_event(int code, int value) {
  if (linuxToHID.find(code) == linuxToHID.end())
    return;
  uint8_t hid_code = linuxToHID[code];

  if (hid_code >= 0xE0 && hid_code <= 0xE7) {
    uint8_t bit = 1 << (hid_code - 0xE0);
    if (value == 1)
      modifiers |= bit;
    else if (value == 0)
      modifiers &= ~bit;
  } else {
    if (value == 1)
      pressedKeys.insert(hid_code);
    else if (value == 0)
      pressedKeys.erase(hid_code);
  }
}

struct KeyboardDevice {
  int fd;
  libevdev *dev;
};

std::vector<KeyboardDevice> find_all_keyboards() {
  std::vector<KeyboardDevice> keyboards;
  struct udev *udev = udev_new();
  struct udev_enumerate *enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, devices) {
    const char *path = udev_list_entry_get_name(entry);
    struct udev_device *dev = udev_device_new_from_syspath(udev, path);
    const char *devnode = udev_device_get_devnode(dev);
    if (!devnode || !strstr(devnode, "event")) {
      udev_device_unref(dev);
      continue;
    }

    int fd = open(devnode, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      udev_device_unref(dev);
      continue;
    }

    libevdev *evdev = nullptr;
    if (libevdev_new_from_fd(fd, &evdev) < 0) {
      close(fd);
      udev_device_unref(dev);
      continue;
    }

    if (libevdev_has_event_type(evdev, EV_KEY) &&
        libevdev_has_event_code(evdev, EV_KEY, KEY_A)) {
      keyboards.push_back({fd, evdev});
    } else {
      libevdev_free(evdev);
      close(fd);
    }
    udev_device_unref(dev);
  }
  udev_enumerate_unref(enumerate);
  udev_unref(udev);
  return keyboards;
}

class BLEScanner : public QObject {
  Q_OBJECT

public:
  BLEScanner(std::vector<KeyboardDevice> &kbs,
             std::vector<struct pollfd> &fdsIn, QObject *parent = nullptr)
      : QObject(parent), keyboards(kbs), fds(fdsIn) {
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    discoveryAgent->setLowEnergyDiscoveryTimeout(8000);

    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BLEScanner::onDeviceDiscovered);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
            &BLEScanner::onScanFinished);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BLEScanner::onScanError);

    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
  }

private slots:
  void onDeviceDiscovered(const QBluetoothDeviceInfo &info) {
    if (!(info.coreConfigurations() &
          QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
      return;
    for (const auto &d : devices)
      if (d.address() == info.address())
        return;
    devices.push_back(info);
    std::cout << devices.size() - 1 << ") " << info.name().toStdString() << " ["
              << info.address().toString().toStdString() << "]\n";
  }

  void onScanFinished() {
    if (devices.empty()) {
      std::cout << "No BLE devices found.\n";
      QCoreApplication::quit();
      return;
    }
    std::cout << "Select device index to connect: ";
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
    controller.reset(QLowEnergyController::createCentral(selectedDevice, this));
    connect(controller.get(), &QLowEnergyController::connected, this,
            &BLEScanner::onConnected);
    connect(controller.get(), &QLowEnergyController::serviceDiscovered, this,
            &BLEScanner::onServiceDiscovered);
    connect(controller.get(), &QLowEnergyController::discoveryFinished, this,
            &BLEScanner::onServiceDiscoveryFinished);
    controller->connectToDevice();
  }

  void onConnected() { controller->discoverServices(); }

  void onServiceDiscovered(const QBluetoothUuid &uuid) {
    std::cout << "Discovered service: " << uuid.toString().toStdString()
              << "\n";
  }

  void onServiceDiscoveryFinished() {
    for (const auto &uuid : controller->services()) {
      QLowEnergyService *service = controller->createServiceObject(uuid, this);
      if (!service)
        continue;

      connect(service, &QLowEnergyService::stateChanged, this,
              [this, service](QLowEnergyService::ServiceState state) {
                if (state == QLowEnergyService::RemoteServiceDiscovered) {
                  for (const auto &ch : service->characteristics()) {
                    auto props = ch.properties();
                    if ((props & QLowEnergyCharacteristic::Write) ||
                        (props & QLowEnergyCharacteristic::WriteNoResponse)) {
                      writeChar = ch;
                      targetService = service;
                      startPolling();
                      return;
                    }
                  }
                }
              });
      service->discoverDetails();
    }
  }

  void startPolling() {
    keyPollTimer = new QTimer(this);
    connect(keyPollTimer, &QTimer::timeout, this, [this]() {
      if (poll(fds.data(), fds.size(), 0) > 0) {
        for (size_t i = 0; i < keyboards.size(); ++i) {
          if (fds[i].revents & POLLIN) {
            struct input_event ev;
            while (libevdev_next_event(keyboards[i].dev,
                                       LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
              if (ev.type == EV_KEY) {
                handle_key_event(ev.code, ev.value);

                std::array<uint8_t, 8> report = {0};
                report[0] = modifiers;
                size_t idx = 2;
                for (uint8_t key : pressedKeys) {
                  if (key >= 0xE0 && key <= 0xE7)
                    continue;
                  if (idx < 8)
                    report[idx++] = key;
                }
                QByteArray data(reinterpret_cast<const char *>(report.data()),
                                report.size());
                targetService->writeCharacteristic(
                    writeChar, data,
                    (writeChar.properties() & QLowEnergyCharacteristic::Write)
                        ? QLowEnergyService::WriteWithResponse
                        : QLowEnergyService::WriteWithoutResponse);
              }
            }
          }
        }
      }
    });
    keyPollTimer->start(10);
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
  QLowEnergyCharacteristic writeChar;
  QLowEnergyService *targetService = nullptr;
  std::vector<KeyboardDevice> &keyboards;
  std::vector<struct pollfd> &fds;
  QTimer *keyPollTimer = nullptr;
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  auto keyboards = find_all_keyboards();
  if (keyboards.empty()) {
    std::cerr << "No keyboards found.\n";
    return 1;
  }
  std::vector<struct pollfd> fds;
  for (const auto &kbd : keyboards)
    fds.push_back({kbd.fd, POLLIN, 0});

  BLEScanner scanner(keyboards, fds);
  int result = app.exec();
  for (auto &kbd : keyboards) {
    libevdev_free(kbd.dev);
    close(kbd.fd);
  }
  return result;
}
