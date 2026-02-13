import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';
import 'package:bonsoir/bonsoir.dart';
import 'package:flutter/material.dart';
import 'discovery_dialog.dart';
import 'web_socket_service.dart';
import 'scoreboard_state.dart';
import 'team_model.dart';
import 'team_configuration_page.dart';

class ScoreboardControlPage extends StatefulWidget {
  const ScoreboardControlPage({super.key});

  @override
  State<ScoreboardControlPage> createState() => _ScoreboardControlPageState();
}

class _ScoreboardControlPageState extends State<ScoreboardControlPage> {
  ScoreboardState? _state;
  List<Team> _teams = [];
  WebSocketService? _wsService;
  ConnectionStatus _connectionStatus = ConnectionStatus.disconnected;

  BonsoirDiscovery? _discovery;
  StreamSubscription<BonsoirDiscoveryEvent>? _subscription;
  StreamSubscription<ConnectionStatus>? _connectionSubscription;
  StreamSubscription<List<Team>>? _teamsSubscription;
  StreamSubscription<Map<String, dynamic>>? _imageSubscription;

  final Map<String, Uint8List> _imageCache = {};

  final StreamController<BonsoirDiscoveryEvent> _discoveryStreamController = StreamController<BonsoirDiscoveryEvent>.broadcast();
  final List<BonsoirService> _discoveredServices = [];
  BonsoirService? _connectedService;
  bool _isDiscoverySupported = true;
  String _discoveryError = '';

  final Map<String, TextEditingController> _penaltyControllers = {};
  final Map<String, TextEditingController> _teamNameControllers = {};
  final Map<String, FocusNode> _teamFocusNodes = {};

  @override
  void initState() {
    super.initState();
    _teamFocusNodes['home'] = FocusNode();
    _teamFocusNodes['away'] = FocusNode();
    _startDiscovery();
  }

  @override
  void dispose() {
    _stopDiscovery();
    _discoveryStreamController.close();
    _connectionSubscription?.cancel();
    _teamsSubscription?.cancel();
    _imageSubscription?.cancel();
    _wsService?.dispose();
    for (var node in _teamFocusNodes.values) {
      node.dispose();
    }
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

      _teamsSubscription = _wsService!.teamsStream.listen((teams) {
        setState(() {
          _teams = teams;
        });
        // Request images for players that have them
        for (var team in teams) {
          for (var player in team.players) {
            if (player.hasImage) {
              // Always request to ensure we have the latest (in case it changed)
              _wsService!.getPlayerImage(team.name, player.number);
            }
          }
        }
      });

      _imageSubscription = _wsService!.imageStream.listen((imageData) {
        String team = imageData['team'];
        int number = imageData['number'];
        String? base64 = imageData['data'];
        if (base64 != null) {
          setState(() {
            _imageCache["${team}_$number"] = base64Decode(base64);
          });
        }
      });

      try {
        int port = service.port;
        await _wsService!.connect(host, port);
        _wsService!.stateStream.listen((newState) {
          setState(() {
            _state = newState;
          });
        });
        _wsService!.getTeams();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Connecting to ${service.name} ($host:$port)')),
        );
      } catch (e) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to initialize connection on $host:${service.port}')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('ScoreMaster'),
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
            tooltip: 'Reset Game',
            onPressed: () => _showResetConfirmation(),
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

  void _showResetConfirmation() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Reset Game?'),
        content: const Text('This will reset scores, shots, and the clock to their initial values. Are you sure?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              _wsService?.sendCommand('resetGame');
              Navigator.pop(context);
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.red,
              foregroundColor: Colors.white,
            ),
            child: const Text('Reset'),
          ),
        ],
      ),
    );
  }

  void _showGoalPlayerSelection(bool isHome) {
    String teamName = isHome ? _state!.homeTeamName : _state!.awayTeamName;
    print('Looking for team: $teamName among ${_teams.length} teams');
    for (var t in _teams) {
      print('Available team: ${t.name}');
    }

    Team? team = _teams.cast<Team?>().firstWhere((t) => t?.name == teamName, orElse: () => null);

    if (team == null || team.players.isEmpty) {
      print('Team not found or has no players. Triggering generic goal.');
      // If no roster is configured, just trigger a generic goal
      _wsService?.triggerGoal(isHome);
      return;
    }

    // Filter out placeholder players (number 0, name "New Team")
    final actualPlayers = team.players.where((p) => !(p.number == 0 && p.name == "New Team")).toList();
    
    // Sort players by number
    actualPlayers.sort((a, b) => a.number.compareTo(b.number));

    print('Showing grid for team: ${team.name} with ${actualPlayers.length} players');
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Scored by ($teamName)'),
        content: SizedBox(
          width: 400, // Fixed width for grid
          child: GridView.builder(
            shrinkWrap: true,
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 3,
              mainAxisSpacing: 10,
              crossAxisSpacing: 10,
              childAspectRatio: 0.8,
            ),
            itemCount: actualPlayers.length + 1,
            itemBuilder: (context, index) {
              if (index == actualPlayers.length) {
                return InkWell(
                  onTap: () {
                    if (isHome) {
                      _wsService?.addHomeScore();
                    } else {
                      _wsService?.addAwayScore();
                    }
                    Navigator.pop(context);
                  },
                  child: const Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      CircleAvatar(
                        radius: 30,
                        child: Icon(Icons.help_outline),
                      ),
                      SizedBox(height: 4),
                      Text('Unknown', style: TextStyle(fontSize: 12)),
                    ],
                  ),
                );
              }
              
              final player = actualPlayers[index];
              Uint8List? imageBytes = _imageCache["${teamName}_${player.number}"];

              return InkWell(
                onTap: () {
                  _wsService?.triggerGoal(isHome, playerNumber: player.number);
                  Navigator.pop(context);
                },
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    CircleAvatar(
                      radius: 30,
                      backgroundImage: imageBytes != null ? MemoryImage(imageBytes) : null,
                      child: imageBytes == null ? Text('${player.number}') : null,
                    ),
                    const SizedBox(height: 4),
                    Text(player.name, 
                      style: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold),
                      overflow: TextOverflow.ellipsis,
                    ),
                    Text('#${player.number}', style: const TextStyle(fontSize: 10)),
                  ],
                ),
              );
            },
          ),
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
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
    bool isValidTeam = _teams.any((t) => t.name == teamName);

    return Column(
      children: [
        RawAutocomplete<String>(
          optionsBuilder: (TextEditingValue textEditingValue) {
            return _teams
                .map((t) => t.name)
                .where((name) => name.toLowerCase().contains(textEditingValue.text.toLowerCase()));
          },
          onSelected: (String selection) {
            _wsService?.sendCommand(
              isHome ? 'setHomeTeamName' : 'setAwayTeamName',
              value: selection,
            );
          },
          textEditingController: controller,
          focusNode: _teamFocusNodes[isHome ? 'home' : 'away']!,
          optionsViewBuilder: (context, onSelected, options) {
            return Align(
              alignment: Alignment.topLeft,
              child: Material(
                elevation: 4.0,
                child: SizedBox(
                  width: 250,
                  child: ListView.builder(
                    padding: EdgeInsets.zero,
                    shrinkWrap: true,
                    itemCount: options.length,
                    itemBuilder: (BuildContext context, int index) {
                      final String option = options.elementAt(index);
                      return ListTile(
                        title: Text(option),
                        onTap: () => onSelected(option),
                      );
                    },
                  ),
                ),
              ),
            );
          },
          fieldViewBuilder: (context, textEditingController, focusNode, onFieldSubmitted) {
            return TextField(
              controller: textEditingController,
              focusNode: focusNode,
              maxLength: 8,
              decoration: InputDecoration(
                labelText: isHome ? 'Home Team' : 'Away Team',
                border: const OutlineInputBorder(),
                counterText: "",
                suffixIcon: isValidTeam ? const Icon(Icons.check_circle, color: Colors.green, size: 16) : null,
              ),
              onChanged: (val) {
                _wsService?.sendCommand(
                  isHome ? 'setHomeTeamName' : 'setAwayTeamName',
                  value: val,
                );
              },
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
        const SizedBox(height: 4),
        ElevatedButton(
          onPressed: isValidTeam ? () => _showGoalPlayerSelection(isHome) : null,
          style: ElevatedButton.styleFrom(
            backgroundColor: isValidTeam ? Colors.red : Colors.grey.shade800,
            foregroundColor: isValidTeam ? Colors.white : Colors.grey.shade500,
            padding: const EdgeInsets.symmetric(horizontal: 24),
            minimumSize: const Size(100, 36),
          ),
          child: const Text('GOAL', style: TextStyle(fontWeight: FontWeight.bold)),
        ),
        const SizedBox(height: 16),
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
