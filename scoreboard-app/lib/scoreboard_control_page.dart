import 'dart:async';
import 'package:bonsoir/bonsoir.dart';
import 'package:flutter/material.dart';
import 'discovery_dialog.dart';

class ScoreboardControlPage extends StatefulWidget {
  const ScoreboardControlPage({super.key});

  @override
  State<ScoreboardControlPage> createState() => _ScoreboardControlPageState();
}

class _ScoreboardControlPageState extends State<ScoreboardControlPage> {
  int homeScore = 0;
  int awayScore = 0;
  int homeShots = 0;
  int awayShots = 0;
  String homeTeam = "HOME";
  String awayTeam = "AWAY";
  String clockMode = "Stopped";
  int period = 1;

  BonsoirDiscovery? _discovery;
  StreamSubscription<BonsoirDiscoveryEvent>? _subscription;
  final StreamController<BonsoirDiscoveryEvent> _discoveryStreamController = StreamController<BonsoirDiscoveryEvent>.broadcast();
  final List<BonsoirService> _discoveredServices = [];
  BonsoirService? _connectedService;
  bool _isDiscoverySupported = true;
  String _discoveryError = '';

  @override
  void initState() {
    super.initState();
    _startDiscovery();
  }

  @override
  void dispose() {
    _stopDiscovery();
    _discoveryStreamController.close();
    super.dispose();
  }

  Future<void> _startDiscovery() async {
    try {
      _discovery = BonsoirDiscovery(type: '_hockey-score._tcp');
      await _discovery!.initialize();
      
      final eventStream = _discovery!.eventStream;
      if (eventStream != null) {
        _subscription = eventStream.listen((event) {
          _discoveryStreamController.add(event);
          
          final service = event.service;
          if (service == null) return;
          debugPrint('[Main Discovery] Event: ${event.runtimeType} for ${service.name}');
          debugPrint('[Main Discovery] Service JSON: ${service.toJson()}');

          switch (event) {
            case BonsoirDiscoveryServiceFoundEvent():
              debugPrint('[Main Discovery] Found service: ${service.name}. Resolved: ${service.toJson()['service.host'] != null}');
              setState(() {
                if (!_discoveredServices.any((s) => s.name == service.name)) {
                  _discoveredServices.add(service);
                }
              });
              service.resolve(_discovery!.serviceResolver);
              break;
            case BonsoirDiscoveryServiceResolvedEvent():
              debugPrint('[Main Discovery] Resolved service: ${service.name}. Host: ${service.toJson()['service.host']}');
              setState(() {
                int index = _discoveredServices.indexWhere((s) => s.name == service.name);
                if (index != -1) {
                  _discoveredServices[index] = service;
                }
              });
              break;
            case BonsoirDiscoveryServiceLostEvent():
              setState(() {
                _discoveredServices.removeWhere((s) => s.name == service.name);
                if (_connectedService?.name == service.name) {
                  _connectedService = null;
                }
              });
              break;
            default:
              break;
          }
        });
        await _discovery!.start();
      } else {
        setState(() {
          _isDiscoverySupported = false;
          _discoveryError = 'Discovery event stream is null after initialization.';
        });
      }
    } catch (e) {
      setState(() {
        _isDiscoverySupported = false;
        _discoveryError = e.toString();
      });
      debugPrint('Discovery error: $e');
    }
  }

  Future<void> _stopDiscovery() async {
    await _subscription?.cancel();
    await _discovery?.stop();
  }

  void _connectToService(BonsoirService service) {
    setState(() {
      _connectedService = service;
    });
    final json = service.toJson();
    String host = json['service.host'] ?? "Unknown";
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Connected to ${service.name} ($host)')),
    );
  }

  void _updateScore(bool isHome, int delta) {
    setState(() {
      if (isHome) {
        homeScore = (homeScore + delta).clamp(0, 99);
      } else {
        awayScore = (awayScore + delta).clamp(0, 99);
      }
    });
  }

  void _updateShots(bool isHome, int delta) {
    setState(() {
      if (isHome) {
        homeShots = (homeShots + delta).clamp(0, 99);
      } else {
        awayShots = (awayShots + delta).clamp(0, 99);
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Scoreboard Control'),
        actions: [
          IconButton(
            icon: Icon(_connectedService == null ? Icons.link_off : Icons.link),
            onPressed: _showDiscoveryDialog,
          ),
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () {
              setState(() {
                homeScore = 0;
                awayScore = 0;
                homeShots = 0;
                awayShots = 0;
                clockMode = "Stopped";
                period = 1;
              });
            },
          ),
        ],
      ),
      body: _connectedService == null 
        ? Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(_isDiscoverySupported ? Icons.search : Icons.error_outline, 
                     size: 64, 
                     color: _isDiscoverySupported ? Colors.grey : Colors.red),
                const SizedBox(height: 16),
                Text(_isDiscoverySupported 
                    ? 'Search for a scoreboard...' 
                    : 'Discovery Error: $_discoveryError'),
                const SizedBox(height: 16),
                if (_isDiscoverySupported)
                  ElevatedButton(
                    onPressed: _showDiscoveryDialog,
                    child: const Text('Browse Scoreboards'),
                  ),
              ],
            ),
          )
        : SingleChildScrollView(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _buildClockControls(),
                const SizedBox(height: 24),
                Row(
                  children: [
                    Expanded(child: _buildTeamSection(true)),
                    const SizedBox(width: 16),
                    Expanded(child: _buildTeamSection(false)),
                  ],
                ),
                const SizedBox(height: 24),
                _buildPeriodControls(),
                const SizedBox(height: 24),
                _buildPenaltySection(),
              ],
            ),
          ),
    );
  }

  void _showDiscoveryDialog() {
    showDialog(
      context: context,
      builder: (context) => ScoreboardDiscoveryDialog(
        discovery: _discovery!,
        eventStream: _discoveryStreamController.stream,
        discoveredServices: _discoveredServices,
        onServiceSelected: (service) {
          _connectToService(service);
          Navigator.pop(context);
        },
      ),
    );
  }

  Widget _buildClockControls() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Text('Clock Mode: $clockMode', style: Theme.of(context).textTheme.headlineSmall),
            const SizedBox(height: 16),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                ElevatedButton.icon(
                  onPressed: () => setState(() => clockMode = "Running"),
                  icon: const Icon(Icons.play_arrow),
                  label: const Text('Start'),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.green.withOpacity(0.2)),
                ),
                ElevatedButton.icon(
                  onPressed: () => setState(() => clockMode = "Stopped"),
                  icon: const Icon(Icons.stop),
                  label: const Text('Stop'),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.red.withOpacity(0.2)),
                ),
                ElevatedButton.icon(
                  onPressed: () => setState(() => clockMode = "Clock"),
                  icon: const Icon(Icons.access_time),
                  label: const Text('Time'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTeamSection(bool isHome) {
    return Column(
      children: [
        TextField(
          decoration: InputDecoration(
            labelText: isHome ? 'Home Team' : 'Away Team',
            border: const OutlineInputBorder(),
          ),
          onChanged: (val) => setState(() => isHome ? homeTeam = val : awayTeam = val),
        ),
        const SizedBox(height: 16),
        Text('Score', style: Theme.of(context).textTheme.titleMedium),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            IconButton(onPressed: () => _updateScore(isHome, -1), icon: const Icon(Icons.remove)),
            Text('${isHome ? homeScore : awayScore}', style: Theme.of(context).textTheme.headlineMedium),
            IconButton(onPressed: () => _updateScore(isHome, 1), icon: const Icon(Icons.add)),
          ],
        ),
        const SizedBox(height: 8),
        Text('Shots', style: Theme.of(context).textTheme.titleMedium),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            IconButton(onPressed: () => _updateShots(isHome, -1), icon: const Icon(Icons.remove)),
            Text('${isHome ? homeShots : awayShots}', style: Theme.of(context).textTheme.titleMedium),
            IconButton(onPressed: () => _updateShots(isHome, 1), icon: const Icon(Icons.add)),
          ],
        ),
      ],
    );
  }

  Widget _buildPeriodControls() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text('Period: $period', style: Theme.of(context).textTheme.titleLarge),
            Row(
              children: [
                IconButton(
                  onPressed: () => setState(() => period = (period > 1) ? period - 1 : 1),
                  icon: const Icon(Icons.remove),
                ),
                IconButton(
                  onPressed: () => setState(() => period = (period < 4) ? period + 1 : 4),
                  icon: const Icon(Icons.add),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPenaltySection() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('Penalties', style: Theme.of(context).textTheme.titleLarge),
        const SizedBox(height: 8),
        Row(
          children: [
            Expanded(
              child: ElevatedButton(
                onPressed: () {},
                child: const Text('Add Home Penalty'),
              ),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: ElevatedButton(
                onPressed: () {},
                child: const Text('Add Away Penalty'),
              ),
            ),
          ],
        ),
      ],
    );
  }
}
