import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:Scanner3D/src/services/socket_service.dart';

class ConnectionStatusAppBar extends StatelessWidget
    implements PreferredSizeWidget {
  const ConnectionStatusAppBar({super.key});

  @override
  Size get preferredSize => const Size.fromHeight(kToolbarHeight);

  @override
  Widget build(BuildContext context) {
    return Consumer<SocketService>(builder: (context, socketService, _) {
      return AppBar(
          backgroundColor: socketService.isConnected
              ? Theme.of(context).primaryColor
              : Theme.of(context).disabledColor,
          title: Text(socketService.isConnected ? "Connected" : "No connection",
              style: const TextStyle(color: Colors.white)),
          centerTitle: true);
    });
  }
}
