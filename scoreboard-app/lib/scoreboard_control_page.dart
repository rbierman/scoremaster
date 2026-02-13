import 'dart:async';
import 'package:bonsoir/bonsoir.dart';
import 'package:flutter/material.dart';
import 'discovery_dialog.dart';
import 'web_socket_service.dart';
import 'scoreboard_state.dart';
import 'team_configuration_page.dart';

class ScoreboardControlPage extends StatefulWidget {
  const ScoreboardControlPage({super.key});

  @override
  State<ScoreboardControlPage> createState() => _ScoreboardControlPageState();
}

class _ScoreboardControlPageState extends State<ScoreboardControlPage> {
  ScoreboardState? _state;
  WebSocketService? _wsService;
  ConnectionStatus _connectionStatus = ConnectionStatus.disconnected;

  BonsoirDiscovery? _discovery;
  StreamSubscription<BonsoirDiscoveryEvent>? _subscription;
  StreamSubscription<ConnectionStatus>? _connectionSubscription;
  final StreamController<BonsoirDiscoveryEvent> _discoveryStreamController = StreamController<BonsoirDiscoveryEvent>.broadcast();
  final List<BonsoirService> _discoveredServices = [];
  BonsoirService? _connectedService;
  bool _isDiscoverySupported = true;
  String _discoveryError = '';

  final Map<String, TextEditingController> _penaltyControllers = {};
  final Map<String, TextEditingController> _teamNameControllers = {};

  @override
  void initState() {
    super.initState();
    _startDiscovery();
  }

  @override
  void dispose() {
    _stopDiscovery();
    _discoveryStreamController.close();
    _connectionSubscription?.cancel();
    _wsService?.dispose();
    for (var controller in _penaltyControllers.values) {
      controller.dispose();
    }
    for (var controller in _teamNameControllers.values) {
      controller.dispose();
    }
    super.dispose();
  }

  TextEditingController _getPenaltyController(bool isHome, int index, int playerNumber) {
    String key = '${isHome ? "home" : "away"}_$index';
    String newText = playerNumber > 0 ? playerNumber.toString() : '';
    if (!_penaltyControllers.containsKey(key)) {
      _penaltyControllers[key] = TextEditingController(text: newText);
    } else {
      var controller = _penaltyControllers[key]!;
      if (controller.text != newText) {
        // We only update the controller if it's not currently focused
        // to avoid interrupting the user's typing.
        // We use a simple way to check if this controller is focused.
        // In a more complex app we'd use FocusNodes.
      }
    }
    return _penaltyControllers[key]!;
  }

  TextEditingController _getTeamNameController(bool isHome, String teamName) {
    String key = isHome ? "home" : "away";
    if (!_teamNameControllers.containsKey(key)) {
      _teamNameControllers[key] = TextEditingController(text: teamName);
    } else {
      var controller = _teamNameControllers[key]!;
      if (controller.text != teamName) {
        // Same as above
      }
    }
    return _teamNameControllers[key]!;
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
      _connectionSubscription?.cancel();
      _wsService?.dispose();
      _wsService = WebSocketService();
      
      _connectionSubscription = _wsService!.connectionStream.listen((status) {
        setState(() {
          _connectionStatus = status;
        });
      });

      try {
        await _wsService!.connect(host, 9000);
        _wsService!.stateStream.listen((newState) {
          setState(() {
            _state = newState;
          });
        });
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Connecting to ${service.name} ($host:9000)')),
        );
      } catch (e) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to initialize connection on $host:9000')),
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
            icon: const Icon(Icons.people),
            tooltip: 'Configure Teams',
            onPressed: _wsService == null ? null : () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => TeamConfigurationPage(wsService: _wsService!),
                ),
              );
            },
          ),
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
      body: Column(
        children: [
          if (_connectedService != null && _connectionStatus != ConnectionStatus.connected)
            Container(
              color: _connectionStatus == ConnectionStatus.connecting ? Colors.orange : Colors.red,
              width: double.infinity,
              padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 16),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  SizedBox(
                    width: 16,
                    height: 16,
                    child: CircularProgressIndicator(
                      strokeWidth: 2,
                      valueColor: AlwaysStoppedAnimation<Color>(Colors.white.withOpacity(0.8)),
                    ),
                  ),
                  const SizedBox(width: 12),
                  Text(
                    _connectionStatus == ConnectionStatus.connecting 
                        ? 'Connecting to scoreboard...' 
                        : 'Connection lost. Retrying...',
                    style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
                  ),
                ],
              ),
            ),
          Expanded(
            child: _connectedService == null 
              ? _buildDiscoveryBody()
              : _state == null
                ? const Center(child: CircularProgressIndicator())
                : _buildControlBody(),
          ),
        ],
      ),
    );
  }

  Widget _buildDiscoveryBody() {
    return Center(
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
    );
  }

  Widget _buildControlBody() {
    return SingleChildScrollView(
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
    );
  }

  void _showDiscoveryDialog() {
    if (_discovery == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Discovery service is not ready')),
      );
      return;
    }
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
    String timeStr = '${_state!.timeMinutes.toString().padLeft(2, '0')}:${_state!.timeSeconds.toString().padLeft(2, '0')}';
    bool isClockRunning = _state!.isClockRunning;

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            SegmentedButton<ClockMode>(
              segments: const [
                ButtonSegment(value: ClockMode.game, label: Text('Game'), icon: Icon(Icons.sports_hockey)),
                ButtonSegment(value: ClockMode.intermission, label: Text('Intermission'), icon: Icon(Icons.timer_outlined)),
                ButtonSegment(value: ClockMode.timeOfDay, label: Text('Time'), icon: Icon(Icons.access_time)),
              ],
              selected: { _state!.clockMode },
              onSelectionChanged: (Set<ClockMode> newSelection) {
                ClockMode selected = newSelection.first;
                if (selected == ClockMode.game) {
                  _wsService?.sendCommand('setClockMode', value: 'Game');
                } else if (selected == ClockMode.intermission) {
                  // If switching to intermission, common to set to 15:00 if it was at 0
                  if (_state!.timeMinutes == 0 && _state!.timeSeconds == 0) {
                    _wsService?.sendCommand('setTime', minutes: 15, seconds: 0);
                  }
                  _wsService?.sendCommand('setClockMode', value: 'Intermission');
                } else {
                  _wsService?.sendCommand('setClockMode', value: 'TimeOfDay');
                }
              },
            ),
            const SizedBox(height: 16),
            InkWell(
              onTap: isClockRunning ? null : _showSetTimeDialog,
              child: Column(
                children: [
                  Text(timeStr, style: Theme.of(context).textTheme.displayMedium?.copyWith(
                    fontFamily: 'monospace',
                    color: isClockRunning ? null : Colors.blue,
                  )),
                  if (!isClockRunning && _state!.clockMode != ClockMode.timeOfDay)
                    const Text('Tap to set time', style: TextStyle(fontSize: 10, color: Colors.blue)),
                ],
              ),
            ),
            const SizedBox(height: 16),
            if (_state!.clockMode != ClockMode.timeOfDay)
              ElevatedButton.icon(
                onPressed: () => _wsService?.sendCommand('toggleClock'),
                style: ElevatedButton.styleFrom(
                  minimumSize: const Size(200, 50),
                  backgroundColor: isClockRunning ? Colors.orange : Colors.green,
                  foregroundColor: Colors.white,
                ),
                icon: Icon(isClockRunning ? Icons.pause : Icons.play_arrow),
                label: Text(isClockRunning ? 'PAUSE CLOCK' : 'START CLOCK', 
                  style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 18)),
              ),
          ],
        ),
      ),
    );
  }

  void _showSetTimeDialog() {
    final minController = TextEditingController(text: _state!.timeMinutes.toString());
    final secController = TextEditingController(text: _state!.timeSeconds.toString());

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Set Remaining Time'),
        content: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Expanded(
              child: TextField(
                controller: minController,
                decoration: const InputDecoration(labelText: 'Min'),
                keyboardType: TextInputType.number,
              ),
            ),
            const Text(' : ', style: TextStyle(fontSize: 24)),
            Expanded(
              child: TextField(
                controller: secController,
                decoration: const InputDecoration(labelText: 'Sec'),
                keyboardType: TextInputType.number,
              ),
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              int mins = int.tryParse(minController.text) ?? 0;
              int secs = int.tryParse(secController.text) ?? 0;
              _wsService?.sendCommand('setTime', minutes: mins, seconds: secs);
              Navigator.pop(context);
            },
            child: const Text('Set'),
          ),
        ],
      ),
    );
  }

  Widget _buildTeamSection(bool isHome) {
    int score = isHome ? _state!.homeScore : _state!.awayScore;
    int shots = isHome ? _state!.homeShots : _state!.awayShots;
    String teamName = isHome ? _state!.homeTeamName : _state!.awayTeamName;
    final controller = _getTeamNameController(isHome, teamName);

    return Column(
      children: [
        TextField(
          maxLength: 8,
          decoration: InputDecoration(
            labelText: isHome ? 'Home Team' : 'Away Team',
            border: const OutlineInputBorder(),
            counterText: "",
          ),
          controller: controller,
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
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Expanded(child: _buildTeamPenalties(true)),
            const SizedBox(width: 16),
            Expanded(child: _buildTeamPenalties(false)),
          ],
        ),
      ],
    );
  }

  Widget _buildTeamPenalties(bool isHome) {
    List<Penalty> penalties = isHome ? _state!.homePenalties : _state!.awayPenalties;
    bool isClockRunning = _state!.isClockRunning;

    return Column(
      children: List.generate(2, (index) {
        Penalty p = penalties[index];
        bool isActive = p.secondsRemaining > 0;
        final controller = _getPenaltyController(isHome, index, p.playerNumber);

        return Card(
          color: isActive ? Colors.red.withOpacity(0.1) : null,
          child: Padding(
            padding: const EdgeInsets.all(8.0),
            child: Column(
              children: [
                Row(
                  children: [
                    Expanded(
                      flex: 2,
                      child: TextField(
                        enabled: !isClockRunning,
                        decoration: InputDecoration(
                          labelText: 'Player #', 
                          isDense: true,
                          helperText: isClockRunning ? 'Stop clock to edit' : null,
                          helperStyle: const TextStyle(fontSize: 10),
                        ),
                        keyboardType: TextInputType.number,
                        controller: controller,
                        onChanged: (val) {
                          int player = int.tryParse(val) ?? 0;
                          _wsService?.sendCommand(isHome ? 'setHomePenalty' : 'setAwayPenalty', 
                            index: index, value: p.secondsRemaining, player: player);
                        },
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      flex: 3,
                      child: Text(
                        '${(p.secondsRemaining ~/ 60)}:${(p.secondsRemaining % 60).toString().padLeft(2, '0')}',
                        style: const TextStyle(fontFamily: 'monospace', fontWeight: FontWeight.bold),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    _penaltyTimeButton(isHome, index, controller, 120, '2:00', !isClockRunning),
                    _penaltyTimeButton(isHome, index, controller, 300, '5:00', !isClockRunning),
                    IconButton(
                      icon: const Icon(Icons.clear, size: 20),
                      onPressed: isClockRunning ? null : () {
                        controller.clear();
                        _wsService?.sendCommand(isHome ? 'setHomePenalty' : 'setAwayPenalty', 
                          index: index, value: 0, player: 0);
                      },
                    ),
                  ],
                ),
              ],
            ),
          ),
        );
      }),
    );
  }

  Widget _penaltyTimeButton(bool isHome, int index, TextEditingController controller, int seconds, String label, bool enabled) {
    return TextButton(
      style: TextButton.styleFrom(padding: EdgeInsets.zero, minimumSize: const Size(40, 30)),
      onPressed: enabled ? () {
        int player = int.tryParse(controller.text) ?? 0;
        _wsService?.sendCommand(isHome ? 'setHomePenalty' : 'setAwayPenalty', 
          index: index, value: seconds, player: player);
      } : null,
      child: Text(label, style: TextStyle(fontSize: 12, color: enabled ? null : Colors.grey)),
    );
  }
}
