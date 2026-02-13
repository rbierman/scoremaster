import 'package:flutter/material.dart';
import 'scoreboard_control_page.dart';

void main() {
  runApp(const ScoreboardControlApp());
}

class ScoreboardControlApp extends StatelessWidget {
  const ScoreboardControlApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Hockey Scoreboard Control',
      theme: ThemeData(
        brightness: Brightness.dark,
        primarySwatch: Colors.blue,
        useMaterial3: true,
      ),
      home: const ScoreboardControlPage(),
    );
  }
}
