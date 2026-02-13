import 'dart:async';
import 'package:bonsoir/bonsoir.dart';
import 'package:flutter/material.dart';
import 'discovery_dialog.dart';
import 'web_socket_service.dart';
import 'scoreboard_state.dart';

class ScoreboardControlPage extends StatefulWidget {
  const ScoreboardControlPage({super.key});

  @override
  State<ScoreboardControlPage> createState() => _ScoreboardControlPageState();
}

class _ScoreboardControlPageState extends State<ScoreboardControlPage> {
  ScoreboardState? _state;
  WebSocketService? _wsService;

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
    _wsService?.dispose();
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

          switch (event) {
            case BonsoirDiscoveryServiceFoundEvent():
              setState(() {
                if (!_discoveredServices.any((s) => s.name == service.name)) {
                  _discoveredServices.add(service);
                }
              });
              service.resolve(_discovery!.serviceResolver);
              break;
            case BonsoirDiscoveryServiceResolvedEvent():
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
                  _wsService?.dispose();
                  _wsService = null;
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
    }
  }

  Future<void> _stopDiscovery() async {
    await _subscription?.cancel();
    await _discovery?.stop();
  }

  void _connectToService(BonsoirService service) async {
    setState(() {
      _connectedService = service;
    });
    
    final json = service.toJson();
    String? host = json['service.host'];
    
    if (host != null) {
      _wsService?.dispose();
      _wsService = WebSocketService();
      try {
        await _wsService!.connect(host, 9000);
        _wsService!.stateStream.listen((newState) {
          setState(() {
            _state = newState;
          });
        });
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Connected to ${service.name} ($host:9000)')),
        );
      } catch (e) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to connect to WebSocket on $host:9000')),
        );
      }
    }
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
            onPressed: () => _wsService?.sendCommand('resetGame'),
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
        : _state == null
          ? const Center(child: CircularProgressIndicator())
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
    String modeStr = _state!.clockMode.toString().split('.').last;
    String timeStr = '${_state!.timeMinutes.toString().padLeft(2, '0')}:${_state!.timeSeconds.toString().padLeft(2, '0')}';

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Text(timeStr, style: Theme.of(context).textTheme.displayMedium?.copyWith(fontFamily: 'monospace')),
            Text('Mode: $modeStr', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 16),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                ElevatedButton.icon(
                  onPressed: () => _wsService?.sendCommand('toggleClock'),
                  icon: Icon(_state!.clockMode == ClockMode.running ? Icons.pause : Icons.play_arrow),
                  label: Text(_state!.clockMode == ClockMode.running ? 'Pause' : 'Start'),
                ),
                ElevatedButton.icon(
                  onPressed: () => _wsService?.sendCommand('setClockMode', value: 'Clock'),
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
    int score = isHome ? _state!.homeScore : _state!.awayScore;
    int shots = isHome ? _state!.homeShots : _state!.awayShots;
    String teamName = isHome ? _state!.homeTeamName : _state!.awayTeamName;

    return Column(
      children: [
        TextField(
          maxLength: 8,
          decoration: InputDecoration(
            labelText: isHome ? 'Home Team' : 'Away Team',
            border: const OutlineInputBorder(),
            counterText: "",
          ),
          controller: TextEditingController.fromValue(
            TextEditingValue(
              text: teamName,
              selection: TextSelection.collapsed(offset: teamName.length),
            ),
          ),
          onChanged: (val) {
            _wsService?.sendCommand(
              isHome ? 'setHomeTeamName' : 'setAwayTeamName',
              value: val,
            );
          },
        ),
        const SizedBox(height: 16),
        Text('Score', style: Theme.of(context).textTheme.titleMedium),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            IconButton(
              onPressed: () => _wsService?.sendCommand(isHome ? 'addHomeScore' : 'addAwayScore', delta: -1),
              icon: const Icon(Icons.remove)
            ),
            Text('$score', style: Theme.of(context).textTheme.headlineMedium),
            IconButton(
              onPressed: () => _wsService?.sendCommand(isHome ? 'addHomeScore' : 'addAwayScore', delta: 1),
              icon: const Icon(Icons.add)
            ),
          ],
        ),
        const SizedBox(height: 8),
        Text('Shots', style: Theme.of(context).textTheme.titleMedium),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            IconButton(
              onPressed: () => _wsService?.sendCommand(isHome ? 'addHomeShots' : 'addAwayShots', delta: -1),
              icon: const Icon(Icons.remove)
            ),
            Text('$shots', style: Theme.of(context).textTheme.titleMedium),
            IconButton(
              onPressed: () => _wsService?.sendCommand(isHome ? 'addHomeShots' : 'addAwayShots', delta: 1),
              icon: const Icon(Icons.add)
            ),
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
            Text('Period: ${_state!.currentPeriod}', style: Theme.of(context).textTheme.titleLarge),
            ElevatedButton(
              onPressed: () => _wsService?.sendCommand('nextPeriod'),
              child: const Text('Next Period'),
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
                onPressed: () => _wsService?.sendCommand('addHomePenalty', value: 120), // 2 mins
                child: const Text('Add Home 2:00'),
              ),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: ElevatedButton(
                onPressed: () => _wsService?.sendCommand('addAwayPenalty', value: 120),
                child: const Text('Add Away 2:00'),
              ),
            ),
          ],
        ),
      ],
    );
  }
}
