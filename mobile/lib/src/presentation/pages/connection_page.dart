import 'dart:io';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:Scanner3D/src/presentation/widgets/button_style.dart';
import 'package:Scanner3D/src/services/socket_service.dart';

class ConnectionPage extends StatefulWidget {
  const ConnectionPage({super.key});

  @override
  State<ConnectionPage> createState() => _ConnectionPageState();
}

class _ConnectionPageState extends State<ConnectionPage> {
  final TextEditingController _ipController = TextEditingController();
  final GlobalKey<FormState> _formKey = GlobalKey<FormState>();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        body: Container(
            alignment: Alignment.center,
            padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 60),
            child:
                Consumer<SocketService>(builder: (context, socketService, _) {
              return socketService.isConnected
                  ? _buildConnectionInfoScreen()
                  : _buildConnectScreen();
            })));
  }

  @override
  void dispose() {
    _ipController.dispose();
    super.dispose();
  }

  Widget _buildConnectScreen() {
    return Column(children: [
      Expanded(
        flex: 10,
        child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
          Text("Connect to Device",
              style: TextStyle(
                  fontSize: 24, color: Theme.of(context).shadowColor)),
          const SizedBox(height: 20),
          Form(
              key: _formKey,
              child: TextFormField(
                  cursorColor: Theme.of(context).primaryColor,
                  controller: _ipController,
                  decoration: InputDecoration(
                      focusedBorder: getBorder(Theme.of(context).primaryColor),
                      enabledBorder: getBorder(Theme.of(context).shadowColor),
                      focusedErrorBorder:
                          getBorder(Theme.of(context).disabledColor),
                      errorBorder: getBorder(Theme.of(context).disabledColor),
                      labelText: 'Enter Server IP',
                      labelStyle:
                          TextStyle(color: Theme.of(context).shadowColor),
                      hintText: "127.0.0.1"),
                  validator: (value) {
                    if (value == null || value.isEmpty) {
                      return 'Please enter the server IP';
                    }
                    if (!isValidIpAddress(value)) {
                      return 'Invalid IP address format';
                    }
                    return null;
                  }))
        ]),
      ),
      Expanded(
          flex: 0,
          child: ButtonStyles().button("Connect", () {
            if (_formKey.currentState!.validate()) {
              SocketService.instance.connectSocket(_ipController.text);
            }
          }, Theme.of(context).primaryColor))
    ]);
  }

  Widget _buildConnectionInfoScreen() {
    return Column(children: [
      Expanded(
        flex: 10,
        child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
          Text("Hardware IP Address:",
              style: TextStyle(
                  fontSize: 24, color: Theme.of(context).shadowColor)),
          const SizedBox(height: 20),
          Consumer<SocketService>(builder: (context, socketService, _) {
            return Text(
              socketService.ipAddress,
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.w600,
              ),
            );
          })
        ]),
      ),
      Expanded(
          flex: 0,
          child: ButtonStyles().button("Disconnect", () {
            SocketService.instance.disconnectSockets();
          }, Theme.of(context).disabledColor))
    ]);
  }

  OutlineInputBorder getBorder(Color color) {
    return OutlineInputBorder(
      borderSide: BorderSide(color: color, width: 1.0),
    );
  }

  bool isValidIpAddress(String value) {
    try {
      InternetAddress(value);
      return true;
    } catch (_) {
      return false;
    }
  }
}
