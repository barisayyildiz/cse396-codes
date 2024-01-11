import 'package:flutter/material.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:provider/provider.dart';
import 'package:Scanner3D/src/presentation/icons/scanner_icons.dart';
import 'package:Scanner3D/src/presentation/pages/connection_page.dart';
import 'package:Scanner3D/src/presentation/pages/current_scan_page.dart';
import 'package:Scanner3D/src/presentation/pages/scan_list_page.dart';
import 'package:Scanner3D/src/presentation/pages/splash_view_page.dart';
import 'package:Scanner3D/src/presentation/widgets/connection_status_app_bar.dart';
import 'package:Scanner3D/src/services/notification_service.dart';
import 'package:Scanner3D/src/services/scan_provider.dart';
import 'package:Scanner3D/src/services/socket_service.dart';

final FlutterLocalNotificationsPlugin flutterLocalNotificationsPlugin =
    FlutterLocalNotificationsPlugin();
final GlobalKey<NavigatorState> navigatorKey = GlobalKey<NavigatorState>();

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  runApp(MultiProvider(providers: [
    ChangeNotifierProvider(create: (_) => ScanProvider.instance),
    ChangeNotifierProvider(create: (_) => SocketService.instance),
  ], child: const MyApp()));

  await NotificationService.initialize(
      flutterLocalNotificationsPlugin, navigatorKey);
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
        navigatorKey: navigatorKey,
        title: 'Scanner 3D',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
            primaryColor: const Color.fromARGB(255, 36, 161, 157),
            disabledColor: const Color.fromARGB(255, 193, 29, 29),
            shadowColor: const Color.fromARGB(255, 112, 112, 112),
            highlightColor: const Color.fromARGB(255, 58, 58, 58)),
        home: const SplashViewPage());
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  int _selectedIndex = 0;

  final List<Widget> _pageOptions = <Widget>[
    const ConnectionPage(),
    const CurrentScanPage(),
    const ScanListPage()
  ];

  void _onItemTapped(int index) {
    setState(() {
      _selectedIndex = index;
    });
  }

  @override
  void initState() {
    super.initState();
  }

  @override
  void dispose() {
    SocketService.instance.disconnectSockets();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        appBar: const ConnectionStatusAppBar(),
        body: _pageOptions.elementAt(_selectedIndex),
        bottomNavigationBar: BottomNavigationBar(
            iconSize: 28,
            useLegacyColorScheme: false,
            items: const [
              BottomNavigationBarItem(
                  icon: Icon(Icons.compare_arrows), label: 'Connect'),
              BottomNavigationBarItem(icon: Icon(Scanner.svg), label: 'Scan'),
              BottomNavigationBarItem(
                  icon: Icon(Icons.view_list), label: 'Your Scans')
            ],
            currentIndex: _selectedIndex,
            selectedItemColor: Theme.of(context).primaryColor,
            unselectedItemColor: Theme.of(context).shadowColor,
            onTap: _onItemTapped));
  }
}
