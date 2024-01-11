import 'dart:io';
import 'dart:convert';
import 'dart:developer' as developer;
import 'package:Scanner3D/main.dart';
import 'package:Scanner3D/src/presentation/pages/receive_scanned_object_page.dart';
import 'package:Scanner3D/src/services/notification_service.dart';
import 'package:Scanner3D/src/services/scan_provider.dart';
import 'package:flutter/material.dart';

class SocketService extends ChangeNotifier {
  late Socket _serverSocket;
  late Socket _configSocket;
  late Socket _broadcastSocket;
  late Socket _imageSocket;
  late Socket _liveSocket;

  String _ipAddress = "127.0.0.1";
  String get ipAddress => _ipAddress;
  final int _serverPort = 3000;
  final int _configPort = 3001;
  final int _broadcastPort = 3002;
  final int _imagePort = 3003;
  final int _livePort = 3004;

  bool _isConnected = false;
  bool get isConnected => _isConnected;

  StringBuffer stringBuffer = StringBuffer();

  SocketService._private();
  static final SocketService _instance = SocketService._private();
  static SocketService get instance => _instance;

  int _currentRound = 0;
  int _totalRound = 1;
  int _fileSize = 0;
  int _totalFileSize = 1;
  int _receivedFileSize = 0;
  int get currentRound => _currentRound;
  int get totalRound => _totalRound;
  int get fileSize => _fileSize;
  int get totalFileSize => _totalFileSize;
  int get receivedFileSize => _receivedFileSize;

  void connectSocket(String ipAddress) async {
    bool flag = true;
    _ipAddress = ipAddress;

    try {
      _serverSocket = await Socket.connect(_ipAddress, _serverPort);

      _serverSocket.listen((List<int> data) {}, onDone: () {
        disconnectSockets();
        developer.log('Server socket closed');
      }, onError: (error) {
        showCustomToast(navigatorKey.currentContext!, "Error in socket!");
      });
    } catch (e) {
      flag = false;
      showCustomToast(navigatorKey.currentContext!, "Error in server socket!");
    }

    if (flag) {
      try {
        _configSocket = await Socket.connect(_ipAddress, _configPort);

        _configSocket.listen((List<int> data) {}, onDone: () {
          disconnectSockets();
          developer.log('Config socket closed');
        }, onError: (error) {
          showCustomToast(navigatorKey.currentContext!, "Error in socket!");
        });
      } catch (e) {
        flag = false;
        showCustomToast(navigatorKey.currentContext!, "Error in socket!");
      }
    }

    if (flag) {
      try {
        _broadcastSocket = await Socket.connect(_ipAddress, _broadcastPort);

        _broadcastSocket.listen((List<int> data) async {
          String receivedData = utf8.decode(data);

          if (receivedData == "scanner_state RUNNING") {
            ScanProvider.instance.startScan();
            ScanProvider.instance.onScanCompleted = () {
              NotificationService.showBigTextNotification(
                  title: "Scanning completed successfully",
                  body: "Let's take a look at the scanned object",
                  fln: flutterLocalNotificationsPlugin);

              navigatorKey.currentState?.push(MaterialPageRoute(
                  builder: (context) => const ReceiveScannedObjectPage()));
            };
          }
          if (receivedData == "scanner_state CANCELLED") {
            _currentRound = 0;
            _totalRound = 1;
            ScanProvider.instance.cancelScan();
          }
          if (receivedData == "scanner_state FINISHED") {
            ScanProvider.instance.finishScan();
          }
          developer.log('Broadcast socket received data: $receivedData');
        }, onDone: () {
          disconnectSockets();
          developer.log('Broadcast socket closed');
        }, onError: (error) {
          showCustomToast(navigatorKey.currentContext!, "Error in socket!");
        });
      } catch (e) {
        flag = false;
        showCustomToast(navigatorKey.currentContext!, "Error in socket!");
      }
    }

    if (flag) {
      try {
        _imageSocket = await Socket.connect(_ipAddress, _imagePort);

        _imageSocket.listen((List<int> data) {}, onDone: () {
          disconnectSockets();
          developer.log('Image socket closed');
        }, onError: (error) {
          developer.log('Image socket error: $error');
        });
      } catch (e) {
        flag = false;
        showCustomToast(navigatorKey.currentContext!, "Error in socket!");
      }
    }

    if (flag) {
      try {
        _liveSocket = await Socket.connect(_ipAddress, _livePort);

        _liveSocket.listen((List<int> data) async {
          String receivedData = utf8.decode(data);
          developer.log(receivedData);
          if (receivedData == "START_SCANNING") {
            _liveSocket.write("ack");
          }

          if (receivedData.startsWith("ROUND")) {
            List<String> parts = receivedData.split(" ");

            if (parts.length == 3) {
              try {
                _currentRound = int.parse(parts[1]);
                _totalRound = int.parse(parts[2]);
                notifyListeners();
              } catch (e) {
                showCustomToast(
                    navigatorKey.currentContext!, "Error parsing integers!");
              }
            } else {
              developer.log("Invalid data format");
            }
            _liveSocket.write("ack");
          }
          if (receivedData == "FINISH_SCANNING") {
            _liveSocket.write("ack");
          }

          if (receivedData.startsWith("FILE_END")) {
            _fileSize = 0;
            _totalFileSize = 1;
            List<String> parts = receivedData.split(" ");
            try {
              _receivedFileSize = int.parse(parts[1]);
            } catch (e) {
              showCustomToast(
                  navigatorKey.currentContext!, "Error parsing integers!");
            }
            notifyListeners();
            _liveSocket.write("ack");
          } else if (_fileSize > 0) {
            stringBuffer.write(String.fromCharCodes(data));
            _fileSize = _fileSize - receivedData.length;
            notifyListeners();
            _liveSocket.write("ack");
          } else if (receivedData.startsWith("FILE")) {
            _totalRound = 1;
            _currentRound = 0;
            List<String> parts = receivedData.split(" ");
            try {
              int intValue = int.parse(parts[1]);
              _fileSize = intValue;
              _totalFileSize = _fileSize;
              ScanProvider.instance.startReceiving();
            } catch (e) {
              showCustomToast(
                  navigatorKey.currentContext!, "Error parsing integers!");
            }
            _liveSocket.write("ack");
          }
        }, onDone: () {
          disconnectSockets();
          developer.log('Live socket closed');
        }, onError: (error) {
          showCustomToast(navigatorKey.currentContext!, "Error in socket!");
        });
      } catch (e) {
        flag = false;
        showCustomToast(navigatorKey.currentContext!, "Error in socket!");
      }
    }

    if (flag) {
      _isConnected = true;
      notifyListeners();
      developer.log('Sockets connected');
      _serverSocket.write("mobile");
    } else {
      showCustomToast(navigatorKey.currentContext!, "Can not connected!");
    }
  }

  void disconnectSockets() {
    disconnectCommand();
    try {
      _serverSocket.close();
      _configSocket.close();
      _broadcastSocket.close();
      _imageSocket.close();
      _liveSocket.close();
    } catch (e) {
      showCustomToast(navigatorKey.currentContext!, "Error in socket!");
    }
    _isConnected = false;
    notifyListeners();
  }

  void startScanCommand() {
    _configSocket.write("command start");
  }

  void disconnectCommand() {
    _configSocket.write("command disconnect");
  }

  void cancelScanCommand() {
    _configSocket.write("command cancel");
  }

  void showCustomToast(BuildContext context, String message) {
    final scaffold = ScaffoldMessenger.of(context);
    scaffold.showSnackBar(SnackBar(
        content: Text(message, textAlign: TextAlign.center),
        duration: const Duration(seconds: 2),
        backgroundColor: Theme.of(context).disabledColor));
  }
}
